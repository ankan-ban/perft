#include "chess.h"

// note that this is a pseudo-legal move generator
// Two issues:
// 1. for castling it's not checked if the squares are in check (major issue. need to be fixed)
// 2. it's not checked if a move puts the king in check (can probably avoid checking this)

void MoveGenerator::addMove(uint32 src, uint32 dst, uint8 oldPiece, uint8 flags)
{
    moves[nMoves].src = (uint8) src;
    moves[nMoves].dst = (uint8) dst;
    moves[nMoves].capturedPiece = (uint8) oldPiece;
    moves[nMoves].flags = flags;
    nMoves++;
}

void MoveGenerator::addPromotions(uint32 src, uint32 dst, uint8 oldPiece)
{
    addMove(src, dst, oldPiece, PROMOTION_QUEEN);
    addMove(src, dst, oldPiece, PROMOTION_KNIGHT);
    addMove(src, dst, oldPiece, PROMOTION_ROOK);
    addMove(src, dst, oldPiece, PROMOTION_BISHOP);        
}

void MoveGenerator::generatePawnMoves(uint32 curPos)
{
    uint32 finalRank = chance ?  0 : 7;
    uint32 curRank   = RANK(curPos);

    // pawn advancement
    
    // single square forward
    uint32 offset  = chance ? -16 : 16;
    uint32 newPos  = curPos + offset;
    uint32 newRank = RANK(newPos);

    if (ISVALIDPOS(newPos) && ISEMPTY(pos->board[newPos]))
    {
        if(newRank == finalRank)
        {
            // promotion
            addPromotions(curPos, newPos, EMPTY_SQUARE);
        }
        else
        {
            addMove(curPos, newPos, EMPTY_SQUARE, 0);

            // two squares forward
            uint32 startRank = chance ?  6 : 1;
            if (curRank == startRank)
            {
                newPos += offset;
                if (ISEMPTY(pos->board[newPos]))
                    addMove(curPos, newPos, EMPTY_SQUARE, 0);
            }
        }
    }

    // captures
    offset = chance ? -15 : 15;
    newPos = curPos + offset;
    uint32 capturedPiece = pos->board[newPos];
    if (ISVALIDPOS(newPos) && IS_ENEMY_COLOR(capturedPiece, chance))
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
    if (ISVALIDPOS(newPos) && IS_ENEMY_COLOR(capturedPiece, chance))
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

void MoveGenerator::generateOffsetedMove(uint32 curPos, uint32 offset)
{
    uint32 newPos = curPos + offset;
    if(ISVALIDPOS(newPos))
    {
        uint8 capturedPiece = pos->board[newPos];
        if (!IS_OF_COLOR(capturedPiece, chance))
            addMove(curPos, newPos, capturedPiece, 0);
    }
}

void MoveGenerator::generateOffsetedMoves(uint32 curPos, const uint32 jumpTable[], int n)
{
    for (int i = 0; i < n; i++)
    {
        generateOffsetedMove(curPos, jumpTable[i]);
    }
}

void MoveGenerator::generateKnightMoves(uint32 curPos)
{
    const uint32 jumpTable[] = {0x1F, 0x21, 0xE, 0x12, -0x12, -0xE, -0x21, -0x1F};
    generateOffsetedMoves(curPos, jumpTable, 8);
}

void MoveGenerator::generateKingMoves(uint32 curPos)
{
    // normal moves
    const uint32 jumpTable[] = {0xF, 0x10, 0x11, 0x1, -0x1, -0x11, -0x10, -0xF};
    generateOffsetedMoves(curPos, jumpTable, 8);

    // castling
    uint32 castleFlag = chance ? pos->blackCastle : pos->whiteCastle ;
    if ((castleFlag & CASTLE_FLAG_KING_SIDE) && ISEMPTY(pos->board[curPos+1]) && ISEMPTY(pos->board[curPos+2]))
    {
        // no need to check for king's and rook's position as if they have moved, the castle flag would be zero
        // TODO: check if any of the squares involved is in check!
        addMove(curPos, curPos + 0x2, EMPTY_SQUARE, CASTLE_KING_SIDE);
    }
    if ((castleFlag & CASTLE_FLAG_QUEEN_SIDE) && ISEMPTY(pos->board[curPos-1]) && 
        ISEMPTY(pos->board[curPos-2]) && ISEMPTY(pos->board[curPos-3]))
    {
        // no need to check for king's and rook's position as if they have moved, the castle flag would be zero
        // TODO: check if any of the squares involved is in check!
        addMove(curPos, curPos - 0x2, EMPTY_SQUARE, CASTLE_QUEEN_SIDE);
    }
}

void MoveGenerator::generateSlidingMoves(const uint32 curPos, const uint32 offset)
{
    uint32 newPos = curPos;
    while(1)
    {
        newPos += offset;
        if (ISVALIDPOS(newPos))
        {
            uint32 oldPiece = pos->board[newPos];
            if (!ISEMPTY(oldPiece))
            {
                if(!IS_OF_COLOR(oldPiece,chance))
                {
                    addMove(curPos, newPos, oldPiece, 0);
                }
                break;
            }
            else
            {
				addMove(curPos, newPos, EMPTY_SQUARE, 0);
            }
        }
        else
        {
            break;
        }
    }
}

void MoveGenerator::generateRookMoves(uint32 curPos)
{
    generateSlidingMoves(curPos,  0x10);    // up
    generateSlidingMoves(curPos, -0x10);    // down
    generateSlidingMoves(curPos,   0x1);    // right
    generateSlidingMoves(curPos,  -0x1);    // left
}

void MoveGenerator::generateBishopMoves(uint32 curPos)
{
    generateSlidingMoves(curPos,   0xf);    // north-west
    generateSlidingMoves(curPos,  0x11);    // north-east
    generateSlidingMoves(curPos, -0x11);    // south-west
    generateSlidingMoves(curPos,  -0xf);    // south-east
}

void MoveGenerator::generateQueenMoves(uint32 curPos)
{
    generateRookMoves(curPos);
    generateBishopMoves(curPos);
}

void MoveGenerator::generateMovesForSquare(uint32 index088, uint32 colorpiece)
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

// generates moves for the given board position
// returns the no of moves generated
int MoveGenerator::generateMoves (BoardPosition *position, Move *generatedMoves)
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
            if(IS_OF_COLOR(piece, chance))
            {
                generateMovesForSquare(index088, piece);
            }
        }
    }

    return nMoves;
}


// static members have to be defined seperately.. what the crap! ?
BoardPosition *MoveGenerator::pos;
Move *MoveGenerator::moves;
uint32 MoveGenerator::nMoves;
uint32 MoveGenerator::chance;
