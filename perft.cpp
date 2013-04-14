// a simple 0x88 move generater / perft tool


#include "chess.h"

// max no of moves possible for a given board position (this can be as large as 218 ?)
// e.g, test this FEN string "3Q4/1Q4Q1/4Q3/2Q4R/Q4Q2/3Q4/1Q4Rp/1K1BBNNk w - - 0 1"
#define MAX_MOVES 256

// note that this is a pseudo-legal move generator
// Two issues:
// 1. for castling it's not checked if the squares are in check (major issue. need to be fixed)
// 2. it's not checked if a move puts the king in check (can probably avoid checking this)
class MoveGenerator
{
private:
    // TODO: check if passing these structures among functions is faster than making them global
    static BoardPosition *pos;
    static Move *moves;
    static uint32 nMoves;
    static uint32 chance;
    

    __forceinline static void addMove(uint32 src, uint32 dst, uint8 oldPiece, uint8 flags)
    {
        moves[nMoves].src = (uint8) src;
        moves[nMoves].dst = (uint8) dst;
        moves[nMoves].capturedPiece = (uint8) oldPiece;
        moves[nMoves].flags = flags;
        nMoves++;
    }

    __forceinline static void addPromotions(uint32 src, uint32 dst, uint8 oldPiece)
    {
        addMove(src, dst, oldPiece, PROMOTION_QUEEN);
        addMove(src, dst, oldPiece, PROMOTION_KNIGHT);
        addMove(src, dst, oldPiece, PROMOTION_ROOK);
        addMove(src, dst, oldPiece, PROMOTION_BISHOP);        
    }

    __forceinline static void generatePawnMoves(uint32 curPos)
    {
        uint32 finalRank = chance ?  0 : 7;
        uint32 curRank   = RANK(curPos);

        // pawn advancement
        
        // single square forward
        uint32 offset  = chance ? -16 : 16;
        uint32 newPos  = curPos + offset;
        uint32 newRank = RANK(newPos);

        if (ISVALIDPOS(newPos) && (pos->board[newPos] == 0))
        {
            if(newRank == finalRank)
            {
                // promotion
                addPromotions(curPos, newPos, 0);
            }
            else
            {
                addMove(curPos, newPos, 0, 0);

                // two squares forward
                uint32 startRank = chance ?  6 : 1;
                if (curRank == startRank)
                {
                    newPos += offset;
                    if (pos->board[newPos] == 0)
                        addMove(curPos, newPos, 0, 0);
                }
            }
        }

        // captures
        offset = chance ? -15 : 15;
        newPos = curPos + offset;
        uint32 capturedPiece = pos->board[newPos];
        if (ISVALIDPOS(newPos) && (capturedPiece) && (COLOR(capturedPiece) != chance))
        {
            if(newRank == finalRank)
            {
                // promotion
                addPromotions(curPos, newPos, capturedPiece);
            }
            else
            {
                addMove(curPos, newPos, capturedPiece, 0);
            }
        }

        offset = chance ? -17 : 17;
        newPos = curPos + offset;
        capturedPiece = pos->board[newPos];
        if (ISVALIDPOS(newPos) && (capturedPiece) && (COLOR(capturedPiece) != chance))
        {
            if(newRank == finalRank)
            {
                // promotion
                addPromotions(curPos, newPos, capturedPiece);
            }
            else
            {
                addMove(curPos, newPos, capturedPiece, 0);
            }
        }

        // En-passent
        if (pos->enPassent)
        {
            uint32 enPassentFile = pos->enPassent - 1;
            uint32 enPassentRank = chance ? 4 : 3;
            if ((curRank == enPassentRank) && (abs(FILE(curPos) - enPassentFile) == 1))
            {
                uint32 finalRank = chance ? 5 : 2;
                newPos = INDEX088(finalRank, enPassentFile);
                addMove(curPos, newPos, COLOR_PIECE(!chance, PAWN), EN_PASSENT);
            }
        }
    }

