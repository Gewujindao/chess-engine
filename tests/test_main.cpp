// Test suite: FEN round trip, perft against published node counts, and
// alpha-beta scores against the unpruned minimax reference.
// Exit code is the number of failed checks.
#include "bitboard.h"
#include "perft.h"
#include "position.h"
#include "search.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace {

int passed = 0;
int failed = 0;

void check(bool ok) { ok ? ++passed : ++failed; }

double seconds_since(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

struct PerftCase {
    const char* name;
    const char* fen;
    std::vector<std::uint64_t> expected;  // expected[d - 1] is perft(d)
};

// Standard test positions and node counts from the Chess Programming Wiki
// "Perft Results" page.
const std::vector<PerftCase> PERFT_CASES = {
    {"startpos", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
     {20, 400, 8902, 197281, 4865609, 119060324}},
    {"kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
     {48, 2039, 97862, 4085603, 193690690}},
    {"position3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
     {14, 191, 2812, 43238, 674624, 11030083, 178633661}},
    {"position4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
     {6, 264, 9467, 422333, 15833292, 706045033}},
    {"position4_mirrored", "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1",
     {6, 264, 9467, 422333, 15833292, 706045033}},
    {"position5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
     {44, 1486, 62379, 2103487, 89941194}},
    {"position6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
     {46, 2079, 89890, 3894594, 164075551}},
};

void test_fen_round_trip() {
    std::printf("== FEN round trip\n");
    for (const PerftCase& c : PERFT_CASES) {
        bool ok = Position(c.fen).fen() == c.fen;
        std::printf("%-20s %s\n", c.name, ok ? "PASS" : "FAIL");
        check(ok);
    }
}

void test_perft() {
    std::printf("\n== Perft vs published node counts\n");
    std::printf("%-20s %5s %12s %12s %8s  %s\n", "position", "depth", "expected", "engine",
                "seconds", "result");
    for (const PerftCase& c : PERFT_CASES) {
        Position pos(c.fen);
        for (int depth = 1; depth <= int(c.expected.size()); ++depth) {
            auto start = std::chrono::steady_clock::now();
            std::uint64_t nodes = perft(pos, depth);
            double secs = seconds_since(start);
            // make/unmake must restore the exact starting position.
            bool ok = nodes == c.expected[depth - 1] && pos.fen() == c.fen;
            std::printf("%-20s %5d %12llu %12llu %8.2f  %s\n", c.name, depth,
                        (unsigned long long)c.expected[depth - 1], (unsigned long long)nodes, secs,
                        ok ? "PASS" : "FAIL");
            check(ok);
        }
    }
}

struct SearchCase {
    const char* name;
    const char* fen;
    int max_depth;
};

const std::vector<SearchCase> SEARCH_CASES = {
    {"startpos", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5},
    {"kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4},
    {"position3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5},
    {"position4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4},
    {"position4_mirrored", "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 4},
    {"position5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4},
    {"position6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 4},
    {"back_rank_mate_in_1", "6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1", 5},
    {"queen_vs_king", "7k/8/6K1/5Q2/8/8/8/8 w - - 0 1", 5},
    {"en_passant_available", "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3", 4},
    {"promotion_race", "8/P6k/8/8/8/8/6p1/7K w - - 0 1", 5},
    {"black_to_move", "r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3", 4},
};

void test_search() {
    std::printf("\n== Alpha-beta vs unpruned minimax (same evaluate(), fixed depth)\n");
    std::printf("%-22s %5s %9s %9s %12s %12s  %s\n", "position", "depth", "minimax", "alphabeta",
                "mm_nodes", "ab_nodes", "result");
    std::uint64_t total_mm = 0, total_ab = 0;
    for (const SearchCase& c : SEARCH_CASES) {
        Position pos(c.fen);
        for (int depth = 1; depth <= c.max_depth; ++depth) {
            SearchResult mm = minimax_search(pos, depth);
            SearchResult ab = alphabeta_search(pos, depth);
            bool ok = mm.score == ab.score && pos.fen() == c.fen;
            std::printf("%-22s %5d %9d %9d %12llu %12llu  %s\n", c.name, depth, mm.score, ab.score,
                        (unsigned long long)mm.nodes, (unsigned long long)ab.nodes,
                        ok ? "PASS" : "FAIL");
            total_mm += mm.nodes;
            total_ab += ab.nodes;
            check(ok);
        }
    }
    std::printf("total nodes: minimax %llu, alphabeta %llu\n", (unsigned long long)total_mm,
                (unsigned long long)total_ab);
}

}  // namespace

int main() {
    bb::init();
    auto start = std::chrono::steady_clock::now();
    test_fen_round_trip();
    test_perft();
    test_search();
    std::printf("\n== Summary: %d passed, %d failed (%.1f s)\n", passed, failed,
                seconds_since(start));
    return failed;
}
