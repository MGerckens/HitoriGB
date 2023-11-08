#include "latinsquare.h"

#include <gb/gb.h>
#include <rand.h>
#include <stdint.h>
#include <stdlib.h>

#define SWAPS 30

/**
 * @brief Swap two columns in a square array
 * @param square The array
 * @param size The length of the side of the square
 * @param col1 One column to swap
 * @param col2 Other column to swap
 */
void swapColumns(uint8_t* square, uint8_t size, uint8_t col1, uint8_t col2) {
    uint8_t temp;
    for (uint8_t x = 0; x < size; x++) {
        temp = square[col1 * size + x];
        square[col1 * size + x] = square[col2 * size + x];
        square[col2 * size + x] = temp;
    }
}

/**
 * @brief Swap two rows in a square array
 * @param square The array
 * @param size The length of the side of the square
 * @param row1 One row to swap
 * @param row2 Other row to swap
 */
void swapRows(uint8_t* square, uint8_t size, uint8_t row1, uint8_t row2) {
    uint8_t temp;
    for (uint8_t y = 0; y < size; y++) {
        temp = square[y * size + row1];
        square[y * size + row1] = square[y * size + row2];
        square[y * size + row2] = temp;
    }
}

/**
 * @brief Fill a square array with a random Latin square
 * @param square The array to fill
 * @param size The length of the side of the square
 */
void generateLatinSquare(uint8_t* square, uint8_t size) {
    // randomizer is already seeded
    uint8_t index1, index2, x, y, randval, temp;
    uint8_t* randlist = malloc(size * sizeof(uint8_t));

    for (x = 0; x < size; x++) {  // initialize list of numbers
        randlist[x] = x + 1;
    }

    for (x = 0; x < size; x++) {  // shuffles list, to generate non-repeating random sequence
        randval = arand() % size;
        temp = randlist[x];
        randlist[x] = randlist[randval];
        randlist[randval] = temp;
    }

    for (x = 0; x < size; x++) {  // assigns numbers in list to board column, offset by row
        for (y = 0; y < size; y++) {
            square[y * size + x] = randlist[(x + y) % size];
        }
    }

    for (x = 0; x < SWAPS; x++) {  // shuffle the rows and columns
        index1 = arand() % size;
        index2 = arand() % size;
        if (!(rand() & 0x01)) {  // swap columns or rows at random
            swapColumns(square, size, index1, index2);
        } else {
            swapRows(square, size, index1, index2);
        }
    }

    free(randlist);
}
