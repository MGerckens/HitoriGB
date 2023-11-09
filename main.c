#include <gb/gb.h>
#include <rand.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "generator.h"
#include "queues.h"
#include "tilemaps.h"

// offsets added to tile positions to keep board centered on screen
#define OFFSET_X (10 - (boardSize / 2))
#define OFFSET_Y (9 - (boardSize / 2))

// ID of the white tile
#define WHITE 41
enum { EMPTY, BLACK, CIRCLE };

uint8_t boardSize = 5;
uint8_t numTiles;

uint8_t cursor[2] = {0, 0};
uint8_t *board, *marks, *solution;

bool processInput();
void move(uint8_t input);
bool titleInput();
bool checkSolution();
uint8_t fill(uint8_t xi, uint8_t yi);

void main(void) {
    DISPLAY_ON;
    SPRITES_8x8;
restart:
    HIDE_WIN;

    set_bkg_data(0, 59, titleTiles);
    set_bkg_tiles(0, 0, 20, 18, titleLayout);
    // reset bkg location from if it was offset before
    move_bkg(0, 0);
    SHOW_BKG;

    // update board size once immediately, then wait for all inputs to be released
    set_bkg_tile_xy(9, 9, boardSize + 10);
    waitpadup();
    do {
    } while (!titleInput());  // wait on title screen

    HIDE_BKG;
    numTiles = boardSize * boardSize;  // used in a lot of loops

    board = malloc(numTiles * sizeof(uint8_t));
    marks = calloc(numTiles, sizeof(uint8_t));
    solution = malloc(numTiles * sizeof(uint8_t));

    set_bkg_data(0, 42, boardTiles);
    set_bkg_tiles(0, 0, 21, 19, boardLayout);
    SHOW_BKG;
    set_bkg_tile_xy(1, 1, 33);  // loading icon
    set_bkg_tile_xy(2, 1, 31);
    set_bkg_tile_xy(3, 1, 32);
    initarand(clock());

    generateBoard();

    set_bkg_tile_xy(1, 1, WHITE);  // remove loading icon
    set_bkg_tile_xy(2, 1, WHITE);
    set_bkg_tile_xy(3, 1, WHITE);

    set_sprite_data(0, 2, cursorTiles);  // load cursor sprite
    set_sprite_tile(0, 0);

    set_win_tiles(0, 0, 21, 1, boardLayout);

    // offset bkg layer if boardSize is odd, to center the board on the screen
    if (boardSize & 0x01) {
        move_bkg(4, 4);
    }
    move_win(7, 136);
    move_sprite(0, (OFFSET_X + 1) * 8, (OFFSET_Y + 2) * 8);
    // write generated board to screen
    for (int i = 0; i < numTiles; i++) {
        set_bkg_tile_xy((i % boardSize) + OFFSET_X, (i / boardSize) + OFFSET_Y, board[i]);
    }
    SHOW_WIN;
    SHOW_SPRITES;

    while (1) {                // main game loop
        if (processInput()) {  // If start+select, restart game
            cursor[0] = 0;
            cursor[1] = 0;
            free(board);
            free(marks);
            free(solution);

            HIDE_SPRITES;  // avoid tile flickering during restart
            HIDE_BKG;
            goto restart;
        }
        vsync();
    }
}

#define BUTTON_DOWN(x) ((input & (x)) && !(lastInput & (x)))
/**
 * @brief Process input on the title screen to select a board size
 * @return True if start is pressed to pick a size, false otherwise
 */
bool titleInput() {
    uint8_t input = joypad();
    static uint8_t lastInput = 0;
    if (BUTTON_DOWN(J_LEFT)) {
        if (boardSize > 5) {  // update board size
            boardSize--;
        } else {
            boardSize = 15;
        }
    }
    if (BUTTON_DOWN(J_RIGHT)) {
        if (boardSize < 15) {
            boardSize++;
        } else {
            boardSize = 5;
        }
    }
    if (BUTTON_DOWN(J_START)) {
        return true;
    }
    // update display, tile IDs 15-25 are graphics for the numbers 5-15
    set_bkg_tile_xy(9, 9, boardSize + 10);
    lastInput = input;
    return false;
}

/**
 * @brief Process input during the game
 * @return True if start+select is pressed to quit to title, false otherwise
 */
