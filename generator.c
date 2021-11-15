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

const face_t infinite_face = {0, &infinite_face}; //face representing outside of board
uint8_t nb_ret[8]; //return values of neighbors
face_t *incident_ret[4]; //return value of get_incident_faces
face_t *faces; //stores faces between vertices
uint8_t *randlist_order, *randlist_dupes, *colnums, *rownums;

void init_faces();
bool blacken_square(uint8_t y, uint8_t x);
void neighbors(uint8_t x, uint8_t y);
uint8_t get_incident_faces(uint8_t x, uint8_t y);
face_t *get_face(uint8_t x, uint8_t y);
uint8_t pop();

uint8_t flag = 1;
face_t *rep(face_t *self){ //returns root of face
	uint8_t count = 0;
	face_t gp;
	face_t *grandparent = &gp;
    while(self != self->parent){
        grandparent = self->parent->parent;
        self->parent = grandparent;
        self = grandparent;
		count++;
    }
	return self;
}

inline void merge(face_t *self, face_t *other){ //combines two faces
    rep(other)->parent = rep(self);
}

void init_faces(){ //set up solution and faces
	faces = malloc((board_size-1)*(board_size-1)*sizeof(face_t)); //freed at the end of generation

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

inline bool blacken_square_coord(uint8_t coord){ //convert index coord to xy
    return blacken_square(coord/board_size, coord%board_size);
}

bool blacken_square(uint8_t x, uint8_t y){ //return value not used in generation, may be used later in solution checking idk
    if(solution[y*board_size+x]){return false;}
    uint8_t i;
    neighbors(x,y); //gets neighbor vertices and stores coords in nb_ret

    for(i = 0; i < 8; i+=2){
        if(nb_ret[i] != 255 && solution[nb_ret[i+1]*board_size + nb_ret[i]]){return false;} //stop if any neighbor is already black
    }

    uint8_t delta_E = 0;
    for(i = 0; i < 8; i+=2){
        if(nb_ret[i] != 255){delta_E++;} //count neigboring edges
    }
	
	uint8_t delta_F = get_incident_faces(x,y); //count neigboring faces

    if(delta_E - delta_F != 0){return false;} //check if graph is fully connected 
    solution[y*board_size+x] = 1;

    uint8_t index = pop();
    for(i = 0; i < 4; i++){ //merge all neighboring faces into lowest neighbor
        merge(incident_ret[index], incident_ret[i]);
    }

    return true;
}

void neighbors(uint8_t x, uint8_t y){ //return coords of neighbors, or 255 if not on board
    if(x > 0){
        nb_ret[0] = x-1;
        nb_ret[1] = y;
    }else{
        nb_ret[0] = 255;
        nb_ret[1] = 255;
    }
    if(y > 0){
        nb_ret[2] = x;
        nb_ret[3] = y-1;
    }else{
        nb_ret[2] = 255;
        nb_ret[3] = 255;
    }
    if(x < board_size-1){
        nb_ret[4] = x+1;
        nb_ret[5] = y;
    }else{
        nb_ret[4] = 255;
        nb_ret[5] = 255;
    }
    if(y < board_size-1){
        nb_ret[6] = x;
        nb_ret[7] = y+1;
    }else{
        nb_ret[6] = 255;
        nb_ret[7] = 255;
    }
}

uint8_t pop(){ //get index of lowest val neigboring face, only really matters if one is 0
    uint8_t min = 255;
	uint8_t retval = 0;
    for(int8_t i = 0; i < 4; i++){
        if(incident_ret[i]->val < min){
            min = incident_ret[i]->val;
			retval = i;
        }
    }
	return retval;
}


uint8_t get_incident_faces(uint8_t x, uint8_t y){ //return all neigboring faces, and count how many are unique
    incident_ret[0]=get_face(x-1,y-1);
	uint8_t count = 1;
	
	incident_ret[1]=get_face(x-1,y);
	if(incident_ret[1] != incident_ret[0]){count++;}
	
	incident_ret[2]=get_face(x,y-1);
	if(incident_ret[2] != incident_ret[1] && incident_ret[2] != incident_ret[0]){count++;}
	
    incident_ret[3]=get_face(x,y);
	if(incident_ret[3] != incident_ret[2] && incident_ret[3] != incident_ret[1] && incident_ret[3] != incident_ret[0]){count++;}

	return count;
}

face_t * get_face(uint8_t x, uint8_t y){ //return face at coordinate with bounds checking
    if(x<board_size-1 && y<board_size-1){ return rep(&faces[y*(board_size-1)+x]);}
    else{return &infinite_face;}
}


uint8_t best_duplicate(uint8_t coord){ //picks duplicate number to assign to black tile
	uint8_t i,j, temp, randval, x=coord%board_size, y=coord/board_size;
	for(i = 0; i < board_size; i++){randlist_dupes[i] = i+1;} //generate list of numbers from 1 to board_size
	for(i = board_size; i > 0; i--){ //shuffle array
		randval = (uint8_t)arand() % board_size;
		temp = randlist_dupes[i-1];
		randlist_dupes[i-1] = randlist_dupes[randval];
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
	randlist_order = (uint8_t *)malloc(num_tiles*sizeof(uint8_t));
	randlist_dupes = (uint8_t *)calloc(board_size,sizeof(uint8_t));
	colnums = (uint8_t *)calloc(num_tiles,sizeof(uint8_t));
	rownums = (uint8_t *)calloc(num_tiles,sizeof(uint8_t));
	
	for(i = num_tiles; i > 0; i--){  //generate list of numbers from 1 to board_size
		randlist_order[i] = i;
	}
	for(i = num_tiles; i > 0; i--){ //shuffle array
		randval = (uint8_t)arand() % board_size; 
		temp = randlist_order[i-1];
		randlist_order[i-1] = randlist_order[randval];
		randlist_order[randval] = temp;
	}
	for(i = num_tiles; i > 0; i--){ //attempt to blacken all squares in random order
		blacken_square_coord(randlist_order[i]);
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
