#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <rand.h>

#include "latinsquare.h"

#include <gb/gb.h>
#define SWAPS 30


static inline void swap_rows(uint8_t row1, uint8_t row2){ //swaps two rows on the board
	uint8_t temp;
    for(uint8_t i = 0; i < board_size; i++){
        temp = board[row1*board_size + i];
        board[row1*board_size + i] = board[row2*board_size + i];
        board[row2*board_size + i] = temp;
    }
}


void latin_generate(){
	initarand(clock());
    uint8_t row1, row2, i, j, rand, temp;
	uint8_t *randlist = (uint8_t *)malloc(board_size * sizeof(uint8_t));
	
	for(i = 0; i < board_size; i++){ //initialize list of numbers
		randlist[i] = i+1;
	}
	
	for(i = board_size-1; i > 0; i--){ //shuffles list, to generate non-repeating random sequence
		rand = (uint8_t)arand() % board_size;
		temp = randlist[i];
		randlist[i] = randlist[rand];
		randlist[rand] = temp;
	}
	
    for(i = 0; i < board_size; i++){ //assigns numbers in list to board column, offset by row
        for (j = 0; j < board_size; j++) {
            board[i*board_size + j] = randlist[(i + j) % board_size];
        }
    }

    for(i = 0; i < SWAPS; i++){ //shuffle the rows, to be less predictable
        row1 = (uint8_t)arand() % board_size;
        do{row2 = (uint8_t)arand() % board_size;}while(row2 == row1);
        swap_rows(row1, row2);
    }
}