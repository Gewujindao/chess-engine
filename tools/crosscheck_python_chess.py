"""Cross-check engine perft counts against python-chess, an independent move generator.

Two sets of positions are compared:
  1. the standard perft positions at depths python-chess can finish quickly;
  2. positions sampled from seeded random games, perft depth 3 each.

The script prints one line per position and exits with the number of mismatches.

usage: python tools/crosscheck_python_chess.py <path to chess engine executable> <work dir>
"""

import random
import subprocess
import sys
import time
from pathlib import Path

import chess

STANDARD = [
    ("startpos", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5),
    ("kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4),
    ("position3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5),
    ("position4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4),
    ("position4_mirrored", "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 4),
    ("position5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4),
    ("position6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 4),
]

RANDOM_SEED = 2026
RANDOM_GAMES = 100
SAMPLES_PER_GAME = 3
RANDOM_DEPTH = 3


def perft(board, depth):
    if depth == 1:
        return board.legal_moves.count()
    nodes = 0
    for move in board.legal_moves:
        board.push(move)
        nodes += perft(board, depth - 1)
        board.pop()
    return nodes


def random_positions():
    """Play seeded random games and sample positions along the way."""
    rng = random.Random(RANDOM_SEED)
    positions = []
    for _ in range(RANDOM_GAMES):
        board = chess.Board()
        length = rng.randint(10, 120)
        sample_plies = set(rng.sample(range(1, length + 1), SAMPLES_PER_GAME))
        for ply in range(1, length + 1):
            moves = list(board.legal_moves)
            if not moves:
                break
            board.push(rng.choice(moves))
            if ply in sample_plies:
                positions.append(board.fen())
    return positions


def features(fen):
    """Which special rules are reachable from a position (for coverage stats)."""
    board = chess.Board(fen)
    moves = list(board.legal_moves)
    return {
        "in_check": board.is_check(),
        "castling_move": any(board.is_castling(m) for m in moves),
        "en_passant_move": any(board.is_en_passant(m) for m in moves),
        "promotion_move": any(m.promotion for m in moves),
        "no_legal_moves": not moves,
    }


def main():
    engine = Path(sys.argv[1])
    work_dir = Path(sys.argv[2])
    work_dir.mkdir(parents=True, exist_ok=True)

    cases = [(name, fen, depth) for name, fen, depth in STANDARD]
    cases += [(f"random_{i:03d}", fen, RANDOM_DEPTH) for i, fen in enumerate(random_positions())]

    batch = work_dir / "crosscheck_input.txt"
    batch.write_text("".join(f"{depth} {fen}\n" for _, fen, depth in cases))
    start = time.time()
    output = subprocess.run([str(engine), "perft-batch", str(batch)],
                            capture_output=True, text=True, check=True).stdout.split()
    engine_seconds = time.time() - start
    engine_counts = [int(x) for x in output]
    assert len(engine_counts) == len(cases)

    print(f"python-chess {chess.__version__}, random seed {RANDOM_SEED}")
    print(f"{'position':<20} {'depth':>5} {'python-chess':>13} {'engine':>13}  result  fen")
    mismatches = 0
    start = time.time()
    for (name, fen, depth), engine_count in zip(cases, engine_counts):
        reference = perft(chess.Board(fen), depth)
        ok = reference == engine_count
        mismatches += not ok
        print(f"{name:<20} {depth:>5} {reference:>13} {engine_count:>13}  {'PASS' if ok else 'FAIL'}    {fen}")
    python_seconds = time.time() - start

    random_fens = [fen for name, fen, _ in cases if name.startswith("random_")]
    coverage = {}
    for fen in random_fens:
        for key, value in features(fen).items():
            coverage[key] = coverage.get(key, 0) + value
    print(f"\nrandom positions: {len(random_fens)}; root positions with ...")
    for key, count in coverage.items():
        print(f"  {key}: {count}")

    print(f"\ntime: engine {engine_seconds:.1f} s, python-chess {python_seconds:.1f} s")
    print(f"Summary: {len(cases) - mismatches} passed, {mismatches} failed")
    return mismatches


if __name__ == "__main__":
    sys.exit(main())
