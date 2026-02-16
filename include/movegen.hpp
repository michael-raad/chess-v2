#pragma once
#include "position.hpp"

#include <vector>

namespace chess {

struct Move {
  int from;     // 0..63
  int to;       // 0..63
  int promo;    // 0 = no promotion, 1 = N, 2 = B, 3 = R, 4 = Q

  Move() : from(-1), to(-1), promo(0) {}
  Move(int f, int t, int p = 0) : from(f), to(t), promo(p) {}

  bool operator==(const Move &other) const {
    return from == other.from && to == other.to && promo == other.promo;
  }
};

class MoveGenerator {
public:
  explicit MoveGenerator(const Position &pos) : pos_(pos) {}

  // Generate all pseudo-legal moves (excludes moves leaving own king in check).
  std::vector<Move> generate_pseudo_legal();

  // Generate only capture moves (pseudo-legal).
  std::vector<Move> generate_captures();

private:
  const Position &pos_;

  void add_pawn_moves(std::vector<Move> &moves);
  void add_knight_moves(std::vector<Move> &moves);
  void add_bishop_moves(std::vector<Move> &moves);
  void add_rook_moves(std::vector<Move> &moves);
  void add_queen_moves(std::vector<Move> &moves);
  void add_king_moves(std::vector<Move> &moves);
  void add_castling_moves(std::vector<Move> &moves);

  // Sliding piece helpers
  void add_sliding_moves(std::vector<Move> &moves, U64 pieces, const std::vector<std::pair<int, int>> &directions);

  // Check if a move is a capture
  bool is_capture(int from, int to) const;
};

} // namespace chess
