#include "perft.h"

#include "movegen.h"

std::uint64_t perft(Position& pos, int depth) {
    MoveList list;
    generate_legal_moves(pos, list);
    // Bulk counting: at depth 1 the number of legal moves is the leaf count.
    if (depth <= 1) return depth == 1 ? std::uint64_t(list.size) : 1;

    std::uint64_t nodes = 0;
    for (Move m : list) {
        pos.make_move(m);
        nodes += perft(pos, depth - 1);
        pos.unmake_move();
    }
    return nodes;
}

std::vector<std::pair<Move, std::uint64_t>> perft_divide(Position& pos, int depth) {
    MoveList list;
    generate_legal_moves(pos, list);

    std::vector<std::pair<Move, std::uint64_t>> result;
    for (Move m : list) {
        pos.make_move(m);
        result.emplace_back(m, perft(pos, depth - 1));
        pos.unmake_move();
    }
    return result;
}
