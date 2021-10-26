#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "latinsquare.h"
#include <time.h>
#include <rand.h>

#include <gb/gb.h>
#define SWAPS 10

static void swap_rows(uint8_t row1, uint8_t row2){
	uint8_t temp;
    for(uint8_t i = 0; i < board_size; i++){
        temp = board[row1*board_size + i];
        board[row1*board_size + i] = board[row2*board_size + i];
        board[row2*board_size + i] = temp;
    }
}


void latin_generate() {
	initarand(clock());
    uint8_t counter = arand() % board_size;
    uint8_t row1, row2;
    for(uint8_t i = 0; i < board_size; i++){
        for (uint8_t j = 0; j < board_size; j++) {
            board[i*board_size + j] = (counter++ % board_size) + 1;
			//set_bkg_tile_xy(i+7,j+6,board[i*board_size + j]);
        }
        counter++;

    }

    for(uint8_t i = 0; i < SWAPS; i++){
        row1 = (uint8_t)arand() % board_size;
        do{row2 = (uint8_t)arand() % board_size;}while(row2 == row1);
		//printf("%u, %u",row1,row2);
		//set_bkg_tile_xy(0,0,row1);
		//set_bkg_tile_xy(0,1,row2);
        swap_rows(row1, row2);
    }
}