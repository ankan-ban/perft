#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <assert.h>
#include <windows.h>

typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
typedef unsigned long long uint64;

#define BIT(i)   (1 << (i))

// Terminology:
//
// file - column [A - H]
// rank - row    [1 - 8]


// piece constants 
#define PAWN    1
#define KNIGHT  2
#define BISHOP  3
#define ROOK    4
#define QUEEN   5
#define KING    6

// chance (side) constants
#define WHITE   0
#define BLACK   1

// bits 012 - piece
// bit    4 - color
#define COLOR_PIECE(color, piece)   ((color << 4) | (piece))
#define COLOR(colorpiece)           (colorpiece >> 4)
#define PIECE(colorpiece)           (colorpiece & 7)

#define INDEX088(rank, file)        ((rank) << 4 | (file))
#define RANK(index088)              (index088 >> 4)
#define FILE(index088)              (index088 & 0xF)

#define ISVALIDPOS(index088)        (((index088) & 0x88) == 0)

// special move flags
#define CASTLE_QUEEN_SIDE  1
#define CASTLE_KING_SIDE   2
#define EN_PASSENT         3
#define PROMOTION_QUEEN    4
#define PROMOTION_ROOK     5
#define PROMOTION_BISHOP   6
#define PROMOTION_KNIGHT   7

// castle flags in board position (2 and 4)
#define CASTLE_FLAG_KING_SIDE   BIT(CASTLE_KING_SIDE)
#define CASTLE_FLAG_QUEEN_SIDE  BIT(CASTLE_QUEEN_SIDE)

// size 128 bytes
// let's hope this fits in register file
struct BoardPosition
{
    union
    {
        uint8 board[128];    // the 0x88 board

        struct
        {
            uint8 row0[8];  uint8 padding0[4];

            uint8 chance;        // whose move it is
            uint8 whiteCastle;   // whether white can castle
            uint8 blackCastle;   // whether black can castle
            uint8 enPassent;     // col + 1 (where col is the file on which enpassent is possible)

            uint8 row1[8]; uint8 padding1[8];
            uint8 row2[8]; uint8 padding2[8];
            uint8 row3[8]; uint8 padding3[8];
            uint8 row4[8]; uint8 padding4[8];
            uint8 row5[8]; uint8 padding5[8];
            uint8 row6[8]; uint8 padding6[8];
            uint8 row7[8]; uint8 padding7[8];

            // 60 unused bytes (padding) available for storing other structures if needed
        };
    };
};

/*
       The board representation     Free space for other structures

	A	B	C	D	E	F	G	H
8	70	71	72	73	74	75	76	77	78	79	7A	7B	7C	7D	7E	7F
7	60	61	62	63	64	65	66	67	68	69	6A	6B	6C	6D	6E	6F
6	50	51	52	53	54	55	56	57	58	59	5A	5B	5C	5D	5E	5F
5	40	41	42	43	44	45	46	47	48	49	4A	4B	4C	4D	4E	4F
4	30	31	32	33	34	35	36	37	38	39	3A	3B	3C	3D	3E	3F
3	20	21	22	23	24	25	26	27	28	29	2A	2B	2C	2D	2E	2F
2	10	11	12	13	14	15	16	17	18	19	1A	1B	1C	1D	1E	1F
1	00	01	02	03	04	05	06	07	08	09	0A	0B	0C	0D	0E	0F

*/


// size 4 bytes
struct Move
{
    uint8  src;             // source position of the piece
    uint8  dst;             // destination position
    uint8  capturedPiece;   // the piece captured (if any)
    uint8  flags;           // flags to indicate special moves, e.g castling, en passent, promotion, etc
};








/** Declarations for class/methods in Util.cpp **/

// utility functions for reading FEN String, EPD file, displaying board, etc

/*
   Three kinds of board representations are used with varying degrees of readibility and efficiency

   1. Human readable board (EPD file?)
 
	  e.g. for Starting Position:

	  rnbqkbnr
	  pppppppp
	  ........
	  ........
	  ........
	  ........
	  PPPPPPPP
	  RNBQKBNR

  2. FEN string board e.g: 
     "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

  3. 0x88 binary board described at the begining of this file

  The first two fromats are only used for displaying/taking input and interacting with the UCI protocol
  For all other purposes the 0x88 board is used

*/

class Utils {

private:

	// gets the numeric code of the piece represented by a character
	static uint8 getPieceCode(char piece); 

	// Gets char representation of a piece code
	static char getPieceChar(uint8 code);

public:	

	// reads a board from text file
	static void readBoardFromFile(char filename[], char board[8][8]); 
    static void readBoardFromFile(char filename[], BoardPosition *pos);


	// displays the board in human readable form
	static void dispBoard(char board[8][8]); 
    static void dispBoard(BoardPosition *pos);

    // displays a move in human readable form
    static void displayMove(Move move);

    static void board088ToChar(char board[8][8], BoardPosition *pos);
    static void boardCharTo088(BoardPosition *pos, char board[8][8]);


	// reads a FEN string and sets board and other Game Data accorodingly
	static void readFENString(char fen[], BoardPosition *pos);

};