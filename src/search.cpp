#include "search.h"

#include "eval.h"
#include "movegen.h"

#include <algorithm>
#include <cassert>

namespace {

// Captures first, most valuable victim / least valuable attacker; then
// promotions; quiet moves keep generation order.
int order_key(const Position& pos, Move m) {
    int key = 0;
    if (is_capture(m)) {
        PieceType victim = move_flag(m) == EN_PASSANT ? PAWN : piece_type(pos.piece_on(move_to(m)));
        PieceType attacker = piece_type(pos.piece_on(move_from(m)));
        key += 10 * PIECE_VALUE[victim] - PIECE_VALUE[attacker] + 100000;
    }
    if (is_promotion(m)) key += PIECE_VALUE[promotion_type(m)];
    return key;
}

void order_moves(const Position& pos, MoveList& list) {
    int keys[256];
    for (int i = 0; i < list.size; ++i) keys[i] = order_key(pos, list.moves[i]);
    // Insertion sort, descending by key; lists are short.
    for (int i = 1; i < list.size; ++i) {
        Move m = list.moves[i];
        int k = keys[i];
        int j = i - 1;
        while (j >= 0 && keys[j] < k) {
            list.moves[j + 1] = list.moves[j];
            keys[j + 1] = keys[j];
            --j;
        }
        list.moves[j + 1] = m;
        keys[j + 1] = k;
    }
}

int negamax(Position& pos, int depth, int alpha, int beta, int ply, std::uint64_t& nodes) {
    ++nodes;
    MoveList list;
    generate_legal_moves(pos, list);
    // Checkmate and stalemate are scored before the depth cutoff so that both
    // searches treat terminal positions identically.
    if (list.size == 0) return pos.in_check() ? -(MATE_SCORE - ply) : 0;
    if (depth == 0) return evaluate(pos);

    order_moves(pos, list);
    int best = -INFINITE_SCORE;
    for (Move m : list) {
        pos.make_move(m);
        int score = -negamax(pos, depth - 1, -beta, -alpha, ply + 1, nodes);
        pos.unmake_move();
        if (score > best) best = score;
        if (best > alpha) alpha = best;
        if (alpha >= beta) break;  // beta cutoff
    }
    return best;
}

// Returns the score from White's point of view.
int minimax(Position& pos, int depth, int ply, std::uint64_t& nodes) {
    ++nodes;
    const bool white = pos.side_to_move() == WHITE;
    MoveList list;
    generate_legal_moves(pos, list);
    if (list.size == 0) {
        if (!pos.in_check()) return 0;
        return white ? -(MATE_SCORE - ply) : MATE_SCORE - ply;
    }
    if (depth == 0) return white ? evaluate(pos) : -evaluate(pos);

    int best = white ? -INFINITE_SCORE : INFINITE_SCORE;
    for (Move m : list) {
        pos.make_move(m);
        int score = minimax(pos, depth - 1, ply + 1, nodes);
        pos.unmake_move();
        best = white ? std::max(best, score) : std::min(best, score);
    }
    return best;
}

}  // namespace

SearchResult alphabeta_search(Position& pos, int depth) {
    assert(depth >= 1);
    SearchResult result{0, NO_MOVE, 1};
    MoveList list;
    generate_legal_moves(pos, list);
    if (list.size == 0) {
        result.score = pos.in_check() ? -MATE_SCORE : 0;
        return result;
    }

    order_moves(pos, list);
    int alpha = -INFINITE_SCORE;
    for (Move m : list) {
        pos.make_move(m);
        int score = -negamax(pos, depth - 1, -INFINITE_SCORE, -alpha, 1, result.nodes);
        pos.unmake_move();
        if (score > alpha) {
            alpha = score;
            result.best_move = m;
        }
    }
    result.score = alpha;
    return result;
}

SearchResult minimax_search(Position& pos, int depth) {
    assert(depth >= 1);
    SearchResult result{0, NO_MOVE, 1};
    const bool white = pos.side_to_move() == WHITE;
    MoveList list;
    generate_legal_moves(pos, list);
    if (list.size == 0) {
        result.score = pos.in_check() ? -MATE_SCORE : 0;
        return result;
    }

    int best = white ? -INFINITE_SCORE : INFINITE_SCORE;
    for (Move m : list) {
        pos.make_move(m);
        int score = minimax(pos, depth - 1, 1, result.nodes);
        pos.unmake_move();
        if (white ? score > best : score < best) {
            best = score;
            result.best_move = m;
        }
    }
    result.score = white ? best : -best;
    return result;
}
