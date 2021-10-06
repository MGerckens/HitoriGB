// Max Gerckens 10/5/2021


#include <gb/gb.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <rand.h>
#include <gb/crash_handler.h>

#include "test_title_tiles.c"
#include "test_title_layout.c"
#include "board_tiles.c"
#include "game_board_layout.c"
#include "cursor_tile.c"
#include "queue.h"

#define BOARD_SIZE 6 //TODO make this variable
#define SHADE_COUNT 11

uint8_t cursor[2] = {0,0};
uint8_t board[BOARD_SIZE][BOARD_SIZE]; 
uint8_t marks[BOARD_SIZE][BOARD_SIZE];
uint8_t shademap[BOARD_SIZE][BOARD_SIZE];
bool reachable[BOARD_SIZE][BOARD_SIZE];

void process_input();
bool generate_board();
bool check_contiguity();
void fill(uint8_t xi, uint8_t yi);

void main(void)
{
    DISPLAY_ON;
	SPRITES_8x8;
	set_bkg_data(0,12,test_title_tiles); //load title screen
	set_bkg_tiles(0,0,20,18,test_title_layout);
	SHOW_BKG;
	while(!(joypad()&J_START)){}; //wait on title screen
	set_bkg_data(0,18,board_tiles);
	set_bkg_tiles(0,0,20,18,board_layout);
	
	set_sprite_data(0,1,cursor_tile); //load cursor sprite
	set_sprite_tile(0,0);
	move_sprite(0,64,64);
	SHOW_SPRITES;
	
	initarand(clock());
	
	do{}while(!generate_board()); //generate boards until a legal one is made
	for(int i = 0; i < BOARD_SIZE; i++){
		for(int j = 0; j < BOARD_SIZE; j++){
			marks[i][j] = 0;
			set_bkg_tile_xy(i+7,j+6,board[i][j]); //write generated board to screen
		}
	}
	
	while(1) {
		process_input();
        wait_vbl_done();
    }
}

bool generate_board(){
    bool col_used[BOARD_SIZE]; //these track what numbers have been used in each row/column to prevent repeats
    bool row_used[BOARD_SIZE][BOARD_SIZE];
	
    for(uint8_t a = 0; a < BOARD_SIZE; a++){ //reset arrays here in case the function runs again
        for(uint8_t b = 0; b < BOARD_SIZE; b++){
            shademap[a][b] = 0;
            board[a][b] = 0;
			reachable[a][b] = 0;
			row_used[a][b] = false;
        }
    }
	
    uint8_t x,y,i,total;
    for(uint8_t i = 0; i < SHADE_COUNT; i++){//pick numbers to shade
        while(true){
            x = (uint8_t)rand() % BOARD_SIZE;
            y = (uint8_t)rand() % BOARD_SIZE;
            if((y < 1 || (shademap[x][y-1] != 1)) && //check top
               (y + 1 >= BOARD_SIZE || (shademap[x][y+1] != 1)) && //check bottom
               (x < 1 || (shademap[x-1][y] != 1)) && //check left
               (x + 1 >=BOARD_SIZE || (shademap[x+1][y] != 1))){ // check right
                break;
            }

        }
        shademap[x][y] = 1;
    }

    if(!check_contiguity()){ //make sure all unshaded tiles form one contiguous group
        return false;
    }
	
    for(x = 0; x < BOARD_SIZE; x++){//fill in values not shaded, with no repeats
        for(uint8_t a = 0; a < BOARD_SIZE; a++){ //reset after each column
            col_used[a] = false;
        }
        i = 0;
        total = SHADE_COUNT;
        for(y = 0; y < BOARD_SIZE; y++){

            if(shademap[x][y]){continue;} //skip shaded squares
            do{
                i = ((uint8_t)rand() % BOARD_SIZE);
                if(row_used[y][i] && !col_used[i]){total--;}
                if(total < 1){return false;}

            }while(col_used[i] || row_used[y][i]);

            col_used[i] = true;
            row_used[y][i] = true;
            board[x][y] = i+1;
            total = SHADE_COUNT;

        }

    }

	total = SHADE_COUNT;
	for(x = 0; x < BOARD_SIZE; x++){ //fill the shaded squares with duplicate numbers
		for(uint8_t y = 0; y < BOARD_SIZE; y++){
			if(shademap[x][y]){
				do{
					i = (uint8_t)arand() % BOARD_SIZE;
					if(x > y){
						total = board[x][i];
					}else{
						total = board[i][y];
					}
				}while(!total);
				board[x][y] = total;
			}
		}
	}
	return true;
}


