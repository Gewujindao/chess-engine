// Bitboard helpers and precomputed attack tables.
//
// Sliding-piece attacks use the "classical approach" described on the
// Chess Programming Wiki: one precomputed ray per direction and square,
// cut off at the first blocker found with a bit scan.
#pragma once

#include "types.h"

#include <string>

namespace bb {

constexpr Bitboard square_bb(Square s) { return Bitboard(1) << s; }

constexpr Bitboard FILE_A = 0x0101010101010101ULL;
constexpr Bitboard FILE_H = FILE_A << 7;
constexpr Bitboard RANK_1 = 0xFFULL;
constexpr Bitboard RANK_2 = RANK_1 << 8;
constexpr Bitboard RANK_4 = RANK_1 << 24;
constexpr Bitboard RANK_5 = RANK_1 << 32;
constexpr Bitboard RANK_7 = RANK_1 << 48;
constexpr Bitboard RANK_8 = RANK_1 << 56;

int popcount(Bitboard b);
Square lsb(Bitboard b);  // b must be non-zero
Square msb(Bitboard b);  // b must be non-zero

inline Square pop_lsb(Bitboard& b) {
    Square s = lsb(b);
    b &= b - 1;
    return s;
}

// Must be called once before any other function in this namespace.
void init();

Bitboard pawn_attacks(Color c, Square s);
Bitboard knight_attacks(Square s);
Bitboard king_attacks(Square s);
Bitboard bishop_attacks(Square s, Bitboard occupied);
Bitboard rook_attacks(Square s, Bitboard occupied);
inline Bitboard queen_attacks(Square s, Bitboard occupied) {
    return bishop_attacks(s, occupied) | rook_attacks(s, occupied);
}

// Squares strictly between a and b if they share a rank, file or diagonal; else 0.
Bitboard between(Square a, Square b);
// The full line (edge to edge) through a and b if they are aligned; else 0.
Bitboard line(Square a, Square b);

// 8x8 picture of a bitboard, rank 8 at the top. Used for debugging.
std::string to_string(Bitboard b);

}  // namespace bb
