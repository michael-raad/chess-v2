#include "attacks.hpp"

#include <cstdlib>

namespace chess {

U64 knight_attacks[64];
U64 king_attacks[64];

void init_attack_tables() {
  // Initialize knight attacks
  for (int sq = 0; sq < 64; ++sq) {
    U64 atk = 0;
    int f = sq % 8, r = sq / 8;

    // All 8 knight deltas (2 squares in one direction, 1 in perpendicular)
    static const int knight_deltas[] = {-17, -15, -10, -6, 6, 10, 15, 17};
    for (int delta : knight_deltas) {
      int to = sq + delta;
      if (to >= 0 && to < 64) {
        int tf = to % 8, tr = to / 8;
        // Verify knight stays within 2 files of origin (prevents wrapping)
        if (std::abs(f - tf) <= 2 && std::abs(r - tr) <= 2) {
          atk |= (1ULL << to);
        }
      }
    }
    knight_attacks[sq] = atk;
  }

  // Initialize king attacks
  for (int sq = 0; sq < 64; ++sq) {
    U64 atk = 0;
    int f = sq % 8, r = sq / 8;

    for (int dr = -1; dr <= 1; ++dr) {
      for (int df = -1; df <= 1; ++df) {
        if (dr == 0 && df == 0) continue;
        int nf = f + df, nr = r + dr;
        if (nf >= 0 && nf < 8 && nr >= 0 && nr < 8) {
          int to = nr * 8 + nf;
          atk |= (1ULL << to);
        }
      }
    }
    king_attacks[sq] = atk;
  }
}

} // namespace chess