    __forceinline static void generateOffsetedMove(uint32 curPos, uint32 offset)
    {
        uint32 newPos = curPos + offset;
        if(ISVALIDPOS(newPos))
        {
            uint8 capturedPiece = pos->board[newPos];
            if (capturedPiece == 0 || COLOR(capturedPiece) != chance)
                addMove(curPos, newPos, capturedPiece, 0);
        }
    }

    __forceinline static void generateOffsetedMoves(uint32 curPos, const uint32 jumpTable[], int n)
    {
        for (int i = 0; i < n; i++)
        {
            generateOffsetedMove(curPos, jumpTable[i]);
        }
    }

    __forceinline static void generateKnightMoves(uint32 curPos)
    {
        const uint32 jumpTable[] = {0x1F, 0x21, 0xE, 0x12, -0x12, -0xE, -0x21, -0x1F};
        generateOffsetedMoves(curPos, jumpTable, 8);
    }

    __forceinline static void generateKingMoves(uint32 curPos)
    {
        // normal moves
        const uint32 jumpTable[] = {0xF, 0x10, 0x11, 0x1, -0x1, -0x11, -0x10, -0xF};
        generateOffsetedMoves(curPos, jumpTable, 8);

        // castling
        uint32 castleFlag = chance ? pos->blackCastle : pos->whiteCastle ;
        if ((castleFlag & CASTLE_FLAG_KING_SIDE) && (pos->board[curPos+1] == 0) && (pos->board[curPos+2] == 0))
        {
            // no need to check for king's and rook's position as if they have moved, the castle flag would be zero
            // TODO: check if any of the squares involved is in check!
            addMove(curPos, curPos + 0x2, 0, CASTLE_KING_SIDE);
        }
        if ((castleFlag & CASTLE_FLAG_QUEEN_SIDE) && (pos->board[curPos-1] == 0) && 
            (pos->board[curPos-2] == 0) && (pos->board[curPos-3] == 0))
        {
            // no need to check for king's and rook's position as if they have moved, the castle flag would be zero
            // TODO: check if any of the squares involved is in check!
            addMove(curPos, curPos - 0x2, 0, CASTLE_QUEEN_SIDE);
        }
    }

    __forceinline static void generateSlidingMoves(const uint32 curPos, const uint32 offset)
    {
        uint32 newPos = curPos;
        while(1)
        {
            newPos += offset;
            if (ISVALIDPOS(newPos))
            {
                uint32 oldPiece = pos->board[newPos];
                if (oldPiece)
                {
                    if(COLOR(oldPiece) != chance)
                    {
                        addMove(curPos, newPos, oldPiece, 0);
                    }
                    break;
                }
                else
                {
                    addMove(curPos, newPos, 0, 0);
                }
            }
            else
            {
                break;
            }
        }
    }

    __forceinline static void generateRookMoves(uint32 curPos)
    {
        generateSlidingMoves(curPos,  0x10);    // up
        generateSlidingMoves(curPos, -0x10);    // down
        generateSlidingMoves(curPos,   0x1);    // right
        generateSlidingMoves(curPos,  -0x1);    // left
    }

    __forceinline static void generateBishopMoves(uint32 curPos)
    {
        generateSlidingMoves(curPos,   0xf);    // north-west
        generateSlidingMoves(curPos,  0x11);    // north-east
        generateSlidingMoves(curPos, -0x11);    // south-west
        generateSlidingMoves(curPos,  -0xf);    // south-east
    }

    __forceinline static void generateQueenMoves(uint32 curPos)
    {
        generateRookMoves(curPos);
        generateBishopMoves(curPos);
    }

    __forceinline static void generateMovesForSquare(uint32 index088, uint32 colorpiece)
    {
        uint32 piece = PIECE(colorpiece);
        switch(piece)
        {
            case PAWN:
                return generatePawnMoves(index088);
            case KNIGHT:
                return generateKnightMoves(index088);
            case BISHOP:
                return generateBishopMoves(index088);
            case ROOK:
                return generateRookMoves(index088);
            case QUEEN:
                return generateQueenMoves(index088);
            case KING:
                return generateKingMoves(index088);
        }
    }
public:

