// Strictly legal move generation.
#pragma once

#include "position.h"
#include "types.h"

// Fixed-capacity move list. 218 is the known maximum number of legal moves
// in any chess position, so 256 is always enough.
struct MoveList {
    Move moves[256];
    int size = 0;

    void add(Move m) { moves[size++] = m; }
    const Move* begin() const { return moves; }
    const Move* end() const { return moves + size; }
};

// Fills `list` with every legal move for the side to move. Checks, double
// checks, pins, castling through attacked squares and the en passant
// discovered-check case are handled directly, without make/unmake testing.
void generate_legal_moves(const Position& pos, MoveList& list);
