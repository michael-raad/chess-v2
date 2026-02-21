#pragma once
#include "position.hpp"
#include "movegen.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace chess {

// Perft statistics: node counts at a given depth with move type breakdowns.
struct PerftStats {
  uint64_t nodes = 0;        // total nodes (positions)
  uint64_t captures = 0;     // capture moves
  uint64_t en_passants = 0;  // en passant captures
  uint64_t castles = 0;      // castling moves
  uint64_t promotions = 0;   // pawn promotions (including captures)
  uint64_t checks = 0;       // positions leaving opponent in check
  uint64_t discovery_checks = 0;  // moves giving discovered check
  uint64_t double_checks = 0;     // moves giving double check
  uint64_t checkmates = 0;   // positions that are checkmate

  PerftStats &operator+=(const PerftStats &other) {
    nodes += other.nodes;
    captures += other.captures;
    en_passants += other.en_passants;
    castles += other.castles;
    promotions += other.promotions;
    checks += other.checks;
    discovery_checks += other.discovery_checks;
    double_checks += other.double_checks;
    checkmates += other.checkmates;
    return *this;
  }

  // Print stats in a readable format
  void print(const std::string &label = "", int depth = -1) const {
    std::cout << "=== Perft Stats";
    if (!label.empty()) std::cout << " (" << label << ")";
    if (depth >= 0) std::cout << " Depth " << depth;
    std::cout << " ===" << std::endl;
    std::cout << "  Nodes:         " << nodes << std::endl;
    std::cout << "  Captures:      " << captures << std::endl;
    std::cout << "  En Passants:   " << en_passants << std::endl;
    std::cout << "  Castles:       " << castles << std::endl;
    std::cout << "  Promotions:    " << promotions << std::endl;
    std::cout << "  Checks:        " << checks << std::endl;
    std::cout << "  Disc Checks:   " << discovery_checks << std::endl;
    std::cout << "  Double Checks: " << double_checks << std::endl;
    std::cout << "  Checkmates:    " << checkmates << std::endl;
  }
};

// Perft driver: counts nodes, captures, and other stats to a given depth.
// Depth 0 returns 1 node (the current position).
PerftStats perft(Position &pos, int depth);

// Perft breakdown by first move (Stockfish-style).
// Prints nodes explored for each legal first move at the given depth.
void perft_by_move(Position &pos, int depth);

// Helper: check if a side's king is in check
bool is_in_check(const Position &pos, Color side);

// Helper: check if a side is in checkmate
bool is_checkmate(Position &pos, Color side);

// Helper: check if a side is in stalemate (not in check, but no legal moves)
bool is_stalemate(Position &pos, Color side);

// Helper: check if the position is a draw by 50-move rule
bool is_draw_by_50_move_rule(const Position &pos);

// Helper: check if a castling move is legal (king doesn't move through check)
bool is_castling_legal(const Position &pos, int from, int to);

// Generate all legal moves (pseudo-legal moves that don't leave king in check)
std::vector<Move> get_legal_moves(Position &pos);

} // namespace chess
