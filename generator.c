//very loosely adapted from Simon Tatham's version
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "generator.h"
#include "latinsquare.h"
#include "queues.h"
#include <rand.h>
#include <gb/gb.h>

uint8_t num,shade_count;
enum {EMPTY, BLACK, CIRCLE};
bool impossible = false;

#define INGRID(s,x,y) ((x) >= 0 && (x) < (s) && (y) >= 0 && (y) < (s))
static bool blacken_square(uint8_t x, uint8_t y, bool initial);
static void circle_square(uint8_t x, uint8_t y);
static bool has_single_white_region();
static void remove_splits();
static void surrounded_edges();
static uint8_t fill(uint8_t xi, uint8_t yi);

void generate_board()
{
    uint8_t i, j, coord;
    uint8_t n = board_size * board_size;
generate:
	set_bkg_tile_xy(0,3,0);
	set_bkg_tile_xy(0,4,0);
	set_bkg_tile_xy(0,5,0);
	set_bkg_tile_xy(0,6,0);
	set_bkg_tile_xy(0,7,0);
	set_bkg_tile_xy(0,8,0);
	//set_bkg_tile_xy(0,9,0);
    shade_count = 0;
    num = 0;
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
        i = arand() % (board_size*board_size);
        if ((solution[i] == CIRCLE) || (solution[i] == BLACK)) {
            //printf("generator skipping (%d,%d): %s\n", i%board_size, i/board_size,
            //      (solution[i] == CIRCLE) ? "CIRCLE" : "BLACK");
            continue; /* solver knows this must be one or the other already. */
        }

        /* Add a random black cell... */
        if(!blacken_square(i/board_size, i%board_size, true)){continue;}
		set_bkg_tile_xy(0,4,1);
		
        remove_splits();
        //set_bkg_tile_xy(0,5,1);
        if (impossible) {
            //printf("generator made impossible, restarting...\n");
            goto generate;
        }
        //printBoard();
        //printf("num = %d, starting new square\n",num);
    }
    //printf("starting end check\n");
    if(num < n){
		set_bkg_tile_xy(0,6,1);
        for(i = 0; i < n; i++){
            if(solution[i] == EMPTY){
                if(!blacken_square(i/board_size, i%board_size, true)){
					circle_square(i/board_size, i%board_size);
				}
                //remove_splits();
            }
            if (impossible) {
                //printf("generator made impossible, restarting...\n");
                goto generate;
            }
        }
    }
	if(!has_single_white_region){goto generate;}
	set_bkg_tile_xy(0,9,1);
    //printBoard();
    
    uint8_t randnum, dupe, flag = 0;
    for(i = 0; i < board_size; i++){
		set_bkg_tile_xy(0,7,flag);
		flag^=1;
        for (j = 0; j < board_size; ++j) {
            if(solution[i*board_size + j] != BLACK){continue;}
            while(true){
                randnum = arand();
                if(randnum & 0x80){
                    randnum %= board_size;
                    coord = randnum*board_size + j;
                }else{
                    randnum %= board_size;
                    coord = i*board_size + randnum;
                }
                dupe = board[coord];
                if(solution[coord] != BLACK){break;}
            }
            board[i*board_size + j] = dupe;
        }
    }
	//surrounded_edges();
}

#define CHECK(x) ((x==1) ? 1 : 0)

//static void surrounded(uint8_t x, uint8_t y){

static void surrounded_edges(){ 
	uint8_t top = 0x07,bottom = 0x07,left = 0x07,right = 0x07;
	uint8_t bottom_offset;
	for(uint8_t i = 0; i < board_size; i++){
		bottom_offset = i*board_size;
		if(i){
			top &=~ CHECK(solution[i-1]) << 2;
			bottom &=~ CHECK(solution[(board_size-1)*board_size + i-1]) << 2;
			left &=~ CHECK(solution[(i-1)*board_size]) << 2;
			right &=~ CHECK(solution[(i-1)*board_size + board_size]) << 2;
		}else{
			top &=~ 0x04;
			bottom &=~ 0x04;
			left &=~ 0x04;
			right &=~ 0x04;
		}
		if(i!=board_size-1){
			top &=~ CHECK(solution[i+1]) << 1;
			bottom &=~ CHECK(solution[(board_size-1)*board_size + i+1]) << 1;
			left &=~ CHECK(solution[(i+1)*board_size]) << 1;
			right &=~ CHECK(solution[(i+1)*board_size + board_size]) << 1;
		}else{
			top &=~ 0x02;
			bottom &=~ 0x02;
			left &=~ 0x02;
			right &=~ 0x02;
		}
		top &=~ CHECK(solution[i+board_size]);
		bottom &=~ CHECK(solution[(board_size-2)*board_size + i]);
		left &=~ CHECK(solution[i*board_size + 1]);
		right &=~ CHECK(solution[i*board_size -1 ]);
	}
	if(!top || !bottom || !left || !right){set_bkg_tile_xy(8,1,1);}else{set_bkg_tile_xy(8,1,0);}
}

