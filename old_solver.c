void process_input();
void solve();
void check_corners();
void check_row_triples();
void check_col_triples();
void check_unique();
void check_blocked_neighbors();
bool validate_solution();


void solve(){
	for(uint8_t a = 0; a < 3; a++){
		check_corners();
		
		check_row_triples();
		
		check_col_triples();
		
		check_unique();
		
		check_blocked_neighbors();
	}

	for(uint8_t i = 0; i < BOARD_SIZE; i++){
		for(uint8_t j = 0; j < BOARD_SIZE; j++){
			if(solution[i][j] == 1){
				set_bkg_tile_xy(i+7,j+6,0);
			}else if(solution[i][j] == 2){
				set_bkg_tile_xy(i+7,j+6,board[i][j]+8);
			}
		}
	}
}

void generate_board(){
	for(uint8_t i = 0; i < BOARD_SIZE; i++){
		for(uint8_t j = 0; j < BOARD_SIZE; j++){
			marks[i][j] = 0;
			solution[i][j] = 0;
			board[i][j] = (abs(arand()) % BOARD_SIZE)+1;
			set_bkg_tile_xy(i+7,j+6,board[i][j]);
		}
	}
	solve();
	if(!validate_solution()){ generate_board();}
}

bool validate_solution(){
	uint8_t freq[BOARD_SIZE];
	for(uint8_t a = 0; a < BOARD_SIZE; a++){freq[a]=0;}
	for(uint8_t i = 0; i < BOARD_SIZE; i++){
		for(uint8_t j = 0; j < BOARD_SIZE; j++){
			if(solution[i][j]==1){
				continue;
			}else if(freq[j]++){
				return false;
			}
		}
		for(uint8_t a = 0; a < BOARD_SIZE; a++){freq[a]=0;}
	}
	
	for(uint8_t j = 0; j < BOARD_SIZE; j++){
		for(uint8_t i = 0; i < BOARD_SIZE; i++){
			if(solution[i][j]==1){
				continue;
			}else if(freq[i]++){
				return false;
			}
		}
		for(uint8_t a = 0; a < BOARD_SIZE; a++){freq[a]=0;}
	}
	return true;
}

void check_corners(){
	if(board[0][0] == board[1][0]){ // top left
		if(board[0][0] == board[0][1]){
			solution[0][0] = 1;
		}else{
			solution[0][1] = 2;
		}
	}
	if(board[BOARD_SIZE-1][0] == board[BOARD_SIZE-2][0]){ // top right
		if(board[BOARD_SIZE-1][0] == board[BOARD_SIZE-1][1]){
			solution[BOARD_SIZE-1][0] = 1;
		}else{
			solution[BOARD_SIZE-1][1] = 2;
		}
	}
	if(board[0][BOARD_SIZE-1] == board[1][BOARD_SIZE-1]){ // bottom left
		if(board[0][BOARD_SIZE-1] == board[0][BOARD_SIZE-2]){
			solution[0][BOARD_SIZE-1] = 1;
		}else{
			solution[0][BOARD_SIZE-2] = 2;
		}
	}
	if(board[BOARD_SIZE-1][BOARD_SIZE-1] == board[BOARD_SIZE-2][BOARD_SIZE-1]){ // bottom right
		if(board[BOARD_SIZE-1][BOARD_SIZE-1] == board[BOARD_SIZE-1][BOARD_SIZE-2]){
			solution[BOARD_SIZE-1][BOARD_SIZE-1] = 2;
		}else{
			solution[BOARD_SIZE-1][BOARD_SIZE-2] = 2;
		}
	}
	
	if(board[0][1] == board[1][1]){ solution[1][0] = 2;}
	if(board[BOARD_SIZE-1][1] == board[BOARD_SIZE-2][1]){ solution[BOARD_SIZE-2][0] = 2;}
	if(board[0][BOARD_SIZE-2] == board[1][BOARD_SIZE-2]){ solution[1][BOARD_SIZE-1] = 2;}
	if(board[BOARD_SIZE-1][BOARD_SIZE-2] == board[BOARD_SIZE-2][BOARD_SIZE-2]){ solution[BOARD_SIZE-2][BOARD_SIZE-1] = 2;} 

}
void check_row_triples(){
	for(uint8_t j = 0; j < BOARD_SIZE-2; j++){
		for(uint8_t i = 0; i < BOARD_SIZE; i++){
			if((board[i][j]==board[i+2][j])&&(board[i][j]!=board[i+1][j])){
				solution[i+1][j] = 2;
			}
		}
	}
}
void check_col_triples(){
	for(uint8_t i = 0; i < BOARD_SIZE; i++){
		for(uint8_t j = 0; j < BOARD_SIZE-2; j++){
			if((board[i][j]==board[i][j+2])&&(board[i][j]!=board[i][j+1])){
				solution[i][j+1] = 2;
			}
		}
	}
}

void check_unique(){ //TODO: this is basically a war crime and will ruin my career prospects
	uint8_t dupe = 0;
	for(uint8_t j = 0; j < BOARD_SIZE; j++){
		for(uint8_t i = 0; i < BOARD_SIZE; i++){
			for(uint8_t a = 0; a < BOARD_SIZE; a++){
				if((a != i) && (board[a][j] == board[i][j]) && (solution[a][j] != 1)){
					dupe++;
					if(solution[a][j] == 2){solution[i][j] = 1;}
					
				}
				if((a != j) && (board[i][a] == board[i][j]) && (solution[i][a] != 1)){
					dupe++;
					if(solution[i][a] == 2){solution[i][j] = 1;}
					
				}
			}
			if(!dupe){solution[i][j] = 2;}
			dupe = 0;
		}
	}
}

void check_blocked_neighbors(){
	for(uint8_t j = 0; j < BOARD_SIZE; j++){
		for(uint8_t i = 0; i < BOARD_SIZE; i++){
			if(solution[i][j] == 1){
				if(i!=0){solution[i-1][j] = 2;}
				if(i!=(BOARD_SIZE-1)){solution[i+1][j] = 2;}
				if(j!=0){solution[i][j-1] = 2;}
				if(j!=(BOARD_SIZE-1)){solution[i][j+1] = 2;}
			}
		}
	}
}