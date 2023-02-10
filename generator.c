#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <rand.h>
#include <gb/gb.h>

#include "generator.h"
#include "latinsquare.h"

typedef struct Face{
    uint8_t val; //not really needed but makes debugging way easier
    struct Face *parent;
} face_t;

const face_t infinite_face = {0, &infinite_face}; //'infinite' face representing the outside of the board

//SDCC doesn't allow returning structs from functions and chaining function pointers together is slow
uint8_t neighbors[8]; //return values of neighbors
face_t *incident_faces[4]; //return value of get_incident_faces
face_t *faces; //stores faces between vertices
uint8_t *randlist_order, *randlist_dupes, *colnums, *rownums;

void init_faces();
void blacken_square(uint8_t coord);
void get_neighbors(uint8_t x, uint8_t y);
uint8_t get_incident_faces(uint8_t x, uint8_t y);
face_t *get_face(uint8_t x, uint8_t y);

face_t *rep(face_t *self){ //returns root of face set
	face_t *grandparent = NULL;
    while(self != self->parent){
		grandparent = self->parent->parent;
        self->parent = grandparent;
        self = grandparent;
		
    }
	return self;
}

inline void merge(face_t *self, face_t *other){ //combines two faces
    rep(other)->parent = rep(self);
}

void init_faces(){ //set up solution and faces
	faces = malloc((num_tiles - 2*board_size + 1)*sizeof(face_t)); //allocates (board_size-1)^2, since (x-1)^2 = x^2-2x+1
    uint8_t i,j;
    for(i = 0; i < board_size; i++){
        for(j = 0; j < board_size; j++){
            solution[j*board_size+i] = 0;
        }
    }
    for(i = 0; i < board_size-1; i++){
        for(j = 0; j < board_size-1; j++){
            faces[j*(board_size-1)+i].val = j*(board_size-1)+i+1;
            faces[j*(board_size-1)+i].parent = &faces[j*(board_size-1)+i];
        }
    }
}

void get_neighbors(uint8_t x, uint8_t y){ //return coords of neighbors, or 255 if not on board
    if(x > 0){
        neighbors[0] = x-1; neighbors[1] = y;
    }else{
        neighbors[0] = 255; neighbors[1] = 255;
    }
    if(y > 0){
        neighbors[2] = x; neighbors[3] = y-1;
    }else{
        neighbors[2] = 255; neighbors[3] = 255;
    }
    if(x < board_size-1){
        neighbors[4] = x+1; neighbors[5] = y;
    }else{
        neighbors[4] = 255; neighbors[5] = 255;
    }
    if(y < board_size-1){
        neighbors[6] = x; neighbors[7] = y+1;
    }else{
        neighbors[6] = 255; neighbors[7] = 255;
    }
}

uint8_t get_incident_faces(uint8_t x, uint8_t y){ //return all neigboring faces, and count how many are unique
    incident_faces[0]=get_face(x-1,y-1);
	uint8_t count = 1;
	
	incident_faces[1]=get_face(x-1,y);
	if(incident_faces[1] != incident_faces[0]){count++;}
		
	incident_faces[2]=get_face(x,y-1);
	
	if(incident_faces[2] != incident_faces[1] && incident_faces[2] != incident_faces[0]){count++;}
	
    incident_faces[3]=get_face(x,y);
	if(incident_faces[3] != incident_faces[2] && incident_faces[3] != incident_faces[1] && incident_faces[3] != incident_faces[0]){count++;}
	
	return count;
}

face_t * get_face(uint8_t x, uint8_t y){ //return face at coordinate with bounds checking
    if(x<board_size-1 && y<board_size-1){ 
		return rep(&faces[y*(board_size-1)+x]);
	}
    else{return &infinite_face;}
}