bool processInput() {
    uint8_t input = joypad();
    uint8_t coord, x, y;
    bool returnVal = false;
    static uint8_t lastInput = 0;
    static bool solutionDisplayed = false;
    static uint16_t lastSolutionCheckTime = 0;

    if (sys_time - lastSolutionCheckTime > 60) {
        move_win(7, 136);  // recenter window layer
        set_win_tile_xy(8, 0, WHITE);
        set_win_tile_xy(9, 0, WHITE);
        set_win_tile_xy(10, 0, WHITE);
        set_win_tile_xy(11, 0, WHITE);
        lastSolutionCheckTime = sys_time;
    }

    static time_t lastInputChangeTime = 0;  // time the current direction on the d-pad was pressed
    if (lastInput == input) {
        if ((sys_time - lastInputChangeTime > 20) && !(clock() % 4)) {  // if held for less than 0.5s, skip input
            // if longer than 0.5s, move every 4th frame
            move(input);
        }
    } else {
        lastInputChangeTime = sys_time;
        // only use newly pressed button, stops weird behavior when switching from straight to
        // diagonal
        move(input & ~lastInput);
    }

    if (BUTTON_DOWN(J_A)) {  // toggle shaded
        coord = cursor[1] * boardSize + cursor[0];
        if (!marks[coord]) {
            marks[coord] = BLACK;
            set_bkg_tile_xy(cursor[0] + OFFSET_X, cursor[1] + OFFSET_Y, 0);
        } else if (marks[coord] == BLACK) {
            marks[coord] = EMPTY;
            set_bkg_tile_xy(cursor[0] + OFFSET_X, cursor[1] + OFFSET_Y, board[coord]);
        }
    }
    if (BUTTON_DOWN(J_B)) {  // toggle circled
        coord = cursor[1] * boardSize + cursor[0];
        if (!marks[coord]) {
            marks[coord] = CIRCLE;
            set_bkg_tile_xy(cursor[0] + OFFSET_X, cursor[1] + OFFSET_Y, board[coord] + 15);
        } else if (marks[coord] == CIRCLE) {
            marks[coord] = EMPTY;
            set_bkg_tile_xy(cursor[0] + OFFSET_X, cursor[1] + OFFSET_Y, board[coord]);
        }
    }
    if (BUTTON_DOWN(J_SELECT) && BUTTON_DOWN(J_START)) {  // start+select to return to title
        returnVal = true;
    } else if (BUTTON_DOWN(J_SELECT)) {  // displays if solution is correct
        if (checkSolution()) {
            move_win(7, 136);
            set_win_tile_xy(8, 0, 34);
            set_win_tile_xy(9, 0, 35);
            set_win_tile_xy(10, 0, 36);
            set_win_tile_xy(11, 0, 37);
        } else {
            move_win(12, 136);
            set_win_tile_xy(8, 0, 38);
            set_win_tile_xy(9, 0, 39);
            set_win_tile_xy(10, 0, 40);
            // overwrite a previous "correct", which would have written here
            set_win_tile_xy(11, 0, WHITE);
        }
        lastSolutionCheckTime = sys_time;
    } else if (BUTTON_DOWN(J_START)) {  // display correct solution for debugging
        if (!solutionDisplayed) {
            for (x = 0; x < boardSize; x++) {
                for (y = 0; y < boardSize; y++) {
                    coord = y * boardSize + x;
                    marks[coord] = solution[coord];
                    if (solution[coord] == BLACK) {
                        set_bkg_tile_xy(x + OFFSET_X, y + OFFSET_Y, 0);
                    }
                    if (solution[coord] == CIRCLE) {
                        set_bkg_tile_xy(x + OFFSET_X, y + OFFSET_Y, board[coord] + 15);
                    }
                }
            }
            solutionDisplayed = true;
        } else {
            for (x = 0; x < boardSize; x++) {
                for (y = 0; y < boardSize; y++) {
                    coord = y * boardSize + x;
                    marks[coord] = EMPTY;
                    set_bkg_tile_xy(x + OFFSET_X, y + OFFSET_Y, board[coord]);
                }
            }
            solutionDisplayed = false;
        }
    }

    move_sprite(0, (cursor[0] + OFFSET_X + 1) * 8, (cursor[1] + OFFSET_Y + 2) * 8);
    if (boardSize & 0x01) {
        scroll_sprite(0, -4, -4);
    }  // offset cursor by 4px if boardSize is odd
    // change cursor color when on a black square to keep it visible
    if (marks[cursor[1] * boardSize + cursor[0]] == BLACK) {
        set_sprite_tile(0, 1);
    } else {
        set_sprite_tile(0, 0);
    }
    lastInput = input;
    return returnVal;
}

