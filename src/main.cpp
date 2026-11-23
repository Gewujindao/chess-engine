// Command-line front end.
//
//   chess perft  <depth> [fen]   leaf node count
//   chess divide <depth> [fen]   leaf node count per root move
//   chess search <depth> [fen]   alpha-beta search; prints score, best move, nodes
//   chess perft-batch <file>     each line "<depth> <fen>" -> prints the count per line
//
// The FEN defaults to the starting position. It may be given as one quoted
// argument or as the six separate FEN fields.
#include "bitboard.h"
#include "perft.h"
#include "position.h"
#include "search.h"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::string fen_from_args(int argc, char** argv, int first) {
    if (first >= argc) return Position::START_FEN;
    std::string fen = argv[first];
    for (int i = first + 1; i < argc; ++i) fen += std::string(" ") + argv[i];
    return fen;
}

double seconds_since(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

void usage() {
    std::cerr << "usage:\n"
                 "  chess perft  <depth> [fen]\n"
                 "  chess divide <depth> [fen]\n"
                 "  chess search <depth> [fen]\n"
                 "  chess perft-batch <file>\n";
}

}  // namespace

int main(int argc, char** argv) {
    bb::init();
    if (argc < 3) {
        usage();
        return 1;
    }
    const std::string command = argv[1];

    if (command == "perft-batch") {
        std::ifstream in(argv[2]);
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            std::istringstream fields(line);
            int depth;
            fields >> depth;
            std::string fen;
            std::getline(fields >> std::ws, fen);
            Position pos(fen);
            std::cout << perft(pos, depth) << '\n';
        }
        return 0;
    }

    const int depth = std::atoi(argv[2]);
    Position pos(fen_from_args(argc, argv, 3));
    const auto start = std::chrono::steady_clock::now();

    if (command == "perft") {
        std::uint64_t nodes = perft(pos, depth);
        std::cout << "nodes " << nodes << "\ntime " << seconds_since(start) << " s\n";
    } else if (command == "divide") {
        std::uint64_t total = 0;
        for (const auto& [move, count] : perft_divide(pos, depth)) {
            std::cout << Position::move_to_uci(move) << ": " << count << '\n';
            total += count;
        }
        std::cout << "total " << total << '\n';
    } else if (command == "search") {
        SearchResult r = alphabeta_search(pos, depth);
        std::cout << "score " << r.score << "\nbestmove "
                  << (r.best_move == NO_MOVE ? "(none)" : Position::move_to_uci(r.best_move))
                  << "\nnodes " << r.nodes << "\ntime " << seconds_since(start) << " s\n";
    } else {
        usage();
        return 1;
    }
    return 0;
}
