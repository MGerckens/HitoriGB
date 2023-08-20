#include <gb/gb.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <rand.h>

#include "tilemaps.h"
#include "generator.h"
#include "queues.h"

//offsets added to tile positions to keep board centered on screen
#define OFFSET_X (10-(board_size/2))
#define OFFSET_Y (9-(board_size/2))

//ID of the white tile
#define WHITE 41 

uint8_t board_size = 5;
uint8_t num_tiles;

enum {EMPTY, BLACK, CIRCLE};
uint8_t cursor[2] = {0,0};
uint8_t *board, *marks, *solution;

bool process_input();
void move(uint8_t input);
void title_input();
bool check_solution();
uint8_t fill(uint8_t xi, uint8_t yi);

void main(void)
{
    DISPLAY_ON;
	SPRITES_8x8;
restart:
	HIDE_WIN;
	set_bkg_data(0,59,title_tiles); //load title screen
	set_bkg_tiles(0,0,20,18,title_layout);
	move_bkg(0,0); //reset bkg location from if it was offset before
	SHOW_BKG;
	do{}while(joypad() & J_START); //wait until start is released
	do{title_input();}while(!(joypad() & J_START));//wait on title screen
	HIDE_BKG;
	num_tiles = board_size*board_size; //used in a lot of loops
	
	board = (uint8_t *)malloc( num_tiles * sizeof(uint8_t)); //dynamically allocate all arrays based on board size
	marks = (uint8_t *)calloc( num_tiles, sizeof(uint8_t)); 
	solution = (uint8_t *)malloc( num_tiles * sizeof(uint8_t));
	
	set_bkg_data(0,42,board_tiles);
	set_bkg_tiles(0,0,21,19,board_layout);
	SHOW_BKG;
	set_bkg_tile_xy(1,1,33); //loading icon
	set_bkg_tile_xy(2,1,31);
	set_bkg_tile_xy(3,1,32);
	initarand(clock());

	generate_board();
	
	set_bkg_tile_xy(1,1,WHITE); //remove loading icon
	set_bkg_tile_xy(2,1,WHITE);
	set_bkg_tile_xy(3,1,WHITE);
	
	set_sprite_data(0,2,cursor_tiles); //load cursor sprite
	set_sprite_tile(0,0);

	set_win_tiles(0,0,21,1,board_layout);
	
	
	if(board_size&0x01){move_bkg(4,4);} //offset bkg layer if board_size is odd, to center the board on the screen
	move_win(7,136); //window coords are relative to bkg so use the offset but keep y the same, also x is offset by -7 for some reason
	move_sprite(0,(OFFSET_X+1)*8,(OFFSET_Y+2)*8);
	for(int i = 0; i < num_tiles; i++){
		set_bkg_tile_xy((i%board_size)+OFFSET_X,(i/board_size)+OFFSET_Y,board[i]); //write generated board to screen
	}
	SHOW_WIN;
	SHOW_SPRITES;
	
	while(1) { //main game loop
		if(!process_input()){ //If start+select, restart game
			cursor[0] = 0;
			cursor[1] = 0;
			free(board);
			free(marks);
			free(solution);
			
			HIDE_SPRITES; //avoid tile flickering during restart
			HIDE_BKG;
			goto restart;
		}
        vsync();
    }
}

#define BUTTON_DOWN(x) ((input & (x)) && !(last_input & (x)))
void title_input(){
	uint8_t input = joypad();
	static uint8_t last_input = 0;
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
	set_bkg_tile_xy(9,9,board_size + 10); //update display, tile IDs 15-25 are graphics for the numbers 5-15
	last_input = input;
}

