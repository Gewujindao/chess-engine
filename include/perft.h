// Perft: count the leaf nodes of the legal move tree to a fixed depth.
#pragma once

#include "position.h"

#include <cstdint>
#include <utility>
#include <vector>

std::uint64_t perft(Position& pos, int depth);

// Perft split by root move ("divide"), useful for locating a move generation bug.
std::vector<std::pair<Move, std::uint64_t>> perft_divide(Position& pos, int depth);
