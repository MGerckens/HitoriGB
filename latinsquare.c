#include <stdlib.h>
#include <stdint.h>
#include <rand.h>
#include <gb/gb.h>
#include "latinsquare.h"

#define SWAPS 30

void swap_cols(uint8_t col1, uint8_t col2){ //swaps two cols on the board
	uint8_t temp;
    for(uint8_t x = 0; x < board_size; x++){
        temp = board[col1*board_size + x];
        board[col1*board_size + x] = board[col2*board_size + x];
        board[col2*board_size + x] = temp;
    }
}

void swap_rows(uint8_t row1, uint8_t row2){ //swaps two rows on the board
	uint8_t temp;
    for(uint8_t y = 0; y < board_size; y++){
        temp = board[y*board_size + row1];
        board[y*board_size + row1] = board[y*board_size + row2];
        board[y*board_size + row2] = temp;
    }
}


void latin_generate(){
    //randomizer is already seeded
    uint8_t index1, index2, x, y, randval, temp;
	uint8_t * randlist = malloc(board_size * sizeof(uint8_t));
	
	for(x = 0; x < board_size; x++){ //initialize list of numbers
		randlist[x] = x+1;
	}
	
	for(x = 0; x < board_size; x++){ //shuffles list, to generate non-repeating random sequence
		randval = arand() % board_size;
		temp = randlist[x];
		randlist[x] = randlist[randval];
		randlist[randval] = temp;
	}
	
    for(x = 0; x < board_size; x++){ //assigns numbers in list to board column, offset by row
        for (y = 0; y < board_size; y++) {
            board[y*board_size + x] = randlist[(x + y) % board_size];
        }
    }

    for(x = 0; x < SWAPS; x++){ //shuffle the rows and columns
        index1 = arand() % board_size;
        index2 = arand() % board_size;
        if(!(rand()&0x01)){ //swap columns or rows at random
            swap_cols(index1, index2);
        }else{
            swap_rows(index1, index2);
        }
    }

	free(randlist);
}

