#include "position.h"

#include <cassert>
#include <sstream>

namespace {

constexpr const char* PIECE_CHARS = "PNBRQKpnbrqk";

// castling &= CASTLING_MASK[from] & CASTLING_MASK[to] removes the rights lost
// when a king or rook leaves its home square or a rook is captured there.
int castling_mask(Square s) {
    switch (s) {
        case 0: return ~WHITE_OOO;              // a1
        case 4: return ~(WHITE_OO | WHITE_OOO);  // e1
        case 7: return ~WHITE_OO;               // h1
        case 56: return ~BLACK_OOO;             // a8
        case 60: return ~(BLACK_OO | BLACK_OOO);  // e8
        case 63: return ~BLACK_OO;              // h8
        default: return ~0;
    }
}

std::string square_name(Square s) {
    return std::string{char('a' + file_of(s)), char('1' + rank_of(s))};
}

}  // namespace

Position::Position(const std::string& fen) {
    for (Piece& p : board_) p = NO_PIECE;

    std::istringstream in(fen);
    std::string placement, side, castling, ep;
    in >> placement >> side >> castling >> ep >> halfmove_clock_ >> fullmove_number_;

    int file = 0, rank = 7;
    for (char c : placement) {
        if (c == '/') {
            file = 0;
            --rank;
        } else if (c >= '1' && c <= '8') {
            file += c - '0';
        } else {
            Piece p = Piece(std::string(PIECE_CHARS).find(c));
            put_piece(p, make_square(file, rank));
            ++file;
        }
    }

    side_ = side == "w" ? WHITE : BLACK;

    for (char c : castling) {
        if (c == 'K') castling_ |= WHITE_OO;
        if (c == 'Q') castling_ |= WHITE_OOO;
        if (c == 'k') castling_ |= BLACK_OO;
        if (c == 'q') castling_ |= BLACK_OOO;
    }

    ep_square_ = ep == "-" ? NO_SQUARE : make_square(ep[0] - 'a', ep[1] - '1');
}

std::string Position::fen() const {
    std::string out;
    for (int rank = 7; rank >= 0; --rank) {
        int empty = 0;
        for (int file = 0; file < 8; ++file) {
            Piece p = board_[make_square(file, rank)];
            if (p == NO_PIECE) {
                ++empty;
                continue;
            }
            if (empty) out += char('0' + empty);
            empty = 0;
            out += PIECE_CHARS[p];
        }
        if (empty) out += char('0' + empty);
        if (rank) out += '/';
    }

    out += side_ == WHITE ? " w " : " b ";

    std::string castling;
    if (castling_ & WHITE_OO) castling += 'K';
    if (castling_ & WHITE_OOO) castling += 'Q';
    if (castling_ & BLACK_OO) castling += 'k';
    if (castling_ & BLACK_OOO) castling += 'q';
    out += castling.empty() ? "-" : castling;

    out += ' ';
    out += ep_square_ == NO_SQUARE ? "-" : square_name(ep_square_);
    out += ' ' + std::to_string(halfmove_clock_) + ' ' + std::to_string(fullmove_number_);
    return out;
}

void Position::put_piece(Piece p, Square s) {
    Bitboard b = bb::square_bb(s);
    pieces_[piece_color(p)][piece_type(p)] |= b;
    by_color_[piece_color(p)] |= b;
    board_[s] = p;
}

void Position::remove_piece(Square s) {
    Piece p = board_[s];
    Bitboard b = bb::square_bb(s);
    pieces_[piece_color(p)][piece_type(p)] ^= b;
    by_color_[piece_color(p)] ^= b;
    board_[s] = NO_PIECE;
}

void Position::move_piece(Square from, Square to) {
    Piece p = board_[from];
    Bitboard b = bb::square_bb(from) | bb::square_bb(to);
    pieces_[piece_color(p)][piece_type(p)] ^= b;
    by_color_[piece_color(p)] ^= b;
    board_[from] = NO_PIECE;
    board_[to] = p;
}

void Position::make_move(Move m) {
    const Square from = move_from(m);
    const Square to = move_to(m);
    const int flag = move_flag(m);
    const Color us = side_;
    const Piece moving = board_[from];

    Undo undo{m, NO_PIECE, castling_, ep_square_, halfmove_clock_};
    ++halfmove_clock_;

    if (flag == EN_PASSANT) {
        Square captured_square = us == WHITE ? to - 8 : to + 8;
        undo.captured = board_[captured_square];
        remove_piece(captured_square);
    } else if (is_capture(m)) {
        undo.captured = board_[to];
        remove_piece(to);
    }
    if (undo.captured != NO_PIECE || piece_type(moving) == PAWN) halfmove_clock_ = 0;

    move_piece(from, to);

    if (is_promotion(m)) {
        remove_piece(to);
        put_piece(make_piece(us, promotion_type(m)), to);
    } else if (flag == KING_CASTLE) {
        move_piece(to + 1, to - 1);
    } else if (flag == QUEEN_CASTLE) {
        move_piece(to - 2, to + 1);
    }

    ep_square_ = flag == DOUBLE_PUSH ? (from + to) / 2 : NO_SQUARE;
    castling_ &= castling_mask(from) & castling_mask(to);
    if (us == BLACK) ++fullmove_number_;
    side_ = ~us;
    history_.push_back(undo);
}

void Position::unmake_move() {
    assert(!history_.empty());
    const Undo undo = history_.back();
    history_.pop_back();

    const Move m = undo.move;
    const Square from = move_from(m);
    const Square to = move_to(m);
    const int flag = move_flag(m);
    const Color us = ~side_;
    side_ = us;

    if (is_promotion(m)) {
        remove_piece(to);
        put_piece(make_piece(us, PAWN), to);
    } else if (flag == KING_CASTLE) {
        move_piece(to - 1, to + 1);
    } else if (flag == QUEEN_CASTLE) {
        move_piece(to + 1, to - 2);
    }

    move_piece(to, from);

    if (flag == EN_PASSANT) {
        put_piece(undo.captured, us == WHITE ? to - 8 : to + 8);
    } else if (undo.captured != NO_PIECE) {
        put_piece(undo.captured, to);
    }

    castling_ = undo.castling;
    ep_square_ = undo.ep_square;
    halfmove_clock_ = undo.halfmove_clock;
    if (us == BLACK) --fullmove_number_;
}

Bitboard Position::attackers(Square s, Color by, Bitboard occupied) const {
    const Bitboard bishops_queens = pieces_[by][BISHOP] | pieces_[by][QUEEN];
    const Bitboard rooks_queens = pieces_[by][ROOK] | pieces_[by][QUEEN];
    return (bb::pawn_attacks(~by, s) & pieces_[by][PAWN]) |
           (bb::knight_attacks(s) & pieces_[by][KNIGHT]) |
           (bb::king_attacks(s) & pieces_[by][KING]) |
           (bb::bishop_attacks(s, occupied) & bishops_queens) |
           (bb::rook_attacks(s, occupied) & rooks_queens);
}

std::string Position::move_to_uci(Move m) {
    std::string out = square_name(move_from(m)) + square_name(move_to(m));
    if (is_promotion(m)) out += "nbrq"[promotion_type(m) - KNIGHT];
    return out;
}
