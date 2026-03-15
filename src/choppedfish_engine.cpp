#include "choppedfish_engine.hpp"
#include "search.hpp"
#include "eval.hpp"
#include <limits>
#include <optional>
#include <random>
#include <vector>

namespace chess {

ChoppedfishEngine::ChoppedfishEngine(int max_depth)
    : max_depth_(max_depth) {}

Move ChoppedfishEngine::get_best_move(const Position& position) {
    auto legal_moves = chess::get_legal_moves(const_cast<Position&>(const_cast<Position&>(position)));
    
    if (legal_moves.empty()) {
        // No legal moves - should not happen in a valid position
        return Move{0, 0, 0};
    }
    
    std::vector<Move> best_moves;  // Store all equally-best moves
    int best_score = std::numeric_limits<int>::min();
    
    Position pos_copy = position;
    int alpha = std::numeric_limits<int>::min();
    int beta = std::numeric_limits<int>::max();
    
    // Root level negamax search
    // Unlike recursive calls, we don't negate alpha/beta at root
    // Try each legal move and find the one(s) that lead to the best position
    for (const Move& move : legal_moves) {
        // Apply the move
        auto undo_info = pos_copy.apply_move(move.from, move.to, move.promo);
        if (!undo_info) continue; // Skip invalid moves
        
        // After our move, it's opponent's turn
        // Call alphabeta with normal window and negate result to get our perspective
        int opponent_score = alphabeta(pos_copy, max_depth_ - 1, alpha, beta);
        int score = -opponent_score;
        
        // Undo the move
        pos_copy.undo_move(undo_info.value());
        
        // Track moves with the best score
        if (score > best_score) {
            // Found a better move, clear previous best moves and start over
            best_score = score;
            best_moves.clear();
            best_moves.push_back(move);
        } else if (score == best_score && !best_moves.empty()) {
            // Found an equally good move, add it to the list
            best_moves.push_back(move);
        }
        
        alpha = std::max(alpha, score);
        
        // Beta pruning at root
        if (alpha >= beta) {
            break;
        }
    }
    
    // If no moves were added to best_moves (shouldn't happen), return first legal move
    if (best_moves.empty()) {
        return legal_moves[0];
    }
    
    // Choose one of the equally-best moves at random
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, best_moves.size() - 1);
    return best_moves[dis(gen)];
}

int ChoppedfishEngine::alphabeta(Position& position, int depth, int alpha, int beta) {
    // Base case: reached max depth
    if (depth == 0) {
        return evaluate(position);
    }
    
    if (position.halfmove_clock() >= 100) {
        // Draw by 50-move rule
        return evaluate(position); // Should return 0 for draw, but evaluate also handles terminal positions
    }
    
    auto legal_moves = chess::get_legal_moves(position);
    
    // Terminal node (checkmate or stalemate)
    // Negamax ensures perspective is correct: evaluate returns from current side's perspective,
    // and we negate it when returning to parent, so parent gets their own perspective
    if (legal_moves.empty()) {
        return evaluate(position);
    }
    
    // Negamax always maximizes from current player's perspective
    // Recursive calls are negated because they return opponent's perspective
    int max_eval = std::numeric_limits<int>::min();
    
    for (const Move& move : legal_moves) {
        auto undo_info = position.apply_move(move.from, move.to, move.promo);
        if (!undo_info) continue;
        
        // Negamax with alpha-beta: negate window for opponent's perspective
        // Safely handle extreme bounds to avoid integer overflow
        int neg_alpha = (beta == std::numeric_limits<int>::max()) ? std::numeric_limits<int>::min() : -beta;
        int neg_beta = (alpha == std::numeric_limits<int>::min()) ? std::numeric_limits<int>::max() : -alpha;
        
        int eval = -alphabeta(position, depth - 1, neg_alpha, neg_beta);
        position.undo_move(undo_info.value());
        
        max_eval = std::max(max_eval, eval);
        alpha = std::max(alpha, eval);
        
        if (beta <= alpha) {
            break; // Beta cutoff
        }
    }
    
    return max_eval;
}

}


