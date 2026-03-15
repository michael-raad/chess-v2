#include "eval.hpp"
#include "position.hpp"
#include "search.hpp"
#include <limits>

namespace chess {

int evaluate(Position& position) {
    Color side_to_move = position.side_to_move();
    
    // Check for terminal positions
    if (is_checkmate(position, side_to_move)) {
        // Checkmate: side to move has lost - return very negative score
        // Using INT_MIN + 1 to safely negate in negamax without overflow
        return std::numeric_limits<int>::min() + 1;
    }
    
    if (is_stalemate(position, side_to_move)) {
        // Stalemate: draw position
        return 0;
    }

    // Check for 50-move rule draw
    // If halfmove clock >= 100, it's a draw by the 50-move rule
    if (position.halfmove_clock() >= 100) {
        return 0;
    }
    
    int score = 0;
    
    // Piece value table: indexed by Piece enum (0-11)
    // WP, WN, WB, WR, WQ, WK, BP, BN, BB, BR, BQ, BK
    const int piece_values[] = {
        PAWN_VALUE,   KNIGHT_VALUE, BISHOP_VALUE, ROOK_VALUE, QUEEN_VALUE, KING_VALUE,  // White
        PAWN_VALUE,   KNIGHT_VALUE, BISHOP_VALUE, ROOK_VALUE, QUEEN_VALUE, KING_VALUE   // Black
    };
    
    // Sum material for all pieces
    for (int piece = 0; piece < 12; piece++) {
        if (piece == WK || piece == BK) continue; // skip kings for material evaluation
        U64 bb = position.bitboard(static_cast<Piece>(piece));
        int piece_count = __builtin_popcountll(bb);
        int piece_value = piece_values[piece];
        if (piece_count == 0) continue;
        
        // White pieces (0-5) are positive, black pieces (6-11) are negative
        if (piece < 6) {
            score += piece_count * piece_value;
        } else {
            score -= piece_count * piece_value;
        }
    }
    
    // Return score from perspective of side to move
    if (side_to_move == BLACK) {
        score = -score;
    }
    
    return score;
}

}
