#include "generator.h"

#include <gb/gb.h>
#include <rand.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "latinsquare.h"

typedef struct Face {
    uint8_t val;  // not really needed but makes debugging way easier
    struct Face *parent;
} face_t;

//'infinite' face representing the outside of the board
const face_t infiniteFace = {0, &infiniteFace};

uint8_t neighborSquares[8];  // return values of neighborSquares
face_t *neighorFaces[4];     // return value of getNeighborFaces
face_t *faces;               // stores faces between vertices

void initFaces();
void blackenSquare(uint8_t coord);
void getNeighborSquares(uint8_t x, uint8_t y);
uint8_t getNeighborFaces(uint8_t x, uint8_t y);
face_t *getFace(uint8_t x, uint8_t y);
void generateBoard();

face_t *getRoot(face_t *self) {  // returns root of face
    face_t *grandparent = NULL;
    while (self != self->parent) {
        grandparent = self->parent->parent;
        self->parent = grandparent;
        self = grandparent;
    }
    return self;
}

// merges other into self by making self's root the parent of other's root
inline void merge(face_t *self, face_t *other) { getRoot(other)->parent = getRoot(self); }

/**
 * @brief Initialize faces for union-find
 */
void initFaces() {  // set up solution and faces
    // allocates (boardSize-1)^2, since (x-1)^2 = x^2-2x+1
    faces = malloc((numTiles - 2 * boardSize + 1) * sizeof(face_t));
    uint8_t x, y;
    for (x = 0; x < boardSize; x++) {
        for (y = 0; y < boardSize; y++) {
            solution[y * boardSize + x] = 0;
        }
    }
    for (x = 0; x < boardSize - 1; x++) {
        for (y = 0; y < boardSize - 1; y++) {
            faces[y * (boardSize - 1) + x].val = y * (boardSize - 1) + x + 1;
            faces[y * (boardSize - 1) + x].parent = &faces[y * (boardSize - 1) + x];
        }
    }
}

/**
 * @brief Get all neighboring squares, put them into neighborSquares
 * @param x x coord of face
 * @param y y coord of face
 */
void getNeighborSquares(uint8_t x, uint8_t y) {
    if (x > 0) {
        neighborSquares[0] = x - 1;
        neighborSquares[1] = y;
    } else {
        neighborSquares[0] = 255;
        neighborSquares[1] = 255;
    }
    if (y > 0) {
        neighborSquares[2] = x;
        neighborSquares[3] = y - 1;
    } else {
        neighborSquares[2] = 255;
        neighborSquares[3] = 255;
    }
    if (x < boardSize - 1) {
        neighborSquares[4] = x + 1;
        neighborSquares[5] = y;
    } else {
        neighborSquares[4] = 255;
        neighborSquares[5] = 255;
    }
    if (y < boardSize - 1) {
        neighborSquares[6] = x;
        neighborSquares[7] = y + 1;
    } else {
        neighborSquares[6] = 255;
        neighborSquares[7] = 255;
    }
}

/**
 * @brief Finds all neighboring faces and puts them in neighborFaces
 * @param x x coord of face
 * @param y y coord of face
 * @return Number of unique faces found
 */
uint8_t getNeighborFaces(uint8_t x, uint8_t y) {
    neighorFaces[0] = getFace(x - 1, y - 1);
    uint8_t count = 1;

    neighorFaces[1] = getFace(x - 1, y);
    if (neighorFaces[1] != neighorFaces[0]) {
        count++;
    }

    neighorFaces[2] = getFace(x, y - 1);

    if (neighorFaces[2] != neighorFaces[1] && neighorFaces[2] != neighorFaces[0]) {
        count++;
    }

    neighorFaces[3] = getFace(x, y);
    if (neighorFaces[3] != neighorFaces[2] && neighorFaces[3] != neighorFaces[1] &&
        neighorFaces[3] != neighorFaces[0]) {
        count++;
    }

    return count;
}

/**
 * @brief Gets the root of a face
 * @param x x coord of face
 * @param y y coord of face
 * @return The root of the face, or infiniteFace if (x,y) is out of bounds
 */
face_t *getFace(uint8_t x, uint8_t y) {
    if (x < boardSize - 1 && y < boardSize - 1) {
        return getRoot(&faces[y * (boardSize - 1) + x]);
    } else {
        return &infiniteFace;
    }
}

/**
 * @brief Blacken a square
 * @param coord Location of the square
 */