bool check_contiguity(){ // O(n^4), there is definitely a better way to do this
    for(uint8_t i = 0; i < BOARD_SIZE; i++){
        for(uint8_t j = 0; j < BOARD_SIZE; j++){
            fill(i,j);
            for(uint8_t m = 0; m < BOARD_SIZE; m++){
                for(uint8_t n = 0; n < BOARD_SIZE; n++){
                    if(shademap[m][n] != 1 && reachable[m][n] == false){return false;}
                }
            }
        }
    }
    return true;
}

void fill(uint8_t xi, uint8_t yi){ //floodfill using two queues
    node_t *Qx = NULL;			   //this version of SDCC doesn't support passing/returning structs
    node_t *Qy = NULL;			   //and this seemed easier than a queue of arrays
    enqueue(&Qx, xi);
    enqueue(&Qy, yi);
    uint8_t nx;
    uint8_t ny;
    while(Qx!=NULL && Qy!=NULL){
        nx = dequeue(&Qx);
        ny = dequeue(&Qy);
        if(!reachable[nx][ny] && !shademap[nx][ny]){
            reachable[nx][ny] = true;
            if(nx > 0){
                enqueue(&Qx, nx-1);
                enqueue(&Qy, ny);
            }
            if(ny > 0){
                enqueue(&Qx, nx);
                enqueue(&Qy, ny-1);
            }
            if(nx < BOARD_SIZE-1){
                enqueue(&Qx, nx+1);
                enqueue(&Qy, ny);
            }
            if(ny < BOARD_SIZE-1){
                enqueue(&Qx, nx);
                enqueue(&Qy, ny+1);
            }
        }
    }
}


void process_input(){
	uint8_t input = joypad();
	static uint8_t pressed = 0;
	if(input&J_RIGHT && !(pressed&J_RIGHT)){ //move in the specified direction, with bounds checking
		if(cursor[0] < BOARD_SIZE-1){
			cursor[0]++;
		}else{
			cursor[0] = 0;
		}
	}
	if(input&J_LEFT && !(pressed&J_LEFT)){
		if(cursor[0] > 0){
			cursor[0]--;
		}else{
			cursor[0] = BOARD_SIZE-1;
		}
	}
	if(input&J_UP && !(pressed&J_UP)){
		if(cursor[1] > 0){
			cursor[1]--;
		}else{
			cursor[1] = BOARD_SIZE-1;
		}
	}
	if(input&J_DOWN && !(pressed&J_DOWN)){
		if(cursor[1] < BOARD_SIZE-1){
			cursor[1]++;
		}else{
			cursor[1] = 0;
		}
	}
	if(input&J_A && !(pressed&J_A)){ //toggle shaded
		if(!marks[cursor[0]][cursor[1]]){
			marks[cursor[0]][cursor[1]] = 1;
			set_bkg_tile_xy(cursor[0]+7,cursor[1]+6, 0);
		}else if(marks[cursor[0]][cursor[1]] == 1){
			marks[cursor[0]][cursor[1]] = 0;
			set_bkg_tile_xy(cursor[0]+7,cursor[1]+6, board[cursor[0]][cursor[1]]);
		}
	}
	if(input&J_B && !(pressed&J_B)){ //toggle circled
		if(!marks[cursor[0]][cursor[1]]){
			marks[cursor[0]][cursor[1]] = 2;
			set_bkg_tile_xy(cursor[0]+7,cursor[1]+6, board[cursor[0]][cursor[1]] + 8);
		}else if(marks[cursor[0]][cursor[1]] == 2){
			marks[cursor[0]][cursor[1]] = 0;
			set_bkg_tile_xy(cursor[0]+7,cursor[1]+6, board[cursor[0]][cursor[1]]);
		}
	}
	if(input&J_SELECT && !(pressed&J_SELECT)){ //display solution, for debugging
		for(int i = 0; i < BOARD_SIZE; i++){
			for(int j = 0; j < BOARD_SIZE; j++){
				set_bkg_tile_xy(i+7,j+6,board[i][j]);
				marks[i][j]=shademap[i][j];
				if(shademap[i][j]){set_bkg_tile_xy(i+7,j+6,0);}
			}
		}
	}
	
	move_sprite(0, (cursor[0]+8)*8,(cursor[1]+8)*8);	
	pressed = input;	
}

