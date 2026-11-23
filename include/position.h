// Board state: piece bitboards, a mailbox for piece lookup, FEN I/O and make/unmake.
#pragma once

#include "bitboard.h"
#include "types.h"

#include <string>
#include <vector>

class Position {
public:
    static constexpr const char* START_FEN =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    // Parses a FEN string. The input is assumed to be a well-formed, legal position.
    explicit Position(const std::string& fen = START_FEN);

    std::string fen() const;

    void make_move(Move m);
    void unmake_move();

    Color side_to_move() const { return side_; }
    Piece piece_on(Square s) const { return board_[s]; }
    Bitboard pieces(Color c, PieceType t) const { return pieces_[c][t]; }
    Bitboard pieces(Color c) const { return by_color_[c]; }
    Bitboard occupied() const { return by_color_[WHITE] | by_color_[BLACK]; }
    int castling_rights() const { return castling_; }
    Square en_passant_square() const { return ep_square_; }
    Square king_square(Color c) const { return bb::lsb(pieces_[c][KING]); }

    // All pieces of color `by` that attack square s, given an occupancy.
    Bitboard attackers(Square s, Color by, Bitboard occupied) const;
    bool in_check() const { return attackers(king_square(side_), ~side_, occupied()) != 0; }

    // Long algebraic notation, e.g. "e2e4" or "e7e8q".
    static std::string move_to_uci(Move m);

private:
    struct Undo {
        Move move;
        Piece captured;
        int castling;
        Square ep_square;
        int halfmove_clock;
    };

    void put_piece(Piece p, Square s);
    void remove_piece(Square s);
    void move_piece(Square from, Square to);

    Bitboard pieces_[2][6] = {};
    Bitboard by_color_[2] = {};
    Piece board_[64];
    Color side_ = WHITE;
    int castling_ = 0;
    Square ep_square_ = NO_SQUARE;
    int halfmove_clock_ = 0;
    int fullmove_number_ = 1;
    std::vector<Undo> history_;
};
