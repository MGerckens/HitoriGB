//*very* loosely adapted from Simon Tatham's version
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <rand.h>
#include <gb/gb.h>

#include "generator.h"
#include "latinsquare.h"
#include "queues.h"

uint8_t *randlist, *colnums, *rownums;
enum {EMPTY, BLACK, CIRCLE};
bool impossible = false;

#define INGRID(s,x,y) ((x) < (s) && (y) < (s))
#define CHECK(x) (((x)==1) ? 1 : 0)
static bool blacken_square(uint8_t x, uint8_t y, bool initial);
static void circle_square_xy(uint8_t x, uint8_t y);
static void circle_square(uint8_t i);
bool has_single_white_region();
static bool remove_splits(uint8_t coord);
static bool surrounded_edges();
static uint8_t fill(uint8_t xi, uint8_t yi);
static uint8_t best_duplicate(uint8_t coord);

void generate_board()
{
    uint8_t i, j, x, y;
	rownums = (uint8_t *)calloc(num_tiles,sizeof(uint8_t));
	colnums = (uint8_t *)calloc(num_tiles,sizeof(uint8_t));
	randlist = (uint8_t *)calloc(board_size,sizeof(uint8_t));
generate:
	set_bkg_tile_xy(0,3,0); //visual indicators for which stage the generator is in
	set_bkg_tile_xy(0,4,0);
	set_bkg_tile_xy(0,5,0);
	set_bkg_tile_xy(0,6,0);
	set_bkg_tile_xy(0,7,0);
	set_bkg_tile_xy(0,8,0);
	set_bkg_tile_xy(0,9,0);
    impossible = false; //flag if board becomes impossible
    
    //initialize board and solution
    latin_generate(); //generate base latin square
    for(i = 0; i < num_tiles; i++){
		solution[i] = EMPTY; //initialize solution
    }
	set_bkg_tile_xy(0,3,1);

    for(j = 0; j < num_tiles; j++) { //place initial black tiles
        i = arand() % num_tiles;
        if ((solution[i] == CIRCLE) || (solution[i] == BLACK)) {
            if(solution[i+2] == EMPTY){i += 2;}
            else{continue;} //skip already processed tile
        }

        //update edges, the filling function has issues with them and it's really fast
		//if(!surrounded_edges()){goto generate;}
        if(!blacken_square(i/board_size, i%board_size, true)){circle_square(i);} //place black tile
		set_bkg_tile_xy(0,4,1);
		
        if (impossible) { //restart if impossible
            //printf("generator made impossible, restarting...\n");
            goto generate;
        }

    }
	set_bkg_tile_xy(0,6,1);
    for(i = 0; i < num_tiles; i++){ //check each square to make sure all are either black or circled
		if(solution[i] == EMPTY){ 
			if(!surrounded_edges()){goto generate;}
			if(!blacken_square(i/board_size, i%board_size, true)){circle_square(i);} //place black square if possible
			if (impossible) {
				//printf("generator made impossible, restarting...\n");
				goto generate;
			}
		}
    }
	if(!has_single_white_region){goto generate;} //restart if board is split
	set_bkg_tile_xy(0,9,1);
    
    for(i = 0; i < num_tiles; i++){ //count number of times each number appears in each row and column
		if(solution[i] == BLACK){continue;} //exclude black tiles
		j = board[i];
		x = i/board_size;
		y = i%board_size;
		rownums[y*board_size + j-1]++;
		colnums[x*board_size + j-1]++;
	}
	
	for(i = 0; i < num_tiles; i++){ //assign duplicate numbers to black tiles
		if(solution[i] != BLACK){continue;}
		board[i] = best_duplicate(i);
	}
	set_bkg_tile_xy(0,3,0x15); //clear indicators
	set_bkg_tile_xy(0,4,0x15);
	set_bkg_tile_xy(0,5,0x15);
	set_bkg_tile_xy(0,6,0x15);
	set_bkg_tile_xy(0,7,0x15);
	set_bkg_tile_xy(0,8,0x15);
	set_bkg_tile_xy(0,9,0x15);
}

