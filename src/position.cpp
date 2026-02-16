#include "position.hpp"

#include <cctype>
#include <sstream>

namespace chess {

Position::Position() { clear(); }

void Position::clear() {
  bitboards_.fill(0ULL);
  side_ = WHITE;
  ep_square_ = -1;
  castling_ = 0;
  halfmove_ = 0;
  fullmove_ = 1;
}

static int file_rank_to_sq(int file, int rank) { return rank * 8 + file; }

int Position::square_index(char file, char rank) {
  if (file < 'a' || file > 'h' || rank < '1' || rank > '8') return -1;
  int f = file - 'a';
  int r = rank - '1';
  return file_rank_to_sq(f, r);
}

bool Position::set_from_fen(const std::string &fen, std::string *err) {
  clear();
  std::istringstream ss(fen);
  std::string piece_part, side_part, castling_part, ep_part, halfmove_s, fullmove_s;
  if (!(ss >> piece_part)) {
    if (err) {
      *err = "Empty FEN";
    }
    return false;
  }

  // piece placement
  int sq = 56; // start at a8
  for (char c : piece_part) {
    if (c == '/') {
      sq -= 16; // move to next rank
      continue;
    }
    if (std::isdigit((unsigned char)c)) {
      int skip = c - '0';
      sq += skip;
      continue;
    }
    int piece = -1;
    switch (c) {
      case 'P': piece = WP; break;
      case 'N': piece = WN; break;
      case 'B': piece = WB; break;
      case 'R': piece = WR; break;
      case 'Q': piece = WQ; break;
      case 'K': piece = WK; break;
      case 'p': piece = BP; break;
      case 'n': piece = BN; break;
      case 'b': piece = BB; break;
      case 'r': piece = BR; break;
      case 'q': piece = BQ; break;
      case 'k': piece = BK; break;
      default:
        if (err) {
          *err = "Invalid FEN piece char";
        }
        return false;
    }
    if (sq < 0 || sq > 63) { 
      if (err) {
        *err = "Square out of range"; 
      }
      return false;
    }
    bitboards_[piece] |= (1ULL << sq);
    ++sq;
  }

  // side to move
  if (!(ss >> side_part)) { 
    if (err) {
      *err = "Missing side to move"; 
    }
    return false; 
  }
  side_ = (side_part == "w") ? WHITE : BLACK;

  // castling
  if (!(ss >> castling_part)) { 
    if (err) {
      *err = "Missing castling part"; 
    }
    return false; 
  }
  castling_ = 0;
  if (castling_part != "-") {
    for (char c : castling_part) {
      if (c == 'K') castling_ |= 1;
      else if (c == 'Q') castling_ |= 2;
      else if (c == 'k') castling_ |= 4;
      else if (c == 'q') castling_ |= 8;
    }
  }
  

  // en passant
  if (!(ss >> ep_part)) { 
    if (err) {
      *err = "Missing ep part"; 
    }
    return false; 
  }
  if (ep_part == "-") ep_square_ = -1;
  else {
    if (ep_part.size() != 2) { 
      if (err) {
        *err = "Invalid ep";
      } 
      return false; 
    }
    ep_square_ = square_index(ep_part[0], ep_part[1]);
    if (ep_square_ < 0) { 
      if (err) {
        *err = "Invalid ep square"; 
      }
      return false; 
    }
  }

  // halfmove and fullmove
  if (!(ss >> halfmove_s >> fullmove_s)) {
    // allow omission: default 0/1
    halfmove_ = 0;
    fullmove_ = 1;
  } else {
    try { 
      halfmove_ = std::stoi(halfmove_s); 
      fullmove_ = std::stoi(fullmove_s); 
    }
    catch (...) { 
      if (err) {
        *err = "Invalid move counters";
      } 
      return false; 
    }
  }

  return true;
}

std::optional<std::string> Position::get_fen() const {
  // Minimal implementation: piece placement + side + castling + ep + counters
  std::string out;
  out.reserve(80);
  for (int rank = 7; rank >= 0; --rank) {
    int empty = 0;
    for (int file = 0; file < 8; ++file) {
      int sq = file_rank_to_sq(file, rank);
      int p = piece_on_square(sq);
      if (p == NO_PIECE) { ++empty; }
      else {
        if (empty) { 
          out.push_back(char('0' + empty)); 
          empty = 0; 
        }
        char c = '?';
        switch (p) {
          case WP: c = 'P'; break; 
          case WN: c = 'N'; break; 
          case WB: c = 'B'; break; 
          case WR: c = 'R'; break; 
          case WQ: c = 'Q'; break; 
          case WK: c = 'K'; break;
          case BP: c = 'p'; break; 
          case BN: c = 'n'; break; 
          case BB: c = 'b'; break; 
          case BR: c = 'r'; break; 
          case BQ: c = 'q'; break; 
          case BK: c = 'k'; break;
        }
        out.push_back(c);
      }
    }
    if (empty) out.push_back(char('0' + empty));
    if (rank) out.push_back('/');
  }
  out.push_back(' ');
  out.push_back(side_ == WHITE ? 'w' : 'b');
  out.push_back(' ');
  if (castling_ == 0) out += '-'; else {
    if (castling_ & 1) out.push_back('K');
    if (castling_ & 2) out.push_back('Q');
    if (castling_ & 4) out.push_back('k');
    if (castling_ & 8) out.push_back('q');
  }
  out.push_back(' ');
  if (ep_square_ < 0) {
    out += '-';
   } 
   else {
    int f = ep_square_ % 8; 
    int r = ep_square_ / 8; 
    out.push_back(char('a' + f)); 
    out.push_back(char('1' + r));
  }
  out += ' ' + std::to_string(halfmove_) + ' ' + std::to_string(fullmove_);
  return out;
}

U64 Position::occupied() const {
  U64 occ = 0ULL;
  for (auto b : bitboards_) occ |= b;
  return occ;
}

U64 Position::occupancy(Color c) const {
  U64 occ = 0ULL;
  int start = (c == WHITE) ? WP : BP;
  for (int i = 0; i < 6; ++i) occ |= bitboards_[start + i];
  return occ;
}

int Position::piece_on_square(int sq) const {
  if (sq < 0 || sq > 63) return NO_PIECE;
  U64 mask = 1ULL << sq;
  for (int p = 0; p < 12; ++p) if (bitboards_[p] & mask) return p;
  return NO_PIECE;
}

std::optional<Position::UnmoveInfo> Position::apply_move(int from, int to, int promo) {
  if (from < 0 || from > 63 || to < 0 || to > 63) return {};

  int piece = piece_on_square(from);
  int captured = piece_on_square(to);
  if (piece == NO_PIECE) return {};

  // Disallow moving opponent pieces
  Color us = side_;
  bool is_white_piece = (piece >= WP && piece <= WK);
  if ((us == WHITE) != is_white_piece) return {};

  UnmoveInfo info{from, to, promo, captured, castling_, ep_square_, halfmove_, false};

  // Remove moving piece
  bitboards_[piece] &= ~(1ULL << from);

  // Place piece (with promotion if applicable)
  int final_piece = piece;
  if (promo > 0) {
    // Pawn promotion
    int rank = to / 8;
    if (!((us == WHITE && rank == 7) || (us == BLACK && rank == 0))) return {}; // not on promo rank
    int base = (us == WHITE) ? WN : BN;
    final_piece = base + promo - 1; // promo 1=N, 2=B, 3=R, 4=Q
  }
  bitboards_[final_piece] |= (1ULL << to);

  // Remove captured piece
  if (captured != NO_PIECE) {
    bitboards_[captured] &= ~(1ULL << to);
    halfmove_ = 0;
  } else {
    ++halfmove_;
  }

  // Handle pawn double-push (en passant setup)
  ep_square_ = -1;
  if (piece == WP || piece == BP) {
    halfmove_ = 0; // pawn move resets halfmove clock
    if (std::abs(to - from) == 16) {
      // Double push: set ep square to the square the pawn "passed over"
      ep_square_ = (from + to) / 2;
    } else if (captured == NO_PIECE && (to % 8) != (from % 8)) {
      // En passant capture
      info.was_ep_capture = true;
      int ep_victim = (us == WHITE) ? (to - 8) : (to + 8);
      bitboards_[piece_on_square(ep_victim)] &= ~(1ULL << ep_victim);
    }
  }

  // Update castling rights
  if (piece == WK) castling_ &= ~3;   // lose WK and WQ rights
  if (piece == BK) castling_ &= ~12;  // lose BK and BQ rights
  if (piece == WR) {
    if (from == 0) castling_ &= ~2;   // a1
    if (from == 7) castling_ &= ~1;   // h1
  }
  if (piece == BR) {
    if (from == 56) castling_ &= ~8;  // a8
    if (from == 63) castling_ &= ~4;  // h8
  }
  if (captured == WR) {
    if (to == 0) castling_ &= ~2;
    if (to == 7) castling_ &= ~1;
  }
  if (captured == BR) {
    if (to == 56) castling_ &= ~8;
    if (to == 63) castling_ &= ~4;
  }

  // Handle castling move (special case)
  if (piece == WK && from == 4) {
    if (to == 6) { // King-side castling
      bitboards_[WR] &= ~(1ULL << 7);
      bitboards_[WR] |= (1ULL << 5);
    } else if (to == 2) { // Queen-side castling
      bitboards_[WR] &= ~1ULL;
      bitboards_[WR] |= (1ULL << 3);
    }
  }
  if (piece == BK && from == 60) {
    if (to == 62) { // King-side castling
      bitboards_[BR] &= ~(1ULL << 63);
      bitboards_[BR] |= (1ULL << 61);
    } else if (to == 58) { // Queen-side castling
      bitboards_[BR] &= ~(1ULL << 56);
      bitboards_[BR] |= (1ULL << 59);
    }
  }

  // Toggle side
  side_ = (side_ == WHITE) ? BLACK : WHITE;
  if (side_ == WHITE) ++fullmove_;

  return info;
}

void Position::undo_move(const UnmoveInfo &info) {
  side_ = (side_ == WHITE) ? BLACK : WHITE;
  if (side_ == BLACK) --fullmove_;

  int piece = piece_on_square(info.to);
  if (piece == NO_PIECE) return; // Should not happen

  // Restore piece to original square
  bitboards_[piece] &= ~(1ULL << info.to);
  
  // Handle promotion: restore original pawn
  int restored_piece = piece;
  if (info.promo > 0) {
    Color us = side_;
    restored_piece = (us == WHITE) ? WP : BP;
  }
  bitboards_[restored_piece] |= (1ULL << info.from);

  // Restore captured piece
  if (info.captured_piece != NO_PIECE) {
    bitboards_[info.captured_piece] |= (1ULL << info.to);
  }

  // Restore state
  ep_square_ = info.old_ep_sq;
  castling_ = info.old_castling;
  halfmove_ = info.old_halfmove;

  // Undo castling rook moves (mirror of apply_move)
  if (restored_piece == WK && info.from == 4) {
    if (info.to == 6) {
      bitboards_[WR] &= ~(1ULL << 5);
      bitboards_[WR] |= (1ULL << 7);
    } else if (info.to == 2) {
      bitboards_[WR] &= ~(1ULL << 3);
      bitboards_[WR] |= 1ULL;
    }
  }
  if (restored_piece == BK && info.from == 60) {
    if (info.to == 62) {
      bitboards_[BR] &= ~(1ULL << 61);
      bitboards_[BR] |= (1ULL << 63);
    } else if (info.to == 58) {
      bitboards_[BR] &= ~(1ULL << 59);
      bitboards_[BR] |= (1ULL << 56);
    }
  }

  // Restore en passant captured pawn if applicable
  if (info.was_ep_capture) {
    int ep_victim_sq = (restored_piece == WP) ? (info.to - 8) : (info.to + 8);
    int ep_victim = (restored_piece == WP) ? BP : WP;
    bitboards_[ep_victim] |= (1ULL << ep_victim_sq);
  }
}

} // namespace chess

