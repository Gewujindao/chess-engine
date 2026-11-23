// Basic types shared across the engine: squares, pieces, colors and moves.
#pragma once

#include <cstdint>

using Bitboard = std::uint64_t;

// Squares are numbered a1 = 0, b1 = 1, ..., h8 = 63 (little-endian rank-file mapping).
using Square = int;
constexpr Square NO_SQUARE = -1;

enum Color : int { WHITE = 0, BLACK = 1 };
constexpr Color operator~(Color c) { return Color(c ^ 1); }

enum PieceType : int { PAWN = 0, KNIGHT, BISHOP, ROOK, QUEEN, KING, NO_PIECE_TYPE };

// Piece = color * 6 + type. NO_PIECE marks an empty square.
using Piece = int;
constexpr Piece NO_PIECE = 12;
constexpr Piece make_piece(Color c, PieceType t) { return c * 6 + t; }
constexpr Color piece_color(Piece p) { return Color(p / 6); }
constexpr PieceType piece_type(Piece p) { return PieceType(p % 6); }

constexpr int file_of(Square s) { return s & 7; }
constexpr int rank_of(Square s) { return s >> 3; }
constexpr Square make_square(int file, int rank) { return rank * 8 + file; }

// Castling rights bit flags.
enum CastlingRight : int { WHITE_OO = 1, WHITE_OOO = 2, BLACK_OO = 4, BLACK_OOO = 8 };

// A move packs from (6 bits), to (6 bits) and a 4-bit flag into 16 bits.
// Flag layout follows the common "from-to + 4 flag bits" scheme:
//   0 quiet, 1 double pawn push, 2 king castle, 3 queen castle,
//   4 capture, 5 en passant capture,
//   8..11 promotion to N/B/R/Q, 12..15 promotion-capture to N/B/R/Q.
using Move = std::uint16_t;
constexpr Move NO_MOVE = 0;

enum MoveFlag : int {
    QUIET = 0,
    DOUBLE_PUSH = 1,
    KING_CASTLE = 2,
    QUEEN_CASTLE = 3,
    CAPTURE = 4,
    EN_PASSANT = 5,
    PROMOTION = 8,  // bit set for all promotions; low 2 bits select the piece
};

constexpr Move make_move(Square from, Square to, int flag) {
    return Move(from | (to << 6) | (flag << 12));
}
constexpr Square move_from(Move m) { return m & 63; }
constexpr Square move_to(Move m) { return (m >> 6) & 63; }
constexpr int move_flag(Move m) { return m >> 12; }
constexpr bool is_capture(Move m) { return (move_flag(m) & CAPTURE) != 0; }
constexpr bool is_promotion(Move m) { return (move_flag(m) & PROMOTION) != 0; }
constexpr PieceType promotion_type(Move m) { return PieceType(KNIGHT + (move_flag(m) & 3)); }