uint8_t best_duplicate(uint8_t coord){ //picks duplicate number to assign to black tile
	uint8_t i,j, temp, randval, x=coord/board_size, y=coord%board_size;
	for(i = 0; i < board_size; i++){randlist[i] = i+1;} //generate list of numbers from 1 to board_size
	for(i = board_size-1; i > 0; i--){ //shuffle array
		randval = (uint8_t)arand() % board_size;
		temp = randlist[i];
		randlist[i] = randlist[randval];
		randlist[randval] = temp;
	}
	for(i = 0; i < board_size; i++){ //prioritize numbers that appear once in the row and column, to minimize latin square characteristics
		j = randlist[i];
		if(rownums[y*board_size + j-1] == 1 && colnums[x*board_size + j-1] == 1){goto found;}
	}
	for(i = 0; i < board_size; i++){ //if none are found, use the first duplicate 
		j = randlist[i];
		if(rownums[y*board_size + j-1] || colnums[x*board_size + j-1]){goto found;}
	}
	return 0; //should not happen
	
found:
	rownums[y*board_size + j-1]++; //update counters with newly assigned number
	colnums[x*board_size + j-1]++;
	return j; 
}

static bool surrounded_edges(){ //check for surrounded tiles on edges
	set_bkg_tile_xy(0,6,1);
	uint8_t top,bottom,left,right,retval = 1;
	uint8_t bottom_offset = (board_size-1)*board_size;
	for(uint8_t i = 0; i < board_size; i++){
	    top = 0x07,bottom = 0x07,left = 0x07,right = 0x07; //use bit flags to mark which sides are open
		if(i){
			top -= CHECK(solution[i-1])*4;
			bottom -= CHECK(solution[bottom_offset + i-1]) *4;
			left -= CHECK(solution[(i-1)*board_size]) *4;
			right -= CHECK(solution[(i-1)*board_size + (board_size-1)]) *4;
		}else{
			top &=~ 0x04;
			bottom &=~ 0x04;
			left &=~ 0x04;
			right &=~ 0x04;
		}
		if(i!=board_size-1){
			top -= CHECK(solution[i+1])*2;
			bottom -= CHECK(solution[bottom_offset + i+1])*2;
			left -= CHECK(solution[(i+1)*board_size])*2;
			right -= CHECK(solution[(i+1)*board_size + (board_size-1)])*2;
		}else{
			top &=~ 0x02;
			bottom &=~ 0x02;
			left &=~ 0x02;
			right &=~ 0x02;
		}
		top &=~ CHECK(solution[i+board_size]);
		bottom &=~ CHECK(solution[bottom_offset-board_size + i]);
		left &=~ CHECK(solution[i*board_size + 1]);
		right &=~ CHECK(solution[i*board_size + board_size - 2]);
	    
	    if(CHECK(solution[i])){top = 0x07;} //if a tile is black, ignore it
	    if(CHECK(solution[bottom_offset + i])){bottom = 0x07;}
	    if(CHECK(solution[i*board_size])){left = 0x07;}
	    if(CHECK(solution[i*board_size + (board_size-1)])){right = 0x07;}
		
		switch(top){ //if a tile only has one side open, circle the tile on that side
	    case 1:
	        circle_square(i+board_size);
	        break;
	    case 2:
	        circle_square(i+1);
	        break;
	    case 4:
	        circle_square(i-1);
	    }
	    switch(bottom){
	    case 1:
	        circle_square(bottom_offset-board_size + i);
	        break;
	    case 2:
	        circle_square(bottom_offset + i+1);
	        break;
	    case 4:     
	        circle_square(bottom_offset + i-1);
	    }
        switch(left){
        case 1:
            circle_square(i*board_size + 1);
            break;
        case 2:
	        circle_square((i+1)*board_size);
	        break;
	    case 4:
	        circle_square((i-1)*board_size);
        }
	    switch(right){
	    case 1: 
	        circle_square(i*board_size + board_size - 2);
	        break;
	    case 2:
	        circle_square((i+1)*board_size + (board_size-1));
	        break;
	    case 4: 
	        circle_square((i-1)*board_size + (board_size-1));
	    }
	    
	    retval &= top && bottom && left && right; //return 0 if any tile is fully surrounded, otherwise 1
	    
		//printf("%d %d %d %d\n",top,bottom,left,right);
	}
	set_bkg_tile_xy(0,6,0);
    return retval;
}

bool has_single_white_region(){ //returns true if board is not split, otherwise false
	if(!surrounded_edges()){return false;}
	uint8_t black_count=0; //number of black tiles
	for(uint8_t i = 0; i < num_tiles; i++){ //this could be tracked by other functions but it was buggy and this worked first try
		if(solution[i] == BLACK){black_count++;}
	}
	if(black_count <= 1){return true;} //can't be split with less than 2
    uint8_t whitespace_count = (board_size * board_size) - black_count;

    uint8_t fillres;
    if(solution[0] != BLACK){
        fillres = fill(0,0);
    }else{
        fillres = fill(0,1);
    }
	//set_bkg_tile_xy(0,9,fillres >= whitespace_count);
    return fillres >= whitespace_count;
}

static bool blacken_square(uint8_t x, uint8_t y, bool initial){ //only returns false if an initial placement step fails
	uint8_t coord = x*board_size + y;							//i'm pretty sure these x's and y's are swapped but it still works
    if(solution[coord] == CIRCLE){ goto fail; }
    else if(solution[coord] == EMPTY) {
        //printf("blackening %d, %d\n",x,y);

        if (x == 0) { //fail if any sides are already black
            if (solution[coord-board_size] == BLACK) { goto fail;}
        }
        if (y == 0) {
            if (solution[coord-1] == BLACK) { goto fail; }
        }
        if (x == board_size) {
            if (solution[coord+board_size] == BLACK) { goto fail; }
        }
        if (y == board_size) {
            if (solution[coord+1] == BLACK) { goto fail; }
        }
		//set_bkg_tile_xy(x,y,0);
        solution[coord] = BLACK; //mark tile as black
		if(CHECK(solution[coord+board_size+1]) + CHECK(solution[coord+board_size-1]) + CHECK(solution[coord-board_size+1]) + CHECK(solution[coord-board_size-1]) >= 2){
			if(!has_single_white_region()){ //only check this if at least 2 diagonals are black
				solution[coord] = EMPTY;
				circle_square(coord);
				goto fail;
			}
		}

        //circle all neighboring squares
		circle_square_xy(x - 1, y);
		circle_square_xy(x, y - 1);
		circle_square_xy(x + 1, y);
		circle_square_xy(x, y + 1);

        return true;
        fail:
        if(!initial){ //only return true if not called as an initial placement
            impossible = true;
            //printf("impossible to blacken %d, %d, failing\n",x,y);
            return true;
        }
        //printf("impossible to blacken %d, %d, skipping\n",x,y);
        return false;
    }
	return false; //never happens, just here to make the compiler happy
}

static inline void circle_square_xy(uint8_t x, uint8_t y){ //wrapper function for a few places that need xy addressing
	circle_square(x*board_size + y);
}

static void circle_square(uint8_t i){
	uint8_t x = i/board_size;
	uint8_t y = i%board_size;
	if(!(INGRID(board_size,x,y))){return;} //bounds check
    if(solution[i] == BLACK) {
        impossible = true;
        //printf("impossible to circle %d, %d\n",x,y);
        return;
    }
    else if(solution[i] == EMPTY) {
        //printf("circling %d, %d\n",x,y);
        solution[i] = CIRCLE;
        for (uint8_t xt = 0; xt < board_size; xt++) { //black out redundant squares in same row
            if(xt != x && board[xt*board_size + y]==board[i]){
                blacken_square(xt,y, false);
            }
        }
        for (uint8_t yt = 0; yt < board_size; yt++) { //and same column
            if(yt != y && board[x*board_size + yt]==board[i]){
                blacken_square(x,yt, false);
            }
        }
    }
}

static uint8_t fill(uint8_t xi, uint8_t yi){ //floodfill using two queues
    if(solution[xi*board_size + yi] == BLACK){return 0;}
	set_bkg_tile_xy(0,8,1);
    //printf("started fill\n");
    bool *reachable = (bool *)calloc(num_tiles,sizeof(bool));
    node_t *Qx = NULL;			   //this version of SDCC doesn't support passing/returning structs
    node_t *Qy = NULL;			   //and this seemed easier than a queue of arrays
    enqueue(&Qx, xi);
    enqueue(&Qy, yi);
    uint8_t nx, ny;

    while(Qx!=NULL && Qy!=NULL){
        nx = dequeue(&Qx);
        ny = dequeue(&Qy);
		if(!INGRID(board_size,nx,ny)){continue;}
        if(reachable[nx*board_size + ny] == false && solution[nx*board_size + ny] != BLACK){
            reachable[nx*board_size + ny] = true;
			enqueue(&Qx, nx-1);
			enqueue(&Qy, ny);

			enqueue(&Qx, nx);
			enqueue(&Qy, ny-1);

			enqueue(&Qx, nx+1);
			enqueue(&Qy, ny);

			enqueue(&Qx, nx);
			enqueue(&Qy, ny+1);
        }
    }
    free(Qx);
    free(Qy);
	uint8_t count = 0;
	for(uint8_t i = 0; i < num_tiles; i++){ //tried putting this counter in the fill alg itself, but it doublecounts a lot
		if(reachable[i]){			//this isn't that much slower anyway
			count++;
		}
	}
    free(reachable);
    //printf("%d\n",count);
	set_bkg_tile_xy(0,8,0);
    return count;
}