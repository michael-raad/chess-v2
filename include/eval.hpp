#pragma once

#include "position.hpp"

namespace chess {

// Piece values (in centipawns for precision)
constexpr int PAWN_VALUE = 100;
constexpr int KNIGHT_VALUE = 300;
constexpr int BISHOP_VALUE = 325;
constexpr int ROOK_VALUE = 500;
constexpr int QUEEN_VALUE = 900;
constexpr int KING_VALUE = 0; // King is never scored in position evaluation

// Evaluate a position from the perspective of the side to move.
// Positive scores favor the side to move, negative favors the opponent.
// Terminal positions: checkmate returns very negative (side to move lost), stalemate returns 0.
int evaluate(Position& position);

}
