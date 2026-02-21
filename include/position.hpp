#pragma once
#include <cstdint>
#include <string>
#include <array>
#include <optional>

namespace chess {

using U64 = uint64_t; // Unsigned 64-bit integer

enum Color : int { WHITE = 0, BLACK = 1 };

enum PieceType : int { PAWN = 0, KNIGHT, BISHOP, ROOK, QUEEN, KING };

enum Piece : int {
  WP = 0, WN, WB, WR, WQ, WK,
  BP, BN, BB, BR, BQ, BK,
  NO_PIECE = -1
};

enum class PlayerType { HUMAN, AI };

inline Color piece_color(Piece p) { return p < 6 ? WHITE : BLACK; }
inline PieceType piece_type(Piece p) { return PieceType(p % 6); }

// Represents a chess position, including piece placement, side to move, castling rights, en passant square, and move counters.
class Position {
public:
  Position();
  void clear();

  // Parse FEN. Returns true on success; on failure, returns false and fills err if provided.
  bool set_from_fen(const std::string &fen, std::string *err = nullptr);
  std::optional<std::string> get_fen() const;

  // Accessors
  U64 bitboard(Piece p) const { return bitboards_[static_cast<int>(p)]; }
  U64 occupancy(Color c) const;
  U64 occupied() const;

  Color side_to_move() const { return side_; }
  int en_passant_square() const { return ep_square_; } // -1 if none
  int castling_rights() const { return castling_; }    // bitmask: WK=1,WQ=2,BK=4,BQ=8
  int halfmove_clock() const { return halfmove_; }    // halfmove clock for 50-move rule

  // query piece on square (0..63). Returns NO_PIECE if empty.
  int piece_on_square(int sq) const;

  static int square_index(char file, char rank); // file 'a'..'h', rank '1'..'8'

  // Move application and undo
  // A UnmoveInfo stores state needed to undo a move.
  struct UnmoveInfo {
    int from, to, promo;
    int captured_piece;
    int old_castling, old_ep_sq, old_halfmove;
    bool was_ep_capture; // true if this was an en passant capture
  };

  // Apply a move (from, to, promo). Assumes the move is legal (from a legal-move list).
  // Returns undo info or empty on invalid move.
  std::optional<UnmoveInfo> apply_move(int from, int to, int promo = 0);

  // Undo a previously applied move.
  void undo_move(const UnmoveInfo &info);

  // Friend for castling legality check
  friend bool is_castling_legal(const Position &pos, int from, int to);

private:
  // The underscore suffix marks private members and avoids ambiguity with method names.
  // bitboards_[12] holds bitboards for each piece type (WP, WN, ..., BK).
  // Each bitboard has a 1 in the position of squares occupied by that piece type.
  std::array<U64, 12> bitboards_{};
  Color side_ = WHITE; // side to move
  int ep_square_ = -1; // en passant target square (0..63), or -1 if none
  int castling_ = 0; // castling rights bitmask: WK=1, WQ=2, BK=4, BQ=8
  int halfmove_ = 0; // halfmove clock for 50-move rule
  int fullmove_ = 1; // fullmove number, starting at 1 and incremented after Black's move
};

} // namespace chess
