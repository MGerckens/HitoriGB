// Max Gerckens 10/2021
// Board generation algorithm uses portions from GNOME and Simon Tatham's implementations

#include <gb/gb.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <rand.h>
#include <gb/crash_handler.h>

#include "title_tiles.h"
#include "title_layout.h"
#include "board_tiles.h"
#include "game_board_layout.h"
#include "cursor_tile.h"
#include "generator.h"

#define OFFSET_X (10-(board_size/2))
#define OFFSET_Y (9-(board_size/2))

uint8_t board_size = 5;
uint8_t num_tiles;

enum {EMPTY, BLACK, CIRCLE};
uint8_t cursor[2] = {0,0};
uint8_t *board; 
uint8_t *marks;
uint8_t *solution;

void process_input();
//bool generate_board();
//bool check_placement(uint8_t xi, uint8_t yi);
//uint8_t fill(uint8_t xi, uint8_t yi);
void title_input();

void main(void)
{
    DISPLAY_ON;
	SPRITES_8x8;
	set_bkg_data(0,45,title_tiles); //load title screen
	set_bkg_tiles(0,0,20,18,title_layout);
	SHOW_BKG;
	while(!(joypad()&J_START)){
		title_input();
	}; //wait on title screen
	num_tiles = board_size*board_size;
	
	board = (uint8_t *)calloc( board_size, board_size*sizeof(uint8_t)); //dynamically allocate all arrays based on board size
	marks = (uint8_t *)malloc( num_tiles*sizeof(uint8_t)); 
	solution = (uint8_t *)malloc( num_tiles*sizeof(uint8_t));
	
	set_bkg_data(0,22,board_tiles);
	set_bkg_tiles(0,0,20,18,board_layout);
	
	initarand(clock());
	do{generate_board();}while(!has_single_white_region()); //i have no idea why these boards keep getting through this far
	set_bkg_tile_xy(0,6,0x15); //clean up stray tiles
	set_bkg_tile_xy(0,8,0x15);
	set_bkg_tile_xy(0,9,0x15);
	
	set_sprite_data(0,2,cursor_tile); //load cursor sprite
	set_sprite_tile(0,0);
	move_sprite(0,64,64);
	SHOW_SPRITES;
	
	for(int i = 0; i < board_size; i++){
		for(int j = 0; j < board_size; j++){
			marks[i*board_size + j] = 0;
			set_bkg_tile_xy(i+OFFSET_X,j+OFFSET_Y,board[i*board_size + j]); //write generated board to screen
		}
	}
	
	while(1) {
		process_input();
        wait_vbl_done();
    }
}

void title_input(){
	uint8_t input = joypad();
	static uint8_t pressed = 0;
	if((input & J_LEFT) && (board_size > 5) && !(pressed & J_LEFT)){
		board_size--;
	}
	if((input & J_RIGHT) && (board_size < 10) && !(pressed & J_RIGHT)){
		board_size++;
	}
	set_bkg_tile_xy(9,9,board_size+10);
	pressed = input;
}

#define BUTTON_DOWN(x) ((input & (x)) && !(pressed & (x)))
#define CHECK(x) ((x==1) ? 1 : 0)
void process_input(){
	uint8_t input = joypad();
	uint8_t coord;
	static uint8_t pressed = 0;
	static bool solution_up = false;
	if(BUTTON_DOWN(J_RIGHT)){ //move in the specified direction, with bounds checking
		if(cursor[0] < board_size-1){
			cursor[0]++;
		}else{
			cursor[0] = 0;
		}
	}
	if(BUTTON_DOWN(J_LEFT)){
		if(cursor[0] > 0){
			cursor[0]--;
		}else{
			cursor[0] = board_size-1;
		}
	}
	if(BUTTON_DOWN(J_UP)){
		if(cursor[1] > 0){
			cursor[1]--;
		}else{
			cursor[1] = board_size-1;
		}
	}
	if(BUTTON_DOWN(J_DOWN)){
		if(cursor[1] < board_size-1){
			cursor[1]++;
		}else{
			cursor[1] = 0;
		}
	}
	if(BUTTON_DOWN(J_A)){ //toggle shaded
		coord = cursor[0]*board_size + cursor[1];
		if(!marks[coord]){
			marks[coord] = BLACK;
			set_bkg_tile_xy(cursor[0]+OFFSET_X,cursor[1]+OFFSET_Y, 0);
		}else if(marks[coord] == BLACK){
			marks[coord] = EMPTY;
			set_bkg_tile_xy(cursor[0]+OFFSET_X,cursor[1]+OFFSET_Y, board[coord]); 
		}
	}
	if(BUTTON_DOWN(J_B)){ //toggle circled
		coord = cursor[0]*board_size + cursor[1];
		if(!marks[coord]){
			marks[coord] = CIRCLE;
			set_bkg_tile_xy(cursor[0]+OFFSET_X,cursor[1]+OFFSET_Y, board[coord] + 10);
		}else if(marks[coord] == CIRCLE){
			marks[coord] = EMPTY;
			set_bkg_tile_xy(cursor[0]+OFFSET_X,cursor[1]+OFFSET_Y, board[coord]);
		}
	}
	if(BUTTON_DOWN(J_SELECT) && BUTTON_DOWN(J_START)){ //display solution, for debugging
		if(!solution_up){			
			for(uint8_t i = 0; i < board_size; i++){
				for(uint8_t j = 0; j < board_size; j++){
					coord = i*board_size + j;
					//set_bkg_tile_xy(i+OFFSET_X,j+OFFSET_Y,board[coord]);
					marks[coord]=solution[coord];
					if(solution[coord] == BLACK){set_bkg_tile_xy(i+OFFSET_X,j+OFFSET_Y,0);}
					if(solution[coord] == CIRCLE){set_bkg_tile_xy(i+OFFSET_X,j+OFFSET_Y,board[coord] + 10);}
				}
			}
			solution_up = true;
		}else{
			for(uint8_t i = 0; i < board_size; i++){
				for(uint8_t j = 0; j < board_size; j++){
					coord = i*board_size + j;
					marks[coord] = EMPTY;
					set_bkg_tile_xy(i+OFFSET_X,j+OFFSET_Y, board[coord]);
				}
			}
			solution_up = false;
		}
	}
	else if(BUTTON_DOWN(J_SELECT)){
		bool is_correct = true;
		for(uint8_t i = 0; i < num_tiles; i++){
			if(CHECK(solution[i]) != CHECK(marks[i])){
				is_correct = false;
				break;
			}
		}
		set_bkg_tile_xy(0,0,is_correct);
		
	}
	move_sprite(0, (cursor[0]+OFFSET_X+1)*8,(cursor[1]+OFFSET_Y+2)*8);
	if(marks[cursor[0]*board_size + cursor[1]] == BLACK){
		set_sprite_tile(0,1);
	}else{
		set_sprite_tile(0,0);
	}
	pressed = input;	
}