    // generates moves for the given board position
    // returns the no of moves generated
    static int generateMoves (BoardPosition *position, Move *generatedMoves)
    {
        pos = position;
        moves = generatedMoves;
        nMoves = 0;
        chance = pos->chance;

        // loop through all the squares in the board
        // TODO: maybe keep a list of squares containing white and black pieces?

        
        uint32 i, j;

        for (i = 0; i < 8; i++)
        {
            for (j = 0; j < 8; j++)
            {
                uint32 index088 = INDEX088(i, j);
                uint32 piece = position->board[index088];
                if(COLOR(piece) == chance)
                {
                    generateMovesForSquare(index088, piece);
                }
            }
        }

        return nMoves;
    }
};

// static members have to be defined seperately.. what the crap! ?
BoardPosition *MoveGenerator::pos;
Move *MoveGenerator::moves;
uint32 MoveGenerator::nMoves;
uint32 MoveGenerator::chance;



// routines to make a move on the board and to undo it

void makeMove(BoardPosition *pos, Move move)
{

    pos->board[move.dst] = pos->board[move.src];
    pos->board[move.src] = 0;

    uint32 chance = pos->chance;

    if (move.flags)
    {
        // special  moves

        // 1. Castling: update the rook position too
        if(move.flags == CASTLE_FLAG_KING_SIDE)
        {
            if (chance == BLACK)
            {
                pos->board[0x77] = 0; 
                pos->board[0x75] = COLOR_PIECE(BLACK, ROOK);
            }
            else
            {
                pos->board[0x07] = 0; 
                pos->board[0x05] = COLOR_PIECE(WHITE, ROOK);
            }
                        
        }
        else if (move.flags == CASTLE_QUEEN_SIDE)
        {
            if (chance == BLACK)
            {
                pos->board[0x70] = 0; 
                pos->board[0x73] = COLOR_PIECE(BLACK, ROOK);
            }
            else
            {
                pos->board[0x00] = 0; 
                pos->board[0x03] = COLOR_PIECE(WHITE, ROOK);
            }
        }

        // 2. en-passent: clear the captured piece
        else if (move.flags == EN_PASSENT)
        {
            pos->board[INDEX088(RANK(move.src), pos->enPassent - 1)] = 0;
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
uint64 perft(BoardPosition *pos, int depth)
{
    // TODO: check if keeping this local variable is ok
    Move moves[MAX_MOVES];
    uint64 childPerft = 0;

    uint32 nMoves = MoveGenerator::generateMoves(pos, moves);
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
        childPerft += perft(&newPos, depth - 1);
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
    Utils::readFENString("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", &testBoard); // start.. 20 positions
    //Utils::readFENString("3Q4/1Q4Q1/4Q3/2Q4R/Q4Q2/3Q4/1Q4Rp/1K1BBNNk w - - 0 1", &testBoard); // - 218 positions.. correct!
    //Utils::readFENString("8/8/8/4Q3/8/8/8/8 w - - 0 1", &testBoard);
    //Utils::readFENString("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", &testBoard);   // - 48 positions.. correct!
    // the above position exposes a bug starting n = 2

    printf("%d\n", testBoard.whiteCastle);
    Utils::dispBoard(&testBoard);

    Move moves[MAX_MOVES];
    uint32 nMoves = MoveGenerator::generateMoves(&testBoard, moves);
    printf("\nMoves generated: %d\n", nMoves);

    int depth = 6;
    uint64 leafNodes;
    START_TIMER
    leafNodes = perft(&testBoard, depth);
    STOP_TIMER
    printf("\nPerft %d: %llu\n", depth, leafNodes);
    printf("\nTime taken: %g seconds, nps: %llu\n", gTime/1000.0, (uint64) ((leafNodes/gTime)*1000.0));

    return 0;
}