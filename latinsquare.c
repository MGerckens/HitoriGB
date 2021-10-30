#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "latinsquare.h"
#include <time.h>
#include <rand.h>

#include <gb/gb.h>
#define SWAPS 30


static inline void swap_rows(uint8_t row1, uint8_t row2){
	uint8_t temp;
    for(uint8_t i = 0; i < board_size; i++){
        temp = board[row1*board_size + i];
        board[row1*board_size + i] = board[row2*board_size + i];
        board[row2*board_size + i] = temp;
    }
}


void latin_generate() {
	initarand(clock());
    //uint8_t counter = arand() % board_size;
    uint8_t row1, row2, i, j, rand, temp;
	uint8_t *randlist = (uint8_t *)malloc(board_size * sizeof(uint8_t));
	
	for(i = 0; i < board_size; i++){ 
		//set_bkg_tile_xy(i,0,i+1);
		randlist[i] = i+1;
	}
	for(i = board_size-1; i > 0; i--){
		rand = (uint8_t)arand() % board_size;
		temp = randlist[i];
		randlist[i] = randlist[rand];
		randlist[rand] = temp;
	}
    for(i = 0; i < board_size; i++){
        for (j = 0; j < board_size; j++) {
            board[i*board_size + j] = randlist[(i + j) % board_size];
			//set_bkg_tile_xy(i+7,j+6,board[i*board_size + j]);
        }
        //counter++;

    }

    for(i = 0; i < SWAPS; i++){
        row1 = (uint8_t)arand() % board_size;
        do{row2 = (uint8_t)arand() % board_size;}while(row2 == row1);
		//printf("%u, %u",row1,row2);
		//set_bkg_tile_xy(0,0,row1);
		//set_bkg_tile_xy(0,1,row2);
        swap_rows(row1, row2);
    }
}