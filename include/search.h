// Fixed-depth search: alpha-beta (negamax) and an unpruned minimax reference.
#pragma once

#include "position.h"

#include <cstdint>

constexpr int MATE_SCORE = 100000;  // mate in n plies scores MATE_SCORE - n
constexpr int INFINITE_SCORE = 1000000;

struct SearchResult {
    int score;            // centipawns, from the side to move's point of view
    Move best_move;       // NO_MOVE if the root has no legal moves
    std::uint64_t nodes;  // number of positions visited
};

// Negamax alpha-beta search with MVV-LVA capture ordering.
SearchResult alphabeta_search(Position& pos, int depth);

// Plain minimax without any pruning or move ordering, written in the
// max/min form with White-relative scores. It shares only the move generator
// and evaluate() with alphabeta_search, and serves as the reference that the
// alpha-beta score must match exactly.
SearchResult minimax_search(Position& pos, int depth);
