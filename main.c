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
#include "queue.h"

#define SIZE_INDICATOR_X 9
#define SIZE_INDICATOR_Y 9

uint8_t shade_count;
uint8_t board_size = 5;

uint8_t cursor[2] = {0,0};
uint8_t *board; 
uint8_t *marks;
bool *shademap;
uint8_t whitespace_count;

void process_input();
bool generate_board();
bool check_placement(uint8_t xi, uint8_t yi);
uint8_t fill(uint8_t xi, uint8_t yi);
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
	
	board = (uint8_t *)malloc( board_size*board_size*sizeof(uint8_t)); //dynamically allocate all arrays based on board size
	marks = (uint8_t *)malloc( board_size*board_size*sizeof(uint8_t)); //this syntax was invented by a psychopath
	shademap = (bool *)malloc( board_size*board_size*sizeof(bool));
	shade_count = (board_size * board_size)/4;
	
	set_bkg_data(0,22,board_tiles);
	set_bkg_tiles(0,0,20,18,board_layout);
	
	set_sprite_data(0,2,cursor_tile); //load cursor sprite
	set_sprite_tile(0,0);
	move_sprite(0,64,64);
	SHOW_SPRITES;
	
	initarand(clock());
	
	do{}while(!generate_board()); //generate boards until a legal one is made
	for(int i = 0; i < board_size; i++){
		for(int j = 0; j < board_size; j++){
			*(marks + i*board_size + j) = 0;
			set_bkg_tile_xy(i+7,j+6,*(board + i*board_size + j)); //write generated board to screen
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

bool generate_board(){
    bool *col_used = (bool *)malloc( board_size* sizeof(bool)); //these track what numbers have been used in each row/column to prevent repeats
    bool *row_used = (bool *)malloc( board_size*board_size*sizeof(bool));
    whitespace_count = board_size*board_size;

    for(uint8_t a = 0; a < board_size; a++){ //reset arrays here in case the function runs again
        for(uint8_t b = 0; b < board_size; b++){
            *(shademap + a*board_size + b) = false;
            *(board + a*board_size + b)  = 0;
            *(row_used + a*board_size + b)  = false;
        }
    }

    uint8_t x,y,i,total,tries = 0;
    for(i = 0; i < shade_count; i++){//pick numbers to shade
        while(true){
            while(true) {
                x = (uint8_t) rand() % board_size;
                y = (uint8_t) rand() % board_size;
                if((*(shademap + x * board_size + y) == false) &&
                   (y < 1 || !(*(shademap + x*board_size + y-1))) && //check top
                   (y + 1 >= board_size || !(*(shademap + x*board_size + y + 1))) && //check bottom
                   (x < 1 || !(*(shademap + (x-1)*board_size + y))) && //check left
                   (x + 1 >=board_size || !(*(shademap + (x+1)*board_size + y)))){ // check right
                    break;
                }

            }
            if(check_placement(x,y)){
                whitespace_count--;
                break;
            }else {
                *(shademap + x * board_size + y) = false;
            }
        }
        *(shademap + x*board_size + y)  = true;
    }
    

    /*if(!check_contiguity()){ //make sure all unshaded tiles form one contiguous group
        if(check_contiguity_old()){printf("check broke\n");}
        printf("failed contig\n");
        return false;

    }*/

    for(x = 0; x < board_size; x++){//fill in values not shaded, with no repeats
        for(uint8_t a = 0; a < board_size; a++){ //reset after each column
            *(col_used + a) = false;
        }
        i = 0;
        total = shade_count;
        for(y = 0; y < board_size; y++){

            if(*(shademap + x*board_size + y)){continue;} //skip shaded squares
            do{
                i = ((uint8_t)rand() % board_size);
                if(*(row_used + y*board_size + i) && !(*(col_used+i))){total--;}
                if(total < 1){return false;}

            }while(*(col_used + i) || *(row_used + y*board_size + i));

            *(col_used + i) = true;
            *(row_used + y*board_size + i) = true;
            *(board + x*board_size + y) = i+1;
            total = shade_count;
        }
    }
    free(row_used);
    free(col_used);
    uint8_t dupe = 0;
    for(x = 0; x < board_size; x++){ //fill the shaded squares with duplicate numbers
        for(y = 0; y < board_size; y++){
            if(*(shademap + x*board_size + y)){
                do{
                    i = (uint8_t)rand() % board_size;
                    if(i & 0x01){
                        dupe = *(board + x*board_size + i);
                    }else{
                        dupe = *(board + i*board_size + y);
                    }
                }while(!dupe);
                *(board + x*board_size + y) = dupe;
            }
        }
    }
    return true;
}


bool check_placement(uint8_t xi, uint8_t yi){
    uint8_t x_low = 1, y_low = 1, x_high = 1, y_high = 1;
    if(xi == 0){
        x_low = 0;
    }
    if(yi == 0){
        y_low = 0;
    }
    if(xi == board_size){
        x_high = 0;
    }
    if(yi == board_size){
        y_high = 0;
    }
    //if(*(shademap + (xi-x_low)*board_size + yi)){return false;}
    //if(*(shademap + xi*board_size + (yi-y_low))){return false;}
    //if(*(shademap + (xi+x_high)*board_size + yi)){return false;}
    //if(*(shademap + xi*board_size + (yi+y_high))){return false;}

    *(shademap + xi*board_size + yi) = true;
    //if(!*(shademap + (xi-x_low)*board_size + yi-y_low) && fill(xi-x_low,yi-y_low) < whitespace_count) {return false;}
    //if(!*(shademap + (xi-x_low)*board_size + yi+y_high) && fill(xi-x_low,yi+y_high) < whitespace_count) {return false;}
    //if(!*(shademap + (xi+x_high)*board_size + yi-y_low) && fill(xi+x_high,yi-y_low) < whitespace_count) {return false;}
    //if(!*(shademap + (xi+x_high)*board_size + yi+y_high) && fill(xi+x_high,yi+y_high) < whitespace_count) {return false;}

    if(!*(shademap + (xi-x_low)*board_size + yi) && fill(xi-x_low,yi) < whitespace_count) {return false;}
    if(!*(shademap + xi*board_size + yi+y_high) && fill(xi,yi+y_high) < whitespace_count) {return false;}
    if(!*(shademap + (xi+x_high)*board_size + yi) && fill(xi+x_high,yi) < whitespace_count) {return false;}
    if(!*(shademap + xi*board_size + yi+y_low) && fill(xi,yi+y_low) < whitespace_count) {return false;}
    return true;
}

uint8_t fill(uint8_t xi, uint8_t yi){ //floodfill using two queues
    if(*(shademap + xi*board_size + yi) == true){return 0;}
    bool *reachable = (bool *)calloc(board_size,board_size*sizeof(bool));
    node_t *Qx = NULL;			   //this version of SDCC doesn't support passing/returning structs
    node_t *Qy = NULL;			   //and this seemed easier than a queue of arrays
    uint8_t count = 1;
    enqueue(&Qx, xi);
    enqueue(&Qy, yi);
    uint8_t nx;
    uint8_t ny;

    while(Qx!=NULL && Qy!=NULL){
        nx = dequeue(&Qx);
        ny = dequeue(&Qy);
        if(*(reachable + nx*board_size + ny)==false && *(shademap + nx*board_size + ny) == false){
            *(reachable + nx*board_size + ny) = true;
            count++;
            if(nx > 0){
                enqueue(&Qx, nx-1);
                enqueue(&Qy, ny);
            }
            if(ny > 0){
                enqueue(&Qx, nx);
                enqueue(&Qy, ny-1);
            }
            if(nx < board_size-1){
                enqueue(&Qx, nx+1);
                enqueue(&Qy, ny);
            }
            if(ny < board_size-1){
                enqueue(&Qx, nx);
                enqueue(&Qy, ny+1);
            }
        }
    }
    free(Qx);
    free(Qy);
    free(reachable);
    //printf("%d\n",count);
    return count;
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
		if(!*(marks + cursor[0]*board_size + cursor[1])){
			*(marks + cursor[0]*board_size + cursor[1]) = 1;
			set_bkg_tile_xy(cursor[0]+7,cursor[1]+6, 0);
		}else if(*(marks + cursor[0]*board_size + cursor[1]) == 1){
			*(marks + cursor[0]*board_size + cursor[1]) = 0;
			set_bkg_tile_xy(cursor[0]+7,cursor[1]+6, *(board + cursor[0]*board_size + cursor[1])); //typing this line made me physically ill
		}
	}
	if(input&J_B && !(pressed&J_B)){ //toggle circled
		if(!*(marks + cursor[0]*board_size + cursor[1])){
			*(marks + cursor[0]*board_size + cursor[1]) = 2;
			set_bkg_tile_xy(cursor[0]+7,cursor[1]+6, *(board + cursor[0]*board_size + cursor[1]) + 10);
		}else if(*(marks + cursor[0]*board_size + cursor[1]) == 2){
			*(marks + cursor[0]*board_size + cursor[1]) = 0;
			set_bkg_tile_xy(cursor[0]+7,cursor[1]+6, *(board + cursor[0]*board_size + cursor[1]));
		}
	}
	if(input&J_SELECT && !(pressed&J_SELECT)){ //display solution, for debugging
		for(int i = 0; i < board_size; i++){
			for(int j = 0; j < board_size; j++){
				set_bkg_tile_xy(i+7,j+6,*(board + i*board_size + j));
				*(marks + i*board_size + j)=*(shademap + i*board_size + j);
				if(*(shademap + i*board_size + j)){set_bkg_tile_xy(i+7,j+6,0);}
			}
		}
	}
	move_sprite(0, (cursor[0]+8)*8,(cursor[1]+8)*8);
	if(*(marks + cursor[0]*board_size + cursor[1]) == 1){
		set_sprite_tile(0,1);
	}else{
		set_sprite_tile(0,0);
	}
	pressed = input;	
}