/**
 * @brief Update the cursor's position
 * @param input Input bitfield
 * @note Doesn't update sprite position, just variable
 */
void move(uint8_t input) {
    if (input & J_RIGHT) {
        if (cursor[0] < boardSize - 1) {
            cursor[0]++;
        } else {
            cursor[0] = 0;
        }
    }
    if (input & J_LEFT) {
        if (cursor[0] > 0) {
            cursor[0]--;
        } else {
            cursor[0] = boardSize - 1;
        }
    }
    if (input & J_UP) {
        if (cursor[1] > 0) {
            cursor[1]--;
        } else {
            cursor[1] = boardSize - 1;
        }
    }
    if (input & J_DOWN) {
        if (cursor[1] < boardSize - 1) {
            cursor[1]++;
        } else {
            cursor[1] = 0;
        }
    }
}

#define CHECK(x) ((x == 1) ? 1 : 0)
/**
 * @brief Check if marks == solution, then use BFS to test if marks is a valid solution
 */
bool checkSolution() {
    bool isCorrect = true;
    uint8_t x, y;
    for (x = 0; x < numTiles; x++) {  // check if player marks match solution
        if (CHECK(solution[x]) != CHECK(marks[x])) {
            isCorrect = false;
            break;
        }
    }
    if (isCorrect) {  // if perfect match, no need to check further
        return true;
    } else {
        /* the rest is in case of a rare board with multiple solutions, ex:
         * 6 3 7 3 2 1 4
         * 3 1 5 4 7 5 3
         * 6 6 2 1 3 3 5
         * 5 4 4 7 3 5 6
         * 3 7 4 3 6 6 7
         * 1 5 2 2 6 4 7
         * 5 2 5 2 1 1 3
         * has two involving the 6s in top left */

        uint8_t *rowCount = calloc(numTiles, sizeof(uint8_t));
        uint8_t *colCount = calloc(numTiles, sizeof(uint8_t));
        uint8_t blackCount = 0;
        for (x = 0; x < boardSize; x++) {  // store count of each number in rows and columns
            for (y = 0; y < boardSize; y++) {
                if (marks[y * boardSize + x] != BLACK) {
                    rowCount[y * boardSize + board[y * boardSize + x] - 1]++;
                    colCount[x * boardSize + board[y * boardSize + x] - 1]++;
                } else {
                    blackCount++;
                }  // also count black squares for next step
            }
        }

        for (x = 0; x < boardSize;
             x++) {  // if any row or column has a duplicate, solution is false
            for (y = 0; y < boardSize; y++) {
                if (colCount[y * boardSize + x] > 1) {
                    free(rowCount);
                    free(colCount);
                    return false;
                }
                if (rowCount[y * boardSize + x] > 1) {
                    free(rowCount);
                    free(colCount);
                    return false;
                }
            }
        }

        if (marks[0] != BLACK) {  // use flood fill to check for contiguity
            if (fill(0, 0) < (numTiles - blackCount)) {
                free(rowCount);
                free(colCount);
                return false;
            }
        } else {
            if (fill(0, 1) < (numTiles - blackCount)) {
                free(rowCount);
                free(colCount);
                return false;
            }
        }
        free(rowCount);
        free(colCount);
        return true;
    }
}
uint8_t fill(uint8_t xi, uint8_t yi) {  // floodfill using two queues

    bool *reachable = calloc(numTiles, sizeof(bool));
    node_t *Qx = NULL;
    node_t *Qy = NULL;
    enqueue(&Qx, xi);
    enqueue(&Qy, yi);
    uint8_t nx, ny;

    while (Qx != NULL && Qy != NULL) {
        nx = dequeue(&Qx);
        ny = dequeue(&Qy);
        if (!(nx < boardSize && ny < boardSize)) {
            continue;
        }
        if (reachable[ny * boardSize + nx] == false && marks[ny * boardSize + nx] != BLACK) {
            reachable[ny * boardSize + nx] = true;
            enqueue(&Qx, nx - 1);
            enqueue(&Qy, ny);

            enqueue(&Qx, nx);
            enqueue(&Qy, ny - 1);

            enqueue(&Qx, nx + 1);
            enqueue(&Qy, ny);

            enqueue(&Qx, nx);
            enqueue(&Qy, ny + 1);
        }
    }
    free(Qx);
    free(Qy);
    uint8_t count = 0;
    for (uint8_t i = 0; i < numTiles; i++) {
        if (reachable[i]) {
            count++;
        }
    }
    free(reachable);
    return count;
}
