// Max Gerckens 10/5/2021
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
#include "queues.h"
#include "latinsquare.h"
#include "generator.h"

#define SIZE_INDICATOR_X 9
#define SIZE_INDICATOR_Y 9

uint8_t board_size = 5;

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
	set_bkg_data(0,27,title_tiles); //load title screen
	set_bkg_tiles(0,0,20,18,title_layout);
	SHOW_BKG;
	while(!(joypad()&J_START)){
		title_input();
	}; //wait on title screen
	
	board = (uint8_t *)calloc( board_size, board_size*sizeof(uint8_t)); //dynamically allocate all arrays based on board size
	marks = (uint8_t *)malloc( board_size*board_size*sizeof(uint8_t)); //this syntax was invented by a psychopath
	solution = (uint8_t *)malloc( board_size*board_size*sizeof(uint8_t));
	
	set_bkg_data(0,22,board_tiles);
	set_bkg_tiles(0,0,20,18,board_layout);
	
	
	initarand(clock());
	generate_board();
	
	set_sprite_data(0,2,cursor_tile); //load cursor sprite
	set_sprite_tile(0,0);
	move_sprite(0,64,64);
	SHOW_SPRITES;
	
	for(int i = 0; i < board_size; i++){
		for(int j = 0; j < board_size; j++){
			marks[i*board_size + j] = 0;
			set_bkg_tile_xy(i+7,j+6,board[i*board_size + j]); //write generated board to screen
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
	set_bkg_tile_xy(SIZE_INDICATOR_X,SIZE_INDICATOR_Y,board_size+10);
	pressed = input;
}

void process_input(){
	uint8_t input = joypad();
	static uint8_t pressed = 0;
	if(input&J_RIGHT && !(pressed&J_RIGHT)){ //move in the specified direction, with bounds checking
		if(cursor[0] < board_size-1){
			cursor[0]++;
		}else{
			cursor[0] = 0;
		}
	}
	if(input&J_LEFT && !(pressed&J_LEFT)){
		if(cursor[0] > 0){
			cursor[0]--;
		}else{
			cursor[0] = board_size-1;
		}
	}
	if(input&J_UP && !(pressed&J_UP)){
		if(cursor[1] > 0){
			cursor[1]--;
		}else{
			cursor[1] = board_size-1;
		}
	}
	if(input&J_DOWN && !(pressed&J_DOWN)){
		if(cursor[1] < board_size-1){
			cursor[1]++;
		}else{
			cursor[1] = 0;
		}
	}
	if(input&J_A && !(pressed&J_A)){ //toggle shaded
		if(!marks[cursor[0]*board_size + cursor[1]]){
			marks[cursor[0]*board_size + cursor[1]] = 1;
			set_bkg_tile_xy(cursor[0]+7,cursor[1]+6, 0);
		}else if(marks[cursor[0]*board_size + cursor[1]] == 1){
			marks[cursor[0]*board_size + cursor[1]] = 0;
			set_bkg_tile_xy(cursor[0]+7,cursor[1]+6, *(board + cursor[0]*board_size + cursor[1])); //typing this line made me physically ill
		}
	}
	if(input&J_B && !(pressed&J_B)){ //toggle circled
		if(!marks[cursor[0]*board_size + cursor[1]]){
			marks[cursor[0]*board_size + cursor[1]] = 2;
			set_bkg_tile_xy(cursor[0]+7,cursor[1]+6, *(board + cursor[0]*board_size + cursor[1]) + 10);
		}else if(marks[cursor[0]*board_size + cursor[1]] == 2){
			marks[cursor[0]*board_size + cursor[1]] = 0;
			set_bkg_tile_xy(cursor[0]+7,cursor[1]+6, *(board + cursor[0]*board_size + cursor[1]));
		}
	}
	if(input&J_SELECT && !(pressed&J_SELECT)){ //display solution, for debugging
		for(int i = 0; i < board_size; i++){
			for(int j = 0; j < board_size; j++){
				set_bkg_tile_xy(i+7,j+6,board[i*board_size + j]);
				marks[i*board_size + j]=solution[i*board_size + j];
				if(solution[i*board_size + j] == BLACK){set_bkg_tile_xy(i+7,j+6,0);}
				if(solution[i*board_size + j] == CIRCLE){set_bkg_tile_xy(i+7,j+6,board[i*board_size + j] + 10);}
			}
		}
	}
	move_sprite(0, (cursor[0]+8)*8,(cursor[1]+8)*8);
	if(marks[cursor[0]*board_size + cursor[1]] == 1){
		set_sprite_tile(0,1);
	}else{
		set_sprite_tile(0,0);
	}
	pressed = input;	
}