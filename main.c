// Max Gerckens 10/2021

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
//#include "queues.h"

//offsets added to tile positions to keep board centered on screen
#define OFFSET_X (10-(board_size/2))
#define OFFSET_Y (9-(board_size/2))

uint8_t board_size = 5;
uint8_t num_tiles;

enum {EMPTY, BLACK, CIRCLE};
const uint8_t WHITE = 41;
uint8_t cursor[2] = {0,0};
uint8_t *board; 
uint8_t *marks;
uint8_t *solution;
bool restarting = false;

bool process_input();
void title_input();
//bool check_solution();
//uint8_t fill(uint8_t xi, uint8_t yi);

void main(void)
{
    DISPLAY_ON;
	SPRITES_8x8;
restart:
	set_bkg_data(0,59,title_tiles); //load title screen
	set_bkg_tiles(0,0,20,18,title_layout);
	SHOW_BKG;
	do{}while(joypad() & J_START); //wait until start is released
	do{title_input();}while(!(joypad() & J_START));//wait on title screen
	HIDE_BKG;
	num_tiles = board_size*board_size; //used in a lot of loops
	
	
	board = (uint8_t *)calloc( board_size, board_size*sizeof(uint8_t)); //dynamically allocate all arrays based on board size
	marks = (uint8_t *)malloc( num_tiles*sizeof(uint8_t)); 
	solution = (uint8_t *)calloc( num_tiles,sizeof(uint8_t));
	
	set_bkg_data(0,42,board_tiles);
	set_bkg_tiles(0,0,20,18,board_layout);
	SHOW_BKG;
	set_bkg_tile_xy(0,0,33); //loading indicator
	set_bkg_tile_xy(1,0,31);
	set_bkg_tile_xy(2,0,32);
	initarand(clock());

	generate_board();
	
	set_bkg_tile_xy(0,0,WHITE);
	set_bkg_tile_xy(1,0,WHITE);
	set_bkg_tile_xy(2,0,WHITE);
	set_sprite_data(0,2,cursor_tile); //load cursor sprite
	set_sprite_tile(0,0);
	SHOW_SPRITES;
	
	for(int i = 0; i < num_tiles; i++){
		marks[i] = 0; 
		set_bkg_tile_xy((i%board_size)+OFFSET_X,(i/board_size)+OFFSET_Y,board[i]); //write generated board to screen
	}
	
	while(1) { //main game loop
		if(!process_input()){ //If start+select, restart game
			cursor[0] = 0;
			cursor[1] = 0;
			free(board);
			free(marks);
			free(solution);
			//board_size = 5;
			HIDE_SPRITES; //avoid tile flickering during restart
			HIDE_BKG;
			goto restart;
		}
        wait_vbl_done();
    }
}

#define TILE_NUM(x) (((x) <= 10) ? ((x) + 10) : ((x) + 43))
#define BUTTON_DOWN(x) ((input & (x)) && !(pressed & (x)))
void title_input(){
	uint8_t input = joypad();
	static uint8_t pressed = 0;
	if(BUTTON_DOWN(J_LEFT)){
		if(board_size > 5){ //update board size
			board_size--;
		}else{
			board_size = 15;
		}
	}
	if(BUTTON_DOWN(J_RIGHT)){
		if(board_size < 15){
			board_size++;
		}else{
			board_size = 5;
		}
	}
	set_bkg_tile_xy(9,9,TILE_NUM(board_size)); //update display
	pressed = input;
}


