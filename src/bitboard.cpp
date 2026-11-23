#include "bitboard.h"

#include <cassert>

namespace bb {

namespace {

// Ray directions. The first four increase the square index (positive rays),
// the last four decrease it (negative rays). This split decides whether the
// nearest blocker is found with lsb or msb.
enum Direction { NORTH, EAST, NORTH_EAST, NORTH_WEST, SOUTH, WEST, SOUTH_WEST, SOUTH_EAST };
constexpr int DIR_FILE[8] = {0, 1, 1, -1, 0, -1, -1, 1};
constexpr int DIR_RANK[8] = {1, 0, 1, 1, -1, 0, -1, -1};

Bitboard PAWN_ATTACKS[2][64];
Bitboard KNIGHT_ATTACKS[64];
Bitboard KING_ATTACKS[64];
Bitboard RAYS[8][64];
Bitboard BETWEEN[64][64];
Bitboard LINE[64][64];

bool on_board(int file, int rank) { return file >= 0 && file < 8 && rank >= 0 && rank < 8; }

// Attack set from s for a list of (file, rank) offsets, clipped at the board edge.
Bitboard leaper_attacks(Square s, const int (*offsets)[2], int count) {
    Bitboard result = 0;
    for (int i = 0; i < count; ++i) {
        int f = file_of(s) + offsets[i][0];
        int r = rank_of(s) + offsets[i][1];
        if (on_board(f, r)) result |= square_bb(make_square(f, r));
    }
    return result;
}

Bitboard ray_attack(int dir, Square s, Bitboard occupied) {
    Bitboard attacks = RAYS[dir][s];
    Bitboard blockers = attacks & occupied;
    if (blockers) {
        Square first = dir < SOUTH ? lsb(blockers) : msb(blockers);
        attacks ^= RAYS[dir][first];
    }
    return attacks;
}

}  // namespace

// Portable C++17 bit tricks (std::popcount / std::countr_zero are C++20).
// Bit scans multiply an isolated bit by a De Bruijn constant so that the top
// six bits of the product are unique for every power of two.
constexpr Bitboard DE_BRUIJN = 0x03f79d71b4cb0a89ULL;
int DE_BRUIJN_INDEX[64];

int popcount(Bitboard b) {
    b = b - ((b >> 1) & 0x5555555555555555ULL);
    b = (b & 0x3333333333333333ULL) + ((b >> 2) & 0x3333333333333333ULL);
    b = (b + (b >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
    return int((b * 0x0101010101010101ULL) >> 56);
}

Square lsb(Bitboard b) {
    assert(b);
    return DE_BRUIJN_INDEX[((b & (0 - b)) * DE_BRUIJN) >> 58];
}

Square msb(Bitboard b) {
    assert(b);
    b |= b >> 1;
    b |= b >> 2;
    b |= b >> 4;
    b |= b >> 8;
    b |= b >> 16;
    b |= b >> 32;
    return DE_BRUIJN_INDEX[((b ^ (b >> 1)) * DE_BRUIJN) >> 58];
}

void init() {
    Bitboard seen = 0;
    for (int i = 0; i < 64; ++i) {
        int key = int(((Bitboard(1) << i) * DE_BRUIJN) >> 58);
        DE_BRUIJN_INDEX[key] = i;
        seen |= Bitboard(1) << key;
    }
    assert(seen == ~Bitboard(0));  // every power of two maps to a distinct slot


    constexpr int KNIGHT_OFFSETS[8][2] = {{1, 2}, {2, 1}, {2, -1}, {1, -2},
                                          {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}};
    constexpr int KING_OFFSETS[8][2] = {{1, 0}, {1, 1}, {0, 1}, {-1, 1},
                                        {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};
    constexpr int WHITE_PAWN_OFFSETS[2][2] = {{-1, 1}, {1, 1}};
    constexpr int BLACK_PAWN_OFFSETS[2][2] = {{-1, -1}, {1, -1}};

    for (Square s = 0; s < 64; ++s) {
        KNIGHT_ATTACKS[s] = leaper_attacks(s, KNIGHT_OFFSETS, 8);
        KING_ATTACKS[s] = leaper_attacks(s, KING_OFFSETS, 8);
        PAWN_ATTACKS[WHITE][s] = leaper_attacks(s, WHITE_PAWN_OFFSETS, 2);
        PAWN_ATTACKS[BLACK][s] = leaper_attacks(s, BLACK_PAWN_OFFSETS, 2);

        for (int dir = 0; dir < 8; ++dir) {
            Bitboard ray = 0;
            int f = file_of(s) + DIR_FILE[dir];
            int r = rank_of(s) + DIR_RANK[dir];
            while (on_board(f, r)) {
                ray |= square_bb(make_square(f, r));
                f += DIR_FILE[dir];
                r += DIR_RANK[dir];
            }
            RAYS[dir][s] = ray;
        }
    }

    for (Square a = 0; a < 64; ++a) {
        for (int dir = 0; dir < 8; ++dir) {
            int opposite = (dir + 4) % 8;
            Bitboard ray = RAYS[dir][a];
            while (ray) {
                Square b = pop_lsb(ray);
                // Squares from a toward b, minus the ray continuing past b.
                BETWEEN[a][b] = RAYS[dir][a] & ~RAYS[dir][b] & ~square_bb(b);
                LINE[a][b] = RAYS[dir][a] | RAYS[opposite][a] | square_bb(a);
            }
        }
    }
}

Bitboard pawn_attacks(Color c, Square s) { return PAWN_ATTACKS[c][s]; }
Bitboard knight_attacks(Square s) { return KNIGHT_ATTACKS[s]; }
Bitboard king_attacks(Square s) { return KING_ATTACKS[s]; }

Bitboard bishop_attacks(Square s, Bitboard occupied) {
    return ray_attack(NORTH_EAST, s, occupied) | ray_attack(NORTH_WEST, s, occupied) |
           ray_attack(SOUTH_WEST, s, occupied) | ray_attack(SOUTH_EAST, s, occupied);
}

Bitboard rook_attacks(Square s, Bitboard occupied) {
    return ray_attack(NORTH, s, occupied) | ray_attack(EAST, s, occupied) |
           ray_attack(SOUTH, s, occupied) | ray_attack(WEST, s, occupied);
}

Bitboard between(Square a, Square b) { return BETWEEN[a][b]; }
Bitboard line(Square a, Square b) { return LINE[a][b]; }

std::string to_string(Bitboard b) {
    std::string out;
    for (int r = 7; r >= 0; --r) {
        for (int f = 0; f < 8; ++f) out += (b & square_bb(make_square(f, r))) ? "X " : ". ";
        out += '\n';
    }
    return out;
}

}  // namespace bb