void blacken_square(uint8_t coord){
    uint8_t x = coord/board_size;
	uint8_t y = coord%board_size;

	if(solution[y*board_size+x]){return;}
    uint8_t i;

    get_neighbors(x,y); //gets neighbor vertices and stores coords in neighbors

    for(i = 0; i < 8; i+=2){
        if(neighbors[i] != 255 && solution[neighbors[i+1]*board_size + neighbors[i]]){return;} //stop if any neighbor is already black
    }

    uint8_t delta_E = 0;
    for(i = 0; i < 8; i+=2){
        if(neighbors[i] != 255){delta_E++;} //count neigboring edges
    }

	uint8_t delta_F = get_incident_faces(x,y); //count neigboring faces
	
    if(delta_E - delta_F){return;} //fail if graph is not fully connected 
    solution[y*board_size+x] = 1; //if it is still connected, mark the square as blackened 

    uint8_t index = 0; //initialize index to stop compiler warning, this value is always overwritten
	bool found = false;
	for(int8_t i = 0; i < 4; i++){
        if(incident_faces[i] == &infinite_face){ //check if any of the neighboring faces are merged with the infinite face
            index = i;
			found = true;
        }
    }
	if(!found){index=0;} //if none are, merge to top-left by default. Any direction works as long as it's consistent

    for(i = 0; i < 4; i++){ 
        merge(incident_faces[index], incident_faces[i]);
    }
}

uint8_t best_duplicate(uint8_t coord){ //picks duplicate number to assign to black tile
	uint8_t i,j, temp, randval, x=coord%board_size, y=coord/board_size;
	for(i = 0; i < board_size; i++){randlist_dupes[i] = i+1;} //generate list of numbers from 1 to board_size
	for(i = 0; i < board_size; i++){ //shuffle array
		randval = (uint8_t)arand() % board_size;
		temp = randlist_dupes[i];
		randlist_dupes[i] = randlist_dupes[randval];
		randlist_dupes[randval] = temp;
	}
	for(i = 0; i < board_size; i++){ //prioritize numbers that appear once in the row and column, to minimize latin square characteristics
		j = randlist_dupes[i];
		if(rownums[y*board_size + j-1] == 1 && colnums[x*board_size + j-1] == 1){goto found;}
	}
	for(i = 0; i < board_size; i++){ //if none are found, use the first duplicate 
		j = randlist_dupes[i];
		if(rownums[y*board_size + j-1] || colnums[x*board_size + j-1]){goto found;}
	}
	return 0; //should not happen
	
found:
	rownums[y*board_size + j-1]++; //update counters with newly assigned number
	colnums[x*board_size + j-1]++;
	return j; 
}


void generate_board(){
	init_faces();
    uint8_t i,j,x,y, randval, temp;
	randlist_order = (uint8_t *)malloc(num_tiles * sizeof(uint8_t));
	randlist_dupes = (uint8_t *)malloc(board_size * sizeof(uint8_t));
	colnums = (uint8_t *)calloc(num_tiles, sizeof(uint8_t));
	rownums = (uint8_t *)calloc(num_tiles, sizeof(uint8_t));
	for(i = 0; i < num_tiles; i++){  //generate list of numbers from 1 to num_tiles
		randlist_order[i] = i;
	}
	for(i = 0; i < num_tiles; i++){ //shuffle array
		randval = (uint8_t)arand() % num_tiles; 
		temp = randlist_order[i];
		randlist_order[i] = randlist_order[randval];
		randlist_order[randval] = temp;
	}
	for(i = 0; i < num_tiles; i++){ //attempt to blacken all squares in random order
		blacken_square(randlist_order[i]);
	}

	latin_generate();

	for(i = 0; i < num_tiles; i++){ //count number of times each number appears in each row and column
		if(solution[i]){continue;} //exclude black tiles
		j = board[i];
		x = i%board_size;
		y = i/board_size;
		rownums[y*board_size + j-1]++;
		colnums[x*board_size + j-1]++;
	}
	for(i = 0; i < num_tiles; i++){ //replace blackened squares with duplicate numbers
		if(solution[i]){board[i] = best_duplicate(i);}
	}
	free(colnums);
	free(rownums);
	free(randlist_dupes);
	free(randlist_order);
	free(faces);
}