#define CHECK(x) ((x==1) ? 1 : 0)
bool process_input(){ //returns false if start+select is pressed, true otherwise
	uint8_t input = joypad();
	uint8_t coord;
	bool retval = true;
	static uint8_t pressed = 0;
	static bool solution_up = false;
	//static bool check_up = false;
	static uint8_t last_check = 0;
	if((uint8_t)time(NULL) >= last_check+2){
		set_bkg_tile_xy(8,17,WHITE);
		set_bkg_tile_xy(9,17,WHITE);
		set_bkg_tile_xy(10,17,WHITE);
		set_bkg_tile_xy(11,17,WHITE);
		//check_up = false;
		last_check = 0;
	}
	if(BUTTON_DOWN(J_RIGHT)){ //move in the specified direction, with bounds checking and wraparound
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
		coord = cursor[1]*board_size + cursor[0];
		if(!marks[coord]){
			marks[coord] = BLACK;
			set_bkg_tile_xy(cursor[0]+OFFSET_X,cursor[1]+OFFSET_Y, 0);
		}else if(marks[coord] == BLACK){
			marks[coord] = EMPTY;
			set_bkg_tile_xy(cursor[0]+OFFSET_X,cursor[1]+OFFSET_Y, board[coord]); 
		}
	}
	if(BUTTON_DOWN(J_B)){ //toggle circled
		coord = cursor[1]*board_size + cursor[0];
		if(!marks[coord]){
			marks[coord] = CIRCLE;
			set_bkg_tile_xy(cursor[0]+OFFSET_X,cursor[1]+OFFSET_Y, board[coord] + 15);
		}else if(marks[coord] == CIRCLE){
			marks[coord] = EMPTY;
			set_bkg_tile_xy(cursor[0]+OFFSET_X,cursor[1]+OFFSET_Y, board[coord]);
		}
	}
	if(BUTTON_DOWN(J_SELECT) && BUTTON_DOWN(J_START)){ //start+select to return to title
		restarting = true;
		retval = false;
	}else if(BUTTON_DOWN(J_SELECT)){ //checks solution, displays 1 if correct (will polish)
		bool is_correct = true;
		for(uint8_t i = 0; i < num_tiles; i++){
			if(CHECK(solution[i]) != CHECK(marks[i])){
				is_correct = false;
				break;
			}
		}
		if(is_correct){
			set_bkg_tile_xy(8,17,34);
			set_bkg_tile_xy(9,17,35);
			set_bkg_tile_xy(10,17,36);
			set_bkg_tile_xy(11,17,37);
		}else{
			set_bkg_tile_xy(8,17,38);
			set_bkg_tile_xy(9,17,39);
			set_bkg_tile_xy(10,17,40);
			set_bkg_tile_xy(11,17,WHITE);
		}
		last_check = time(NULL);
	}else if(BUTTON_DOWN(J_START)){ //display correct solution for debugging
		if(!solution_up){			
			for(uint8_t i = 0; i < board_size; i++){
				for(uint8_t j = 0; j < board_size; j++){
					coord = j*board_size + i;
					//set_bkg_tile_xy(i+OFFSET_X,j+OFFSET_Y,board[coord]);
					marks[coord]=solution[coord];
					if(solution[coord] == BLACK){set_bkg_tile_xy(i+OFFSET_X,j+OFFSET_Y,0);}
					if(solution[coord] == CIRCLE){set_bkg_tile_xy(i+OFFSET_X,j+OFFSET_Y,board[coord] + 15);}
				}
			}
			solution_up = true;
		}else{
			for(uint8_t i = 0; i < board_size; i++){
				for(uint8_t j = 0; j < board_size; j++){
					coord = j*board_size + i;
					marks[coord] = EMPTY;
					set_bkg_tile_xy(i+OFFSET_X,j+OFFSET_Y, board[coord]);
				}
			}
			solution_up = false;
		}
	}
	
	move_sprite(0, (cursor[0]+OFFSET_X+1)*8,(cursor[1]+OFFSET_Y+2)*8); //move cursor sprite to correct location
	if(marks[cursor[1]*board_size + cursor[0]] == BLACK){ //change cursor color when on a black square to keep it visible
		set_sprite_tile(0,1);
	}else{
		set_sprite_tile(0,0);
	}
	pressed = input;	
	return retval;
}
/*
bool check_solution(){
	bool is_correct = true;
	uint8_t i,j;
	for(i = 0; i < num_tiles; i++){
		if(CHECK(solution[i]) != CHECK(marks[i])){
			is_correct = false;
			break;
		}
	}
	if(is_correct){return true;}
	
	uint8_t *row_count = (uint8_t *)calloc(num_tiles,sizeof(uint8_t));
	uint8_t *col_count = (uint8_t *)calloc(board_size,sizeof(uint8_t));	
	uint8_t black_count = 0;
	for(i = 0; i < board_size; i++){
		for(j = 0; j < board_size; j++){
			if(marks[j*board_size+i] != BLACK){
				row_count[j*board_size + board[j*board_size+i]]++;
				col_count[board[j*board_size+i]]++;
			}else{black_count++;}
		}
	}
	set_bkg_tile_xy(0,0,0);
	for(i = 0; i < board_size; i++){
		if(col_count[i] > 1){return false;}
		for(j = 0; j < board_size; j++){
			if(row_count[j*board_size+i] > 1){return false;}
		}
	}
	
	if(marks[0] != BLACK){
		if(fill(0,0) < (num_tiles - black_count)){return false;}
	}else{
		if(fill(0,1) < (num_tiles - black_count)){return false;}
	}
	return true;
}

#define INGRID(x,y) ((x) < board_size && (y) < board_size)
uint8_t fill(uint8_t xi, uint8_t yi){ //floodfill using two queues
	set_bkg_tile_xy(0,8,1);
    //printf("started fill\n");
    bool *reachable = (bool *)calloc(num_tiles,sizeof(bool));
    node_t *Qx = NULL;			   //this version of SDCC doesn't support passing/returning structs
    node_t *Qy = NULL;			   //and this seemed easier than a queue of arrays
    enqueue(&Qx, xi);
    enqueue(&Qy, yi);
    uint8_t nx, ny;

    while(Qx!=NULL && Qy!=NULL){
        nx = dequeue(&Qx);
        ny = dequeue(&Qy);
		if(!INGRID(nx,ny)){continue;}
        if(reachable[ny*board_size + nx] == false && marks[ny*board_size + nx] != BLACK){
            reachable[ny*board_size + nx] = true;
			enqueue(&Qx, nx-1);
			enqueue(&Qy, ny);

			enqueue(&Qx, nx);
			enqueue(&Qy, ny-1);

			enqueue(&Qx, nx+1);
			enqueue(&Qy, ny);

			enqueue(&Qx, nx);
			enqueue(&Qy, ny+1);
        }
    }
    free(Qx);
    free(Qy);
	uint8_t count = 0;
	for(uint8_t i = 0; i < num_tiles; i++){ //tried putting this counter in the fill alg itself, but it doublecounts a lot
		if(reachable[i]){			//this isn't that much slower anyway
			count++;
		}
	}
    free(reachable);
    //printf("%d\n",count);
	set_bkg_tile_xy(0,8,0);
    return count;
}*/