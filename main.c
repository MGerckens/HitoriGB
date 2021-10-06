// Max Gerckens 2021
// Board generator stolen from GNOME

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

#define BOARD_SIZE 6
#define SHADE_COUNT 11

uint8_t cursor[2] = {0,0};
uint8_t board[BOARD_SIZE][BOARD_SIZE]; /* ={{4,6,5,2,6,5}, 
										{3,4,6,2,5,2}, 
										{2,6,6,4,2,4}, 
										{2,4,1,3,4,6}, 
										{6,3,2,6,1,2}, 
										{1,3,4,6,3,5}};*/
										//https://www.chiark.greenend.org.uk/~sgtatham/puzzles/js/singles.html#6x6de#992652795168036
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
	set_bkg_data(0,12,test_title_tiles);
	set_bkg_tiles(0,0,20,18,test_title_layout);
	SHOW_BKG;
	while(!(joypad()&J_START)){};
	
	set_bkg_data(0,18,board_tiles);
	set_bkg_tiles(0,0,20,18,board_layout);
	set_sprite_data(0,1,cursor_tile);
	set_sprite_tile(0,0);
	move_sprite(0,64,64);
	SHOW_SPRITES;
	initarand(clock());
	bool success;
	do{
		
		
		//initarand(arand());
	}while(!generate_board());
	
	for(int i = 0; i < BOARD_SIZE; i++){
		for(int j = 0; j < BOARD_SIZE; j++){
			marks[i][j] = 0;
			set_bkg_tile_xy(i+7,j+6,board[i][j]);
		}
	}
	while(1) {
		process_input();
        wait_vbl_done();
    }
}
bool blink = false;
bool generate_board(){
	if(!blink){set_bkg_tile_xy(0,0,0);
	}else{ set_bkg_tile_xy(0,0,1);}
	blink = !blink;
    bool col_used[BOARD_SIZE];
    bool row_used[BOARD_SIZE][BOARD_SIZE];
    for(uint8_t a = 0; a < BOARD_SIZE; a++){
        for(uint8_t b = 0; b < BOARD_SIZE; b++){
            shademap[a][b] = 0;
            board[a][b] = 0;
			reachable[a][b] = 0;
        }
    }
    uint8_t x,y,i,total,old_total;
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

    if(!check_contiguity()){
        return false;
    }
    for(x = 0; x < BOARD_SIZE; x++){
        for(y = 0; y < BOARD_SIZE; y++){
            row_used[x][y] = false;
        }
    }
    for(x = 0; x < BOARD_SIZE; x++){//fill in values not shaded, broken
        for(uint8_t a = 0; a < BOARD_SIZE; a++){
            col_used[a] = false;

        }
        i = 0;
        total = SHADE_COUNT;
        old_total = total;
        for(y = 0; y < BOARD_SIZE; y++){

            if(shademap[x][y]){continue;}
            do{
                i = ((uint8_t)rand() % BOARD_SIZE);
                if(row_used[y][i] && !col_used[i]){total--;}
                if(total < 1){return false;}

            }while(col_used[i] || row_used[y][i]);

            col_used[i] = true;
            row_used[y][i] = true;
            board[x][y] = i+1;
            total = old_total - 1;

        }

    }

	total = SHADE_COUNT;
	for(x = 0; x < BOARD_SIZE; x++){
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


bool check_contiguity(){ // this should be a war crime
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

void fill(uint8_t xi, uint8_t yi){
    node_t *Qx = NULL;
    node_t *Qy = NULL;
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
	if(input&J_RIGHT && !(pressed&J_RIGHT)){
		if(cursor[0] < BOARD_SIZE-1){
			cursor[0]++;
		}
	}
	if(input&J_LEFT && !(pressed&J_LEFT)){
		if(cursor[0] > 0){
			cursor[0]--;
		}
	}
	if(input&J_UP && !(pressed&J_UP)){
		if(cursor[1] > 0){
			cursor[1]--;
		}
	}
	if(input&J_DOWN && !(pressed&J_DOWN)){
		if(cursor[1] < BOARD_SIZE-1){
			cursor[1]++;
		}
	}
	if(input&J_A && !(pressed&J_A)){
		if(!marks[cursor[0]][cursor[1]]){
			marks[cursor[0]][cursor[1]] = 1;
			set_bkg_tile_xy(cursor[0]+7,cursor[1]+6, 0);
		}else if(marks[cursor[0]][cursor[1]] == 1){
			marks[cursor[0]][cursor[1]] = 0;
			set_bkg_tile_xy(cursor[0]+7,cursor[1]+6, board[cursor[0]][cursor[1]]);
		}
	}
	if(input&J_B && !(pressed&J_B)){
		if(!marks[cursor[0]][cursor[1]]){
			marks[cursor[0]][cursor[1]] = 2;
			set_bkg_tile_xy(cursor[0]+7,cursor[1]+6, board[cursor[0]][cursor[1]] + 8);
		}else if(marks[cursor[0]][cursor[1]] == 2){
			marks[cursor[0]][cursor[1]] = 0;
			set_bkg_tile_xy(cursor[0]+7,cursor[1]+6, board[cursor[0]][cursor[1]]);
		}
	}
	if(input&J_SELECT && !(pressed&J_SELECT)){
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

