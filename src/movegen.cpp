#include "movegen.hpp"
#include "attacks.hpp"

namespace chess {

std::vector<Move> MoveGenerator::generate_pseudo_legal() {
  std::vector<Move> moves;
  moves.reserve(256);

  add_pawn_moves(moves);
  add_knight_moves(moves);
  add_bishop_moves(moves);
  add_rook_moves(moves);
  add_queen_moves(moves);
  add_king_moves(moves);
  add_castling_moves(moves);

  return moves;
}

std::vector<Move> MoveGenerator::generate_captures() {
  std::vector<Move> moves;
  moves.reserve(128);

  add_pawn_moves(moves);
  add_knight_moves(moves);
  add_bishop_moves(moves);
  add_rook_moves(moves);
  add_queen_moves(moves);
  add_king_moves(moves);

  // Filter to captures only
  std::vector<Move> captures;
  for (const auto &m : moves) {
    if (is_capture(m.from, m.to)) {
      captures.push_back(m);
    }
  }
  return captures;
}

void MoveGenerator::add_pawn_moves(std::vector<Move> &moves) {
  Color us = pos_.side_to_move();
  Color them = (us == WHITE) ? BLACK : WHITE;
  int pawn_piece = (us == WHITE) ? WP : BP;
  int forward = (us == WHITE) ? 8 : -8; // row direction
  int rank_start = (us == WHITE) ? 8 : 48;    // starting rank (for double push)
  int rank_promo = (us == WHITE) ? 56 : 7;    // promotion rank

  U64 pawns = pos_.bitboard(static_cast<Piece>(pawn_piece));
  U64 empty = ~pos_.occupied();
  U64 them_occ = pos_.occupancy(them);
  int ep_sq = pos_.en_passant_square();

  while (pawns) {
    // Get index of least significant pawn and clear it from the bitboard
    // This makes iterating over pawns efficient (no need to check all 64 squares).
    int from = __builtin_ctzll(pawns);
    pawns &= pawns - 1;

    // Single push
    int to = from + forward;
    if (to >= 0 && to < 64 && (empty & (1ULL << to))) {
      if ((to / 8) == (rank_promo / 8)) {
        // Promotion
        moves.emplace_back(from, to, 1); // N
        moves.emplace_back(from, to, 2); // B
        moves.emplace_back(from, to, 3); // R
        moves.emplace_back(from, to, 4); // Q
      } else {
        moves.emplace_back(from, to, 0);
      }

      // Double push
      if ((rank_start >> 3) == (from >> 3)) { // on starting rank
        int to2 = from + 2 * forward; // to2's my word fam 
        if (empty & (1ULL << to2)) {
          moves.emplace_back(from, to2, 0);
        }
      }
    }

    // Captures (diagonals)
    for (int delta : {forward - 1, forward + 1}) {
      int cap_sq = from + delta;
      if (cap_sq >= 0 && cap_sq < 64 && (them_occ & (1ULL << cap_sq))) {
        // Avoid wrapping off board (file boundary)
        int f1 = from % 8, f2 = cap_sq % 8;
        if ((delta == forward - 1 && f2 == f1 - 1) || (delta == forward + 1 && f2 == f1 + 1)) {
          if ((cap_sq / 8) == (rank_promo / 8)) {
            moves.emplace_back(from, cap_sq, 1);
            moves.emplace_back(from, cap_sq, 2);
            moves.emplace_back(from, cap_sq, 3);
            moves.emplace_back(from, cap_sq, 4);
          } else {
            moves.emplace_back(from, cap_sq, 0);
          }
        }
      }
    }

    // En passant
    if (ep_sq >= 0) {
      int delta = (us == WHITE) ? 7 : -7;
      if (from + delta == ep_sq) {
        int f1 = from % 8, f2 = ep_sq % 8;
        if (std::abs(f1 - f2) == 1) {
          moves.emplace_back(from, ep_sq, 0);
        }
      }
      delta = (us == WHITE) ? 9 : -9;
      if (from + delta == ep_sq) {
        int f1 = from % 8, f2 = ep_sq % 8;
        if (std::abs(f1 - f2) == 1) {
          moves.emplace_back(from, ep_sq, 0);
        }
      }
    }
  }
}

void MoveGenerator::add_knight_moves(std::vector<Move> &moves) {
  Color us = pos_.side_to_move();
  int knight_piece = (us == WHITE) ? WN : BN;
  U64 knights = pos_.bitboard(static_cast<Piece>(knight_piece));
  U64 us_occ = pos_.occupancy(us);

  while (knights) {
    int from = __builtin_ctzll(knights);
    knights &= knights - 1;
    U64 targets = knight_attacks[from] & ~us_occ;

    while (targets) {
      int to = __builtin_ctzll(targets);
      targets &= targets - 1;
      moves.emplace_back(from, to, 0);
    }
  }
}

void MoveGenerator::add_bishop_moves(std::vector<Move> &moves) {
  Color us = pos_.side_to_move();
  int bishop_piece = (us == WHITE) ? WB : BB;
  U64 bishops = pos_.bitboard(static_cast<Piece>(bishop_piece));

  static const std::vector<std::pair<int, int>> dirs = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
  add_sliding_moves(moves, bishops, dirs);
}

void MoveGenerator::add_rook_moves(std::vector<Move> &moves) {
  Color us = pos_.side_to_move();
  int rook_piece = (us == WHITE) ? WR : BR;
  U64 rooks = pos_.bitboard(static_cast<Piece>(rook_piece));

  static const std::vector<std::pair<int, int>> dirs = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
  add_sliding_moves(moves, rooks, dirs);
}

void MoveGenerator::add_queen_moves(std::vector<Move> &moves) {
  Color us = pos_.side_to_move();
  int queen_piece = (us == WHITE) ? WQ : BQ;
  U64 queens = pos_.bitboard(static_cast<Piece>(queen_piece));

  static const std::vector<std::pair<int, int>> dirs = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}, {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
  add_sliding_moves(moves, queens, dirs);
}

void MoveGenerator::add_king_moves(std::vector<Move> &moves) {
  Color us = pos_.side_to_move();
  int king_piece = (us == WHITE) ? WK : BK;
  U64 king = pos_.bitboard(static_cast<Piece>(king_piece));
  U64 us_occ = pos_.occupancy(us);

  if (!king) return;
  int from = __builtin_ctzll(king);
  U64 targets = king_attacks[from] & ~us_occ;

  while (targets) {
    int to = __builtin_ctzll(targets);
    targets &= targets - 1;
    moves.emplace_back(from, to, 0);
  }
}

void MoveGenerator::add_castling_moves(std::vector<Move> &moves) {
  Color us = pos_.side_to_move();
  int castling = pos_.castling_rights();
  U64 occ = pos_.occupied();

  if (us == WHITE) {
    // King-side
    if ((castling & 1) && !(occ & 0x60ULL)) { // e1, f1, g1 free
      moves.emplace_back(4, 6, 0);
    }
    // Queen-side
    if ((castling & 2) && !(occ & 0x0EULL)) { // a1, b1, c1, d1 free
      moves.emplace_back(4, 2, 0);
    }
  } else {
    // King-side
    if ((castling & 4) && !(occ & 0x6000000000000000ULL)) {
      moves.emplace_back(60, 62, 0);
    }
    // Queen-side
    if ((castling & 8) && !(occ & 0x0E00000000000000ULL)) {
      moves.emplace_back(60, 58, 0);
    }
  }
}

void MoveGenerator::add_sliding_moves(std::vector<Move> &moves, U64 pieces, const std::vector<std::pair<int, int>> &directions) {
  Color us = pos_.side_to_move();
  Color them = (us == WHITE) ? BLACK : WHITE;
  U64 us_occ = pos_.occupancy(us);
  U64 them_occ = pos_.occupancy(them);

  while (pieces) {
    int from = __builtin_ctzll(pieces);
    pieces &= pieces - 1;
    int f = from % 8, r = from >> 3;

    for (auto [df, dr] : directions) {
      for (int dist = 1; dist < 8; ++dist) {
        int nf = f + dist * df;
        int nr = r + dist * dr;
        if (nf < 0 || nf > 7 || nr < 0 || nr > 7) break;
        int to = nr * 8 + nf;

        if (us_occ & (1ULL << to)) break; // blocked by own piece
        moves.emplace_back(from, to, 0);
        if (them_occ & (1ULL << to)) break; // capture
      }
    }
  }
}

bool MoveGenerator::is_capture(int from, int to) const {
  (void)from; // suppress unused warning
  return (pos_.piece_on_square(to) != NO_PIECE);
}

} // namespace chess
