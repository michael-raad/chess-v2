#include "search.hpp"
#include "movegen.hpp"

namespace chess {

bool is_in_check(const Position &pos, Color side) {
  int king_piece = (side == WHITE) ? WK : BK;
  U64 king = pos.bitboard(static_cast<Piece>(king_piece));
  if (!king) return false;

  int king_sq = __builtin_ctzll(king);

  // Check if any opponent piece attacks the king square
  // Pawn attacks: opponent pawns attack diagonally in their move direction
  int opp_pawn = (side == WHITE) ? BP : WP;
  U64 opp_pawns = pos.bitboard(static_cast<Piece>(opp_pawn));
  // If side==WHITE, opponent pawns are BLACK, which attack -7 and -9
  // If side==BLACK, opponent pawns are WHITE, which attack +7 and +9
  int attack_delta_1 = (side == WHITE) ? -7 : 7;
  int attack_delta_2 = (side == WHITE) ? -9 : 9;
  int king_file = king_sq % 8;
  while (opp_pawns) {
    int pawn_sq = __builtin_ctzll(opp_pawns);
    int pawn_file = pawn_sq % 8;
    opp_pawns &= opp_pawns - 1;
    // Check file boundaries to prevent wrapping
    if (pawn_sq + attack_delta_1 == king_sq && std::abs(pawn_file - king_file) == 1) {
      return true;
    }
    if (pawn_sq + attack_delta_2 == king_sq && std::abs(pawn_file - king_file) == 1) {
      return true;
    }
  }

  // Knight attacks
  extern U64 knight_attacks[64];
  int opp_knight = (side == WHITE) ? BN : WN;
  U64 opp_knights = pos.bitboard(static_cast<Piece>(opp_knight));
  while (opp_knights) {
    int knight_sq = __builtin_ctzll(opp_knights);
    opp_knights &= opp_knights - 1;
    if (knight_attacks[knight_sq] & (1ULL << king_sq)) {
      return true;
    }
  }

  // King attacks
  extern U64 king_attacks[64];
  int opp_king = (side == WHITE) ? BK : WK;
  U64 opp_king_bb = pos.bitboard(static_cast<Piece>(opp_king));
  if (opp_king_bb) {
    int opp_king_sq = __builtin_ctzll(opp_king_bb);
    if (king_attacks[opp_king_sq] & (1ULL << king_sq)) {
      return true;
    }
  }

  // Sliding pieces (bishop/queen diagonals, rook/queen orthogonals)
  U64 occ = pos.occupied();
  auto check_sliding = [&](int piece_idx, const std::vector<std::pair<int, int>> &dirs) {
    U64 pieces = pos.bitboard(static_cast<Piece>(piece_idx));
    while (pieces) {
      int piece_sq = __builtin_ctzll(pieces);
      pieces &= pieces - 1;
      int f = piece_sq % 8, r = piece_sq / 8;

      for (auto [df, dr] : dirs) {
        for (int dist = 1; dist < 8; ++dist) {
          int nf = f + dist * df, nr = r + dist * dr;
          if (nf < 0 || nf > 7 || nr < 0 || nr > 7) break;
          int to = nr * 8 + nf;
          if (to == king_sq) return true;
          if (occ & (1ULL << to)) break;
        }
      }
    }
    return false;
  };

  // Bishop
  int opp_bishop = (side == WHITE) ? BB : WB;
  auto bishop_dirs = std::vector<std::pair<int, int>>{{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
  if (check_sliding(opp_bishop, bishop_dirs)) return true;

  // Rook
  int opp_rook = (side == WHITE) ? BR : WR;
  auto rook_dirs = std::vector<std::pair<int, int>>{{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
  if (check_sliding(opp_rook, rook_dirs)) return true;

  // Queen
  int opp_queen = (side == WHITE) ? BQ : WQ;
  auto queen_dirs = std::vector<std::pair<int, int>>{{0, -1}, {0, 1}, {-1, 0}, {1, 0}, {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
  if (check_sliding(opp_queen, queen_dirs)) return true;

  return false;
}

bool is_checkmate(Position &pos, Color side) {
  if (!is_in_check(pos, side)) return false;

  MoveGenerator gen(pos);
  auto moves = gen.generate_pseudo_legal();

  for (const auto &m : moves) {
    auto undo_info = pos.apply_move(m.from, m.to, m.promo);
    if (undo_info) {
      bool legal = !is_in_check(pos, side);
      pos.undo_move(*undo_info);
      if (legal) return false; // found a legal move
    }
  }
  return true; // no legal moves
}

bool is_castling_legal(const Position &pos, int from, int to) {
  Color us = pos.side_to_move();
  
  // Castling is only valid for king moves of 2 squares
  if (std::abs(to - from) != 2) return true; // not a castling move
  
  // Check if it's actually a king move
  int piece = pos.piece_on_square(from);
  if (piece != WK && piece != BK) return true; // not a king
  
  // King cannot be in check originally
  if (is_in_check(pos, us)) return false;
  
  // Build a temporary position to check if intermediate square is under attack
  Position temp = pos;
  
  // Determine intermediate square(s) to check
  int mid_sq = (from + to) / 2;  // intermediate square
  
  // Move king to intermediate square temporarily to check if it's attacked
  temp.bitboards_[piece] &= ~(1ULL << from);
  temp.bitboards_[piece] |= (1ULL << mid_sq);
  
  if (is_in_check(temp, us)) {
    return false;  // intermediate square under attack
  }
  
  // Move king to destination to check if it's attacked
  temp.bitboards_[piece] &= ~(1ULL << mid_sq);
  temp.bitboards_[piece] |= (1ULL << to);
  
  if (is_in_check(temp, us)) {
    return false;  // destination under attack
  }
  
  return true;  // castling is legal
}

PerftStats perft(Position &pos, int depth) {
  if (depth == 0) {
    PerftStats s;
    s.nodes = 1;
    return s;  // base case: leaf node
  }

  PerftStats stats;
  MoveGenerator gen(pos);
  auto moves = gen.generate_pseudo_legal();

  for (const auto &m : moves) {
    auto undo_info = pos.apply_move(m.from, m.to, m.promo);
    if (!undo_info) continue;

    // Check legality: king must not be in check after the move
    bool legal = !is_in_check(pos, static_cast<Color>(pos.side_to_move() ^ 1));
    
    // Additional check for castling: verify intermediate squares aren't under attack
    if (legal && std::abs(m.to - m.from) == 2) {
      int piece = pos.piece_on_square(m.to);
      if (piece == WK || piece == BK) {
        // Undo to check castling legality before applying again
        pos.undo_move(*undo_info);
        legal = is_castling_legal(pos, m.from, m.to);
        if (!legal) continue;
        // Re-apply the move
        undo_info = pos.apply_move(m.from, m.to, m.promo);
        if (!undo_info) continue;
      }
    }
    
    if (!legal) {
      pos.undo_move(*undo_info);
      continue;
    }

    // Track stats for THIS move (always, regardless of depth)
    if (undo_info->captured_piece != NO_PIECE || undo_info->was_ep_capture) {
      stats.captures++;
    }
    if (undo_info->was_ep_capture) {
      stats.en_passants++;
    }
    if (m.promo > 0) {
      stats.promotions++;
    }
    // Castling: king moves 2 squares
    int piece = pos.piece_on_square(m.to);
    if ((piece == WK || piece == BK) && std::abs(m.to - m.from) == 2) {
      stats.castles++;
    }
    // Check/checkmate detection
    if (is_in_check(pos, pos.side_to_move())) {
      stats.checks++;
      if (is_checkmate(pos, pos.side_to_move())) {
        stats.checkmates++;
      }
    }

    // Recurse
    PerftStats sub = perft(pos, depth - 1);
    stats += sub;

    pos.undo_move(*undo_info);
  }

  return stats;
}

void perft_by_move(Position &pos, int depth) {
  if (depth == 0) return;

  std::cout << "\nPerft by move (depth " << depth << "):" << std::endl;
  std::cout << "Move\t\tNodes" << std::endl;
  std::cout << "----\t\t-----" << std::endl;

  MoveGenerator gen(pos);
  auto moves = gen.generate_pseudo_legal();
  uint64_t total_nodes = 0;

  for (const auto &m : moves) {
    auto undo_info = pos.apply_move(m.from, m.to, m.promo);
    if (!undo_info) continue;

    // Check legality: king must not be in check after the move
    bool legal = !is_in_check(pos, static_cast<Color>(pos.side_to_move() ^ 1));
    
    // Additional check for castling
    if (legal && std::abs(m.to - m.from) == 2) {
      int piece = pos.piece_on_square(m.to);
      if (piece == WK || piece == BK) {
        pos.undo_move(*undo_info);
        legal = is_castling_legal(pos, m.from, m.to);
        if (!legal) continue;
        undo_info = pos.apply_move(m.from, m.to, m.promo);
        if (!undo_info) continue;
      }
    }
    
    if (!legal) {
      pos.undo_move(*undo_info);
      continue;
    }

    // Recurse for remaining depth
    PerftStats sub = perft(pos, depth - 1);
    total_nodes += sub.nodes;

    // Print move and node count
    std::cout << (char)('a' + (m.from % 8)) << (char)('1' + (m.from / 8))
              << (char)('a' + (m.to % 8)) << (char)('1' + (m.to / 8));
    if (m.promo > 0) {
      const char *promo_chars = "nbrq";
      std::cout << promo_chars[m.promo - 1];
    }
    std::cout << "\t\t" << sub.nodes << std::endl;

    pos.undo_move(*undo_info);
  }

  std::cout << "----\t\t-----" << std::endl;
  std::cout << "Total\t\t" << total_nodes << std::endl;
}

} // namespace chess
