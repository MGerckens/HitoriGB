#include <stdint.h>
#include <stdbool.h>

#ifndef GENERATOR_H
#define GENERATOR_H

extern uint8_t *board;
extern uint8_t *solution;
extern uint8_t board_size;
extern uint8_t num_tiles;

extern void generate_board();


#endif //GENERATOR_H
