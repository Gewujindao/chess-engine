#include "movegen.h"

namespace {

using bb::square_bb;

void add_moves(MoveList& list, Square from, Bitboard targets, Bitboard enemy) {
    while (targets) {
        Square to = bb::pop_lsb(targets);
        list.add(make_move(from, to, (enemy & square_bb(to)) ? CAPTURE : QUIET));
    }
}

void add_pawn_move(MoveList& list, Square from, Square to, bool capture, bool promotion) {
    int base = capture ? CAPTURE : QUIET;
    if (promotion) {
        for (int piece = 0; piece < 4; ++piece) list.add(make_move(from, to, base | PROMOTION | piece));
    } else {
        list.add(make_move(from, to, base));
    }
}

}  // namespace

void generate_legal_moves(const Position& pos, MoveList& list) {
    const Color us = pos.side_to_move();
    const Color them = ~us;
    const Bitboard own = pos.pieces(us);
    const Bitboard enemy = pos.pieces(them);
    const Bitboard occupied = own | enemy;
    const Square king = pos.king_square(us);
    const Bitboard checkers = pos.attackers(king, them, occupied);

    // King moves. The king is removed from the occupancy so that a slider
    // checking along a line still "sees" the square directly behind the king.
    const Bitboard occupied_without_king = occupied ^ square_bb(king);
    Bitboard king_targets = bb::king_attacks(king) & ~own;
    while (king_targets) {
        Square to = bb::pop_lsb(king_targets);
        if (!pos.attackers(to, them, occupied_without_king)) {
            list.add(make_move(king, to, (enemy & square_bb(to)) ? CAPTURE : QUIET));
        }
    }

    // In double check only the king may move.
    if (bb::popcount(checkers) > 1) return;

    // In single check, other pieces must capture the checker or block the line.
    const Bitboard check_mask =
        checkers ? checkers | bb::between(king, bb::lsb(checkers)) : ~Bitboard(0);

    // A piece is pinned when it is the only piece between our king and an
    // enemy slider on the same line. A pinned piece may only move along that line.
    Bitboard pinned = 0;
    Bitboard snipers =
        (bb::rook_attacks(king, enemy) & (pos.pieces(them, ROOK) | pos.pieces(them, QUEEN))) |
        (bb::bishop_attacks(king, enemy) & (pos.pieces(them, BISHOP) | pos.pieces(them, QUEEN)));
    while (snipers) {
        Square sniper = bb::pop_lsb(snipers);
        Bitboard blockers = bb::between(king, sniper) & occupied;
        if (bb::popcount(blockers) == 1 && (blockers & own)) pinned |= blockers;
    }
    auto pin_mask = [&](Square from) {
        return (pinned & square_bb(from)) ? bb::line(king, from) : ~Bitboard(0);
    };

    // Knights, bishops, rooks and queens.
    for (PieceType type : {KNIGHT, BISHOP, ROOK, QUEEN}) {
        Bitboard pieces = pos.pieces(us, type);
        while (pieces) {
            Square from = bb::pop_lsb(pieces);
            Bitboard attacks = type == KNIGHT   ? bb::knight_attacks(from)
                               : type == BISHOP ? bb::bishop_attacks(from, occupied)
                               : type == ROOK   ? bb::rook_attacks(from, occupied)
                                                : bb::queen_attacks(from, occupied);
            add_moves(list, from, attacks & ~own & check_mask & pin_mask(from), enemy);
        }
    }

    // Pawns.
    const int forward = us == WHITE ? 8 : -8;
    const Bitboard start_rank = us == WHITE ? bb::RANK_2 : bb::RANK_7;
    const Bitboard promotion_rank = us == WHITE ? bb::RANK_8 : bb::RANK_1;
    const Square ep_square = pos.en_passant_square();

    Bitboard pawns = pos.pieces(us, PAWN);
    while (pawns) {
        Square from = bb::pop_lsb(pawns);
        const Bitboard mask = check_mask & pin_mask(from);

        Square one = from + forward;
        if (!(occupied & square_bb(one))) {
            if (mask & square_bb(one)) {
                add_pawn_move(list, from, one, false, (promotion_rank & square_bb(one)) != 0);
            }
            Square two = one + forward;
            if ((start_rank & square_bb(from)) && !(occupied & square_bb(two)) &&
                (mask & square_bb(two))) {
                list.add(make_move(from, two, DOUBLE_PUSH));
            }
        }

        Bitboard captures = bb::pawn_attacks(us, from) & enemy & mask;
        while (captures) {
            Square to = bb::pop_lsb(captures);
            add_pawn_move(list, from, to, true, (promotion_rank & square_bb(to)) != 0);
        }

        // En passant removes two pieces from one rank at once, which pin masks
        // cannot describe. Instead, rebuild the occupancy after the capture and
        // test directly whether our king would be attacked.
        if (ep_square != NO_SQUARE && (bb::pawn_attacks(us, from) & square_bb(ep_square))) {
            Square captured = ep_square - forward;
            Bitboard after = (occupied ^ square_bb(from) ^ square_bb(captured)) | square_bb(ep_square);
            if (!(pos.attackers(king, them, after) & ~square_bb(captured))) {
                list.add(make_move(from, ep_square, EN_PASSANT));
            }
        }
    }

    // Castling: not out of check, the squares between king and rook empty,
    // and the king may not pass through or land on an attacked square.
    if (!checkers) {
        const Square base = us == WHITE ? 0 : 56;
        const int oo = us == WHITE ? WHITE_OO : BLACK_OO;
        const int ooo = us == WHITE ? WHITE_OOO : BLACK_OOO;
        auto safe = [&](Square s) { return !pos.attackers(s, them, occupied); };

        if ((pos.castling_rights() & oo) &&
            !(occupied & (square_bb(base + 5) | square_bb(base + 6))) && safe(base + 5) &&
            safe(base + 6)) {
            list.add(make_move(king, base + 6, KING_CASTLE));
        }
        if ((pos.castling_rights() & ooo) &&
            !(occupied & (square_bb(base + 1) | square_bb(base + 2) | square_bb(base + 3))) &&
            safe(base + 3) && safe(base + 2)) {
            list.add(make_move(king, base + 2, QUEEN_CASTLE));
        }
    }
}
