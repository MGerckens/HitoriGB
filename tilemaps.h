
#ifndef TILEMAPS_H
#define TILEMAPS_H

#define board_layoutWidth 21
#define board_layoutHeight 19
#define board_layoutBank 0

extern const unsigned char boardLayout[];

#define title_layoutWidth 20
#define title_layoutHeight 18
#define title_layoutBank 0

extern const unsigned char titleLayout[];

#define title_tilesBank 0
extern const unsigned char titleTiles[];

#define board_tilesBank 0
extern const unsigned char boardTiles[];

#define cursor_tileBank 0
extern const unsigned char cursorTiles[];

#endif  // TILEMAPS_H