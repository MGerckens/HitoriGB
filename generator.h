#include <stdbool.h>
#include <stdint.h>

#ifndef GENERATOR_H
#define GENERATOR_H

extern uint8_t *board;
extern uint8_t *solution;
extern uint8_t boardSize;
extern uint8_t numTiles;

void generateBoard();

#endif  // GENERATOR_H
