#pragma once
#include "position.hpp"

namespace chess {

// Pre-computed attack bitboards for knights and kings at each square (0..63).
extern U64 knight_attacks[64];
extern U64 king_attacks[64];

// Initialize attack tables (call once at startup).
void init_attack_tables();

} // namespace chess