static void remove_splits(){
	if(shade_count <= 1){return;}
	set_bkg_tile_xy(0,7,1);
    uint8_t temp, coord;
    if(!has_single_white_region()){
        //printf("already split\n");
        impossible = true;
		set_bkg_tile_xy(0,7,0);
        return;
    }
    for(uint8_t x = 0; x < board_size; x++) {
        for(uint8_t y = 0; y < board_size; y++) {
            coord = x*board_size + y;
            if(solution[coord] != EMPTY){continue;}
            temp = solution[coord];
            solution[coord] = BLACK;
            if(!has_single_white_region()){
                solution[coord] = temp;
                circle_square(x,y);
                continue;
            }
            solution[coord] = temp;
        }
    }
	set_bkg_tile_xy(0,7,0);
}

static bool has_single_white_region(){
	uint8_t sc=0;
	for(uint8_t i = 0; i < board_size*board_size; i++){
		if(solution[i] == BLACK){sc++;}
	}
	if(sc <= 1){return true;}
    uint8_t whitespace_count = (board_size * board_size) - sc;
	set_bkg_tile_xy(5,1,whitespace_count/10);
	set_bkg_tile_xy(6,1,whitespace_count%10);
    uint8_t fillres;
    if(solution[0] != BLACK){
        fillres = fill(0,0);
    }else{
        fillres = fill(0,1);
    }
    //printf("%d >= %d\n",fillres,whitespace_count);
	set_bkg_tile_xy(0,9,fillres >= whitespace_count);
    return fillres >= whitespace_count;
}

static bool blacken_square(uint8_t x, uint8_t y, bool initial){ //only returns false if an initial placement step fails
    if(solution[x*board_size + y] == CIRCLE){ goto fail; }
    else if(solution[x*board_size + y] == EMPTY) {
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
            if (solution[(x - 1) * board_size + y] == BLACK) { goto fail;}
        }
        if (y_low) {
            if (solution[x * board_size + (y - 1)] == BLACK) { goto fail; }
        }
        if (x_high) {
            if (solution[(x + 1) * board_size + y] == BLACK) { goto fail; }
        }
        if (y_high) {
            if (solution[x * board_size + (y + 1)] == BLACK) { goto fail; }
        }
		//set_bkg_tile_xy(x,y,0);
        solution[x * board_size + y] = BLACK;
		if(!has_single_white_region()){goto fail;}
        shade_count++;
        num++;

        if (x_low) {
            circle_square(x - 1, y);
        }
        if (y_low) {
            circle_square(x, y - 1);
        }
        if (x_high) {
            circle_square(x + 1, y);
        }
        if (y_high) {
            circle_square(x, y + 1);
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

static void circle_square(uint8_t x, uint8_t y){
    if(!(INGRID(board_size,x,y))){return;}
    if(solution[x*board_size + y] == BLACK) {
        impossible = true;
        //printf("impossible to circle %d, %d\n",x,y);
        return;
    }
    else if(solution[x*board_size + y] == EMPTY) {
        //printf("circling %d, %d\n",x,y);
        num++;
        solution[x*board_size + y] = CIRCLE;
        for (uint8_t xt = 0; xt < board_size; xt++) {
            if(xt != x && board[xt*board_size + y]==board[x*board_size + y]){
                blacken_square(xt,y, false);
            }
        }
        for (uint8_t yt = 0; yt < board_size; yt++) {
            if(yt != y && board[x*board_size + yt]==board[x*board_size + y]){
                blacken_square(x,yt, false);
            }
        }
    }
}


static uint8_t fill(uint8_t xi, uint8_t yi){ //floodfill using two queues
    
    if(solution[xi*board_size + yi] == BLACK){return 0;}
	//if(!shade_count){return board_size*board_size;}
	//if(shade_count == 1){return board_size*board_size - 1;}
	//set_bkg_tile_xy(5,1,shade_count);
    //printf("started fill\n");
    bool *reachable = (bool *)calloc(board_size*board_size,sizeof(bool));
    node_t *Qx = NULL;			   //this version of SDCC doesn't support passing/returning structs
    node_t *Qy = NULL;			   //and this seemed easier than a queue of arrays
    //uint8_t count = 1;
    enqueue(&Qx, xi);
    enqueue(&Qy, yi);
    uint8_t nx, ny, flag = 0;

    while(Qx!=NULL && Qy!=NULL){
		set_bkg_tile_xy(0,8,flag);
		flag^=1;
        nx = dequeue(&Qx);
        ny = dequeue(&Qy);
		set_bkg_tile_xy(3,0,nx);
		set_bkg_tile_xy(4,0,ny);
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
	for(uint8_t i = 0; i < board_size*board_size; i++){
		if(reachable[i]){
			j++;
		}else{ 
			set_bkg_tile_xy(i/board_size + 7, i%board_size + 6, 0);
		}
	}
    free(reachable);
	set_bkg_tile_xy(5,0,j/10);
	set_bkg_tile_xy(6,0,j%10);
    //printf("%d\n",count);
    return j;
}