void blackenSquare(uint8_t coord) {
    uint8_t x = coord / boardSize;
    uint8_t y = coord % boardSize;

    if (solution[y * boardSize + x]) {
        return;
    }
    uint8_t i;

    getNeighborSquares(x, y);  // gets neighbor vertices and stores coords in neighborSquares

    for (i = 0; i < 8; i += 2) {
        if (neighborSquares[i] != 255 &&
            solution[neighborSquares[i + 1] * boardSize + neighborSquares[i]]) {
            return;
        }  // stop if any neighbor is already black
    }

    uint8_t delta_E = 0;
    for (i = 0; i < 8; i += 2) {
        if (neighborSquares[i] != 255) {
            delta_E++;
        }  // count neigboring edges
    }

    uint8_t delta_F = getNeighborFaces(x, y);  // count neigboring faces

    if (delta_E - delta_F) {
        return;
    }                                 // fail if graph is not fully connected
    solution[y * boardSize + x] = 1;  // if it is still connected, mark the square as blackened

    // initialize index to stop compiler warning, this value is always overwritten
    uint8_t index = 0;
    bool found = false;
    for (int8_t i = 0; i < 4; i++) {
        if (neighorFaces[i] == &infiniteFace) {  // check if any of the neighboring faces are
                                                 // merged with the infinite face
            index = i;
            found = true;
        }
    }

    // if none are, merge to top-left by default. Any direction works as long as it's consistent
    if (!found) {
        index = 0;
    }

    for (i = 0; i < 4; i++) {
        merge(neighorFaces[index], neighorFaces[i]);
    }
}

/**
 * @brief Generates a game board and solution
 */
void generateBoard() {
    initFaces();
    uint8_t i, x, y, randval, temp, coord, best;
    uint8_t *randListOrder = malloc(numTiles * sizeof(uint8_t));
    uint8_t *randListDuplicates = malloc(boardSize * sizeof(uint8_t));
    uint8_t *colnums = calloc(numTiles, sizeof(uint8_t));
    uint8_t *rownums = calloc(numTiles, sizeof(uint8_t));

    for (i = 0; i < numTiles; i++) {  // generate list of numbers from 1 to numTiles
        randListOrder[i] = i;
    }
    for (i = 0; i < numTiles; i++) {  // shuffle array
        randval = (uint8_t)arand() % numTiles;
        temp = randListOrder[i];
        randListOrder[i] = randListOrder[randval];
        randListOrder[randval] = temp;
    }
    for (i = 0; i < numTiles; i++) {  // attempt to blacken all squares in random order
        blackenSquare(randListOrder[i]);
    }

    generateLatinSquare(board, boardSize);

    for (coord = 0; coord < numTiles;
         coord++) {  // count number of times each number appears in each row and column
        if (solution[coord]) {
            continue;
        }  // exclude black tiles
        best = board[coord];
        x = coord % boardSize;
        y = coord / boardSize;
        rownums[y * boardSize + best - 1]++;
        colnums[x * boardSize + best - 1]++;
    }
    for (coord = 0; coord < numTiles; coord++) {
        if (solution[coord]) {
            x = coord % boardSize;
            y = coord / boardSize;
            for (i = 0; i < boardSize; i++) {
                randListDuplicates[i] = i + 1;
            }                                  // generate list of numbers from 1 to boardSize
            for (i = 0; i < boardSize; i++) {  // shuffle array
                randval = (uint8_t)arand() % boardSize;
                temp = randListDuplicates[i];
                randListDuplicates[i] = randListDuplicates[randval];
                randListDuplicates[randval] = temp;
            }
            for (i = 0; i < boardSize; i++) {
                // prioritize numbers that appear once in the row and column, to minimize
                // latin square characteristics
                best = randListDuplicates[i];
                if (rownums[y * boardSize + best - 1] == 1 &&
                    colnums[x * boardSize + best - 1] == 1) {
                    goto found;
                }
            }
            for (i = 0; i < boardSize; i++) {  // if none are found, use the first duplicate
                best = randListDuplicates[i];
                if (rownums[y * boardSize + best - 1] || colnums[x * boardSize + best - 1]) {
                    goto found;
                }
            }
            best = 0;  // should not happen

        found:
            rownums[y * boardSize + best - 1]++;  // update counters with newly assigned number
            colnums[x * boardSize + best - 1]++;
            board[coord] = best;
        }
    }
    free(colnums);
    free(rownums);
    free(randListDuplicates);
    free(randListOrder);
    free(faces);
}
