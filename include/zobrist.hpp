#pragma once

#include "position.hpp"
#include <cstdint>

namespace chess {

// Zobrist hashing for position fingerprinting
// Used for threefold repetition detection

// Generate a 64-bit hash for a position (excludes en passant square per FIDE rules)
// Includes: piece placement, side to move, castling rights
uint64_t get_position_hash(const Position& pos);

} // namespace chess
