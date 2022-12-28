#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <rand.h>

#include "latinsquare.h"

#include <gb/gb.h>
#define SWAPS 30


static inline void swap_cols(uint8_t col1, uint8_t col2){ //swaps two cols on the board
	uint8_t temp;
    for(uint8_t i = 0; i < board_size; i++){
        temp = board[col1*board_size + i];
        board[col1*board_size + i] = board[col2*board_size + i];
        board[col2*board_size + i] = temp;
    }
}

static inline void swap_rows(uint8_t row1, uint8_t row2){ //swaps two rows on the board
	uint8_t temp;
    for(uint8_t i = 0; i < board_size; i++){
        temp = board[i*board_size + row1];
        board[i*board_size + row1] = board[i*board_size + row2];
        board[i*board_size + row2] = temp;
    }
}


void latin_generate(){
	initarand(clock());
    uint8_t index1, index2, i, j, rand, temp;
	uint8_t *randlist = (uint8_t *)malloc(board_size * sizeof(uint8_t));
	
	for(i = 0; i < board_size; i++){ //initialize list of numbers
		randlist[i] = i+1;
	}
	
	for(i = 0; i < board_size; i++){ //shuffles list, to generate non-repeating random sequence
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

    for(i = 0; i < SWAPS; i++){ //shuffle the rows and columns, to be less predictable
        index1 = (uint8_t)arand() % board_size;
        do{index2 = (uint8_t)arand() % board_size;}while(index1 == index2);
        swap_rows(index1, index2);
		swap_cols(index1, index2);
    }
	free(randlist);
}