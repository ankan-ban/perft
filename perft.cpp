// a simple 0x88 move generater / perft tool


#include "chess.h"

// routines to make a move on the board and to undo it
void makeMove(BoardPosition *pos, Move move)
{

    pos->board[move.dst] = pos->board[move.src];
    pos->board[move.src] = EMPTY_SQUARE;

    uint32 chance = pos->chance;

    if (move.flags)
    {
        // special  moves

        // 1. Castling: update the rook position too
        if(move.flags == CASTLE_FLAG_KING_SIDE)
        {
            if (chance == BLACK)
            {
				pos->board[0x77] = EMPTY_SQUARE; 
                pos->board[0x75] = COLOR_PIECE(BLACK, ROOK);
            }
            else
            {
                pos->board[0x07] = EMPTY_SQUARE; 
                pos->board[0x05] = COLOR_PIECE(WHITE, ROOK);
            }
                        
        }
        else if (move.flags == CASTLE_QUEEN_SIDE)
        {
            if (chance == BLACK)
            {
                pos->board[0x70] = EMPTY_SQUARE; 
                pos->board[0x73] = COLOR_PIECE(BLACK, ROOK);
            }
            else
            {
                pos->board[0x00] = EMPTY_SQUARE; 
                pos->board[0x03] = COLOR_PIECE(WHITE, ROOK);
            }
        }

        // 2. en-passent: clear the captured piece
        else if (move.flags == EN_PASSENT)
        {
            pos->board[INDEX088(RANK(move.src), pos->enPassent - 1)] = EMPTY_SQUARE;
        }

        // 3. promotion: update the pawn to promoted piece
        else if (move.flags == PROMOTION_QUEEN)
        {
            pos->board[move.dst] = COLOR_PIECE(chance, QUEEN);
        }
        else if (move.flags == PROMOTION_ROOK)
        {
            pos->board[move.dst] = COLOR_PIECE(chance, ROOK);
        }
        else if (move.flags == PROMOTION_KNIGHT)
        {
            pos->board[move.dst] = COLOR_PIECE(chance, KNIGHT);
        }
        else if (move.flags == PROMOTION_BISHOP)
        {
            pos->board[move.dst] = COLOR_PIECE(chance, BISHOP);
        }
    }

    // update game state variables
    uint8 piece = PIECE(pos->board[move.dst]);
    pos->enPassent = 0;

    if (piece == KING)
    {
        if (chance == BLACK)
        {
            pos->blackCastle = 0;
        }
        else
        {
            pos->whiteCastle = 0;
        }
    }
    else if (piece == ROOK)
    {
        uint32 castleSide = (FILE(move.src) & 1) + 1;   // warning: hack
        uint32 castleMask = BIT(castleSide);
        if (chance == BLACK)
        {
            pos->blackCastle &= ~castleMask;
        }
        else
        {
            pos->whiteCastle &= ~castleMask;
        }
    }
    else if ((piece == PAWN) && (abs(RANK(move.dst) - RANK(move.src)) == 2))
    {
        pos->enPassent = FILE(move.src);
    }

    // flip the chance
    pos->chance = !chance;
}

// this has a bug - doesn't handle special moves like castling, etc correctly!
void undoMove(BoardPosition *pos, Move move, uint8 bc, uint8 wc, uint8 enPassent)
{
    pos->board[move.src] = pos->board[move.dst];    
    pos->board[move.dst] = move.capturedPiece;

    pos->blackCastle = bc;
    pos->whiteCastle = wc;
    pos->enPassent = enPassent;
    pos->chance = !pos->chance;
}


// perft search
template <class Generator>
uint64 perft(BoardPosition *pos, int depth)
{
    // TODO: check if keeping this local variable is ok
    Move moves[MAX_MOVES];
    uint64 childPerft = 0;


    uint32 nMoves = Generator::generateMoves(pos, moves);

    if (depth == 1)
    {
        return nMoves;
    }

    for (uint32 i = 0; i < nMoves; i++)
    {
        //uint8 blackCastle = pos->blackCastle;
        //uint8 whiteCastle = pos->whiteCastle;
        //uint8 enPassent   = pos->enPassent;
		BoardPosition newPos = *pos;
        makeMove(&newPos, moves[i]);
        childPerft += perft<Generator>(&newPos, depth - 1);
        //undoMove(pos, moves[i], blackCastle, whiteCastle, enPassent);
    }
    return childPerft;
}

// for timing CPU code : start
double gTime;
#define START_TIMER { \
    LARGE_INTEGER count1, count2, freq; \
    QueryPerformanceFrequency (&freq);  \
    QueryPerformanceCounter(&count1);

#define STOP_TIMER \
    QueryPerformanceCounter(&count2); \
    gTime = ((double)(count2.QuadPart-count1.QuadPart)*1000.0)/freq.QuadPart; \
    }
// for timing CPU code : end


int main()
{
    BoardPosition testBoard;
    //Utils::readFENString("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", &testBoard); // start.. 20 positions
    //Utils::readFENString("3Q4/1Q4Q1/4Q3/2Q4R/Q4Q2/3Q4/1Q4Rp/1K1BBNNk w - - 0 1", &testBoard); // - 218 positions.. correct!
    //Utils::readFENString("8/8/8/4Q3/8/8/8/8 w - - 0 1", &testBoard);
    Utils::readFENString("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", &testBoard);   // - 48 positions.. correct!
    // the above position exposes a bug starting n = 2

    Utils::dispBoard(&testBoard);

    //Move moves[MAX_MOVES];
    //uint32 nMoves = MoveGenerator::generateMoves(&testBoard, moves);
    //printf("\nMoves generated: %d\n", nMoves);


	MoveGeneratorLUT::init();
    //nMoves = MoveGeneratorLUT::generateMoves(&testBoard, moves);
    //printf("\nMoves generated by LUT: %d\n", nMoves);

	
    int depth = 5;
    uint64 leafNodes;
    START_TIMER
    leafNodes = perft<MoveGenerator>(&testBoard, depth);
    STOP_TIMER
    printf("\n088 Move Generator: ");
    printf("\nPerft %d: %llu", depth, leafNodes);
    printf("\nTime taken: %g seconds, nps: %llu\n", gTime/1000.0, (uint64) ((leafNodes/gTime)*1000.0));

    START_TIMER
    leafNodes = perft<MoveGeneratorLUT>(&testBoard, depth);
    STOP_TIMER
    printf("\nLookup table based Move Generator: ");
    printf("\nPerft %d: %llu", depth, leafNodes);
    printf("\nTime taken: %g seconds, nps: %llu\n", gTime/1000.0, (uint64) ((leafNodes/gTime)*1000.0));


    return 0;
}