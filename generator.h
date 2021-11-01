#include <stdint.h>
#include <stdbool.h>

#ifndef TESTPROJ_GENERATOR_H
#define TESTPROJ_GENERATOR_H

extern uint8_t *board;
extern uint8_t *solution;
extern uint8_t board_size;
extern uint8_t num_tiles;

extern void generate_board();
extern bool has_single_white_region();

#endif //TESTPROJ_GENERATOR_H