bool process_input(){ //returns false if start+select is pressed, true otherwise
	uint8_t input = joypad();
	uint8_t coord, x, y;
	bool retval = true;
	static uint8_t last_input = 0;
	static bool solution_up = false;
	static uint8_t last_check = 0;
	uint8_t current_time = (uint8_t)time(NULL); //time in seconds

	if(current_time >= last_check+2){
		move_win(7,136); //recenter window layer
		set_win_tile_xy(8,0,WHITE);
		set_win_tile_xy(9,0,WHITE);
		set_win_tile_xy(10,0,WHITE);
		set_win_tile_xy(11,0,WHITE);
		last_check = 0;
	}

	static uint8_t time_held = 0; //time the current direction on the d-pad was pressed
	if(last_input == input){
		time_held++;
		if((time_held > 20) && !(clock() % 4)){ //if held for less than 0.5s, skip input
			//if longer than 0.5s, move every 3rd frame 
			move(input);
		}
	}else{
		time_held = 0;
		move(input &~ last_input); //only use new button, stops weird behavior when switching from straight to diagonal
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
		retval = false;
	}else if(BUTTON_DOWN(J_SELECT)){ //displays if solution is correct 
		if(check_solution()){
			move_win(7,136);
			set_win_tile_xy(8,0,34);
			set_win_tile_xy(9,0,35);
			set_win_tile_xy(10,0,36);
			set_win_tile_xy(11,0,37);
		}else{
			move_win(12,136);
			set_win_tile_xy(8,0,38);
			set_win_tile_xy(9,0,39);
			set_win_tile_xy(10,0,40);
			set_win_tile_xy(11,0,WHITE); //overwrite a previous correct, which would have written here
		}
		last_check = current_time;
	}else if(BUTTON_DOWN(J_START)){ //display correct solution for debugging
		if(!solution_up){			
			for(x = 0; x < board_size; x++){
				for(y = 0; y < board_size; y++){
					coord = y*board_size + x;
					marks[coord]=solution[coord];
					if(solution[coord] == BLACK){set_bkg_tile_xy(x+OFFSET_X,y+OFFSET_Y,0);}
					if(solution[coord] == CIRCLE){set_bkg_tile_xy(x+OFFSET_X,y+OFFSET_Y,board[coord] + 15);}
				}
			}
			solution_up = true;
		}else{
			for(x = 0; x < board_size; x++){
				for(y = 0; y < board_size; y++){
					coord = y*board_size + x;
					marks[coord] = EMPTY;
					set_bkg_tile_xy(x+OFFSET_X,y+OFFSET_Y, board[coord]);
				}
			}
			solution_up = false;
		}
	}
	
	move_sprite(0, (cursor[0]+OFFSET_X+1)*8,(cursor[1]+OFFSET_Y+2)*8); //move cursor sprite to correct location
	if(board_size&0x01){scroll_sprite(0,-4,-4);} //offset cursor if board_size is odd
	if(marks[cursor[1]*board_size + cursor[0]] == BLACK){ //change cursor color when on a black square to keep it visible
		set_sprite_tile(0,1);
	}else{
		set_sprite_tile(0,0);
	}
	last_input = input;	
	return retval;
}


inline void move(uint8_t input){
	if(input & J_RIGHT){ 
		if(cursor[0] < board_size-1){
			cursor[0]++;
		}else{
			cursor[0] = 0;
		}
	}
	if(input & J_LEFT){
		if(cursor[0] > 0){
			cursor[0]--;
		}else{
			cursor[0] = board_size-1;
		}
	}
	if(input & J_UP){
		if(cursor[1] > 0){
			cursor[1]--;
		}else{
			cursor[1] = board_size-1;
		}
	}
	if(input & J_DOWN){
		if(cursor[1] < board_size-1){
			cursor[1]++;
		}else{
			cursor[1] = 0;
		}
	}

}

#define CHECK(x) ((x==1) ? 1 : 0)
bool check_solution(){
	bool is_correct = true;
	uint8_t x,y;
	for(x = 0; x < num_tiles; x++){ //check if player marks match solution
		if(CHECK(solution[x]) != CHECK(marks[x])){
			is_correct = false;
			break;
		}
	}
	//return is_correct;
	if(is_correct){//if perfect match, no need to check further
		return true;
	}else{
		
		/* the rest is in case of a rare board with multiple solutions, ex:
		* 6,3,7,3,2,1,4,
		* 3,1,5,4,7,5,3,
		* 6,6,2,1,3,3,5,
		* 5,4,4,7,3,5,6,
		* 3,7,4,3,6,6,7,
		* 1,5,2,2,6,4,7,
		* 5,2,5,2,1,1,3
		* has two involving the 6s in top left */
		
		uint8_t *row_count = (uint8_t *)calloc(num_tiles, sizeof(uint8_t));
		uint8_t *col_count = (uint8_t *)calloc(num_tiles, sizeof(uint8_t));	
		uint8_t black_count = 0;
		for(x = 0; x < board_size; x++){ //store count of each number in rows and columns
			for(y = 0; y < board_size; y++){
				if(marks[y*board_size+x] != BLACK){
					row_count[y*board_size + board[y*board_size+x]-1]++;
					col_count[x*board_size + board[y*board_size+x]-1]++;
				}else{black_count++;} //also count black squares for next step
			}
		}
		
		for(x = 0; x < board_size; x++){ //if any row or column has a duplicate, solution is false
			for(y = 0; y < board_size; y++){
				if(col_count[y*board_size+x] > 1){
					free(row_count);
					free(col_count);
					return false;
				}
				if(row_count[y*board_size+x] > 1){
					free(row_count);
					free(col_count);
					return false;
				}
			}
		}
		
		if(marks[0] != BLACK){ //use flood fill to check for contiguity
			if(fill(0,0) < (num_tiles - black_count)){
				free(row_count);
				free(col_count);
				return false;
			}
		}else{
			if(fill(0,1) < (num_tiles - black_count)){
				free(row_count);
				free(col_count);
				return false;
			}
		}
		free(row_count);
		free(col_count);
		return true;		
	}
}
uint8_t fill(uint8_t xi, uint8_t yi){ //floodfill using two queues
	
    bool *reachable = (bool *)calloc(num_tiles, sizeof(bool));
    node_t *Qx = NULL;
    node_t *Qy = NULL;
    enqueue(&Qx, xi);
    enqueue(&Qy, yi);
    uint8_t nx, ny;

    while(Qx!=NULL && Qy!=NULL){
        nx = dequeue(&Qx);
        ny = dequeue(&Qy);
		if(!(nx < board_size && ny < board_size)){continue;}
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
    return count;
}