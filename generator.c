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

uint8_t num,shade_count,n;
uint8_t *randlist, *colnums, *rownums;
enum {EMPTY, BLACK, CIRCLE};
bool impossible = false;

#define INGRID(s,x,y) ((x) < (s) && (y) < (s))
#define CHECK(x) ((x==1) ? 1 : 0)
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
    n = board_size * board_size;
	rownums = (uint8_t *)calloc(n,sizeof(uint8_t));
	colnums = (uint8_t *)calloc(n,sizeof(uint8_t));
	randlist = (uint8_t *)calloc(board_size,sizeof(uint8_t));
generate:
	set_bkg_tile_xy(0,3,0);
	set_bkg_tile_xy(0,4,0);
	set_bkg_tile_xy(0,5,0);
	set_bkg_tile_xy(0,6,0);
	set_bkg_tile_xy(0,7,0);
	set_bkg_tile_xy(0,8,0);
	set_bkg_tile_xy(0,9,0);
    shade_count = 0;
    num = n;
    impossible = false;
    
    //initialize board and solution
    latin_generate();
    for(i = 0; i < n; i++){
		solution[i] = EMPTY;
    }
	set_bkg_tile_xy(0,3,1);

    /* Add black squares at random, using bits of solver as we go (to lay
     * white squares), until we can lay no more blacks. */

    for(j = 0; j < n; j++) { //by the end, every square should be black or circled
        i = arand() % n;
        if ((solution[i] == CIRCLE) || (solution[i] == BLACK)) {
            if(solution[i+2] == EMPTY){i += 2;}
            else{continue;} /* solver knows this must be one or the other already. */
        }

        /* Add a random black cell... */
		if(!surrounded_edges()){goto generate;}
        if(!blacken_square(i/board_size, i%board_size, true)){continue;}
		set_bkg_tile_xy(0,4,1);
		
        if (impossible) {
            //printf("generator made impossible, restarting...\n");
            goto generate;
        }

    }
    for(i = 0; i < n; i++){
		set_bkg_tile_xy(0,6,1);
		if(solution[i] == EMPTY){
			surrounded_edges();
			if(remove_splits(i)){continue;}
			if(!blacken_square(i/board_size, i%board_size, true)){
				circle_square(i);
			}
			//remove_splits();
			
			if (impossible) {
				//printf("generator made impossible, restarting...\n");
				goto generate;
			}
		}
    }
	if(!has_single_white_region){goto generate;}
	set_bkg_tile_xy(0,9,1);
	
    
    for(i = 0; i < n; i++){
		if(solution[i] == BLACK){continue;}
		j = board[i];
		x = i/board_size;
		y = i%board_size;
		rownums[y*board_size + j-1]++;
		colnums[x*board_size + j-1]++;
	}
	
	for(i = 0; i < n; i++){
		if(solution[i] != BLACK){continue;}
		board[i] = best_duplicate(i);
	}
	set_bkg_tile_xy(0,3,0x15);
	set_bkg_tile_xy(0,4,0x15);
	set_bkg_tile_xy(0,5,0x15);
	set_bkg_tile_xy(0,6,0x15);
	set_bkg_tile_xy(0,7,0x15);
	set_bkg_tile_xy(0,8,0x15);
	set_bkg_tile_xy(0,9,0x15);
}

uint8_t best_duplicate(uint8_t coord){
	uint8_t i,j, temp, randval, x=coord/board_size, y=coord%board_size;
	for(i = 0; i < board_size; i++){randlist[i] = i+1;}
	for(i = board_size-1; i > 0; i--){
		randval = (uint8_t)arand() % board_size;
		temp = randlist[i];
		randlist[i] = randlist[randval];
		randlist[randval] = temp;
	}
	for(i = 0; i < board_size; i++){
		j = randlist[i];
		if(rownums[y*board_size + j-1] == 1 && colnums[x*board_size + j-1] == 1){goto found;}
	}
	for(i = 0; i < board_size; i++){
		j = randlist[i];
		if(rownums[y*board_size + j-1] || colnums[x*board_size + j-1]){goto found;}
	}
	return 0; //should not happen
	
found:
	rownums[y*board_size + j-1]++;
	colnums[x*board_size + j-1]++;
	return j;
}

static bool surrounded_edges(){ 
	set_bkg_tile_xy(0,6,1);
	uint8_t top,bottom,left,right,retval = 1;
	uint8_t bottom_offset = (board_size-1)*board_size;
	for(uint8_t i = 0; i < board_size; i++){
	    top = 0x07,bottom = 0x07,left = 0x07,right = 0x07;
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
	    
	    if(CHECK(solution[i])){top = 0x07;}
	    if(CHECK(solution[bottom_offset + i])){bottom = 0x07;}
	    if(CHECK(solution[i*board_size])){left = 0x07;}
	    if(CHECK(solution[i*board_size + (board_size-1)])){right = 0x07;}
		
		switch(top){
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
	    
	    retval &= top && bottom && left && right;
	    
		//printf("%d %d %d %d\n",top,bottom,left,right);
	}
	set_bkg_tile_xy(0,6,0);
    return retval;
}

static bool remove_splits(uint8_t coord){
	if(shade_count <= 1){return true;}
	set_bkg_tile_xy(0,7,1);
    //uint8_t coord;
    /*if(!has_single_white_region()){
        //printf("already split\n");
        impossible = true;
		set_bkg_tile_xy(0,7,0);
        return false;
    }*/
	//coord = x*board_size + y;
	if(solution[coord] != EMPTY){return false;}
	solution[coord] = BLACK;
	if(!has_single_white_region()){
		solution[coord] = EMPTY;
		circle_square(coord);
		return true;
	}
	solution[coord] = EMPTY;
	set_bkg_tile_xy(0,7,0);
	return false;
}

bool has_single_white_region(){
	if(!surrounded_edges()){return false;}
	uint8_t sc=0; //this could be tracked by other functions but it was buggy and this worked first try
	for(uint8_t i = 0; i < n; i++){
		if(solution[i] == BLACK){sc++;}
	}
	if(sc <= 1){return true;}
    uint8_t whitespace_count = (board_size * board_size) - sc;

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
        uint8_t x_low = 1, y_low = 1, x_high = 1, y_high = 1;
        if (x == 0) {
            x_low = 0;
        }
        if (y == 0) {
            y_low = 0;
        }
        if (x == board_size) {
            x_high = 0;
        }
        if (y == board_size) {
            y_high = 0;
        }

        if (x_low) {
            if (solution[coord-board_size] == BLACK) { goto fail;}
        }
        if (y_low) {
            if (solution[coord-1] == BLACK) { goto fail; }
        }
        if (x_high) {
            if (solution[coord+board_size] == BLACK) { goto fail; }
        }
        if (y_high) {
            if (solution[coord+1] == BLACK) { goto fail; }
        }
		//set_bkg_tile_xy(x,y,0);
        solution[coord] = BLACK;
		if(CHECK(solution[coord+board_size+1]) + CHECK(solution[coord+board_size-1]) + CHECK(solution[coord-board_size+1]) + CHECK(solution[coord-board_size-1]) >= 2){
			if(!has_single_white_region()){
				solution[coord] = EMPTY;
				//num++;
				circle_square(coord);
				goto fail;
			}
		}
		//if(!has_single_white_region()){goto fail;}
        shade_count++;
        num--;

        if (x_low) {
            circle_square_xy(x - 1, y);
        }
        if (y_low) {
            circle_square_xy(x, y - 1);
        }
        if (x_high) {
            circle_square_xy(x + 1, y);
        }
        if (y_high) {
            circle_square_xy(x, y + 1);
        }

        return true;
        fail:
        if(!initial){
            impossible = true;
            //printf("impossible to blacken %d, %d, failing\n",x,y);
            return true;
        }
        //printf("impossible to blacken %d, %d, skipping\n",x,y);
        return false;
    }
}

static inline void circle_square_xy(uint8_t x, uint8_t y){
	circle_square(x*board_size + y);
}

static void circle_square(uint8_t i){
	uint8_t x = i/board_size;
	uint8_t y = i%board_size;
    if(!(INGRID(board_size,x,y))){return;}
    if(solution[i] == BLACK) {
        impossible = true;
        //printf("impossible to circle %d, %d\n",x,y);
        return;
    }
    else if(solution[i] == EMPTY) {
        //printf("circling %d, %d\n",x,y);
        num--;
        solution[i] = CIRCLE;
        for (uint8_t xt = 0; xt < board_size; xt++) {
            if(xt != x && board[xt*board_size + y]==board[i]){
                blacken_square(xt,y, false);
            }
        }
        for (uint8_t yt = 0; yt < board_size; yt++) {
            if(yt != y && board[x*board_size + yt]==board[i]){
                blacken_square(x,yt, false);
            }
        }
    }
}

static uint8_t fill(uint8_t xi, uint8_t yi){ //floodfill using two queues
    
    if(solution[xi*board_size + yi] == BLACK){return 0;}
	set_bkg_tile_xy(0,8,1);
	//if(!shade_count){return board_size*board_size;}
	//if(shade_count == 1){return board_size*board_size - 1;}
	//set_bkg_tile_xy(5,1,shade_count);
    //printf("started fill\n");
    bool *reachable = (bool *)calloc(n,sizeof(bool));
    node_t *Qx = NULL;			   //this version of SDCC doesn't support passing/returning structs
    node_t *Qy = NULL;			   //and this seemed easier than a queue of arrays
    //uint8_t count = 1;
    enqueue(&Qx, xi);
    enqueue(&Qy, yi);
    uint8_t nx, ny;

    while(Qx!=NULL && Qy!=NULL){
		//set_bkg_tile_xy(0,8,flag);

        nx = dequeue(&Qx);
        ny = dequeue(&Qy);
		//set_bkg_tile_xy(3,0,nx);
		//set_bkg_tile_xy(4,0,ny);
		//delay(50);
		if(!INGRID(board_size,nx,ny)){continue;}
        if(reachable[nx*board_size + ny] == false && solution[nx*board_size + ny] != BLACK){
            reachable[nx*board_size + ny] = true;
            //count++;
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
	uint8_t j = 0;
	for(uint8_t i = 0; i < n; i++){
		if(reachable[i]){
			j++;
		}
	}
    free(reachable);
	//set_bkg_tile_xy(5,0,j/10);
	//set_bkg_tile_xy(6,0,j%10);
    //printf("%d\n",count);
	set_bkg_tile_xy(0,8,0);
    return j;
}