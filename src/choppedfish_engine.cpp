#include "choppedfish_engine.hpp"
#include "search.hpp"
#include <limits>
#include <optional>
#include <random>
#include <vector>

namespace chess {

ChoppedfishEngine::ChoppedfishEngine(int max_depth)
    : max_depth_(max_depth) {}

MoveEvaluation ChoppedfishEngine::get_best_move(const Position& position, int depth, long long movetime_ms) {
    // ChoppedfishEngine uses its own fixed depth; depth and movetime_ms parameters are ignored
    (void)depth;
    (void)movetime_ms;
    Position pos_copy = position;  // Copy for move legality checking
    auto legal_moves = chess::get_legal_moves(pos_copy);
    
    if (legal_moves.empty()) {
        // No legal moves - should not happen in a valid position
        return MoveEvaluation{Move{0, 0, 0}, 0};
    }
    
    std::vector<Move> best_moves;  // Store all equally-best moves
    int best_score = std::numeric_limits<int>::min();
    
    int alpha = std::numeric_limits<int>::min();
    int beta = std::numeric_limits<int>::max();
    
    // Initialize search history with game history and current position
    auto search_history = position_history_;
    search_history[get_position_hash(pos_copy)]++;
    
    // Root level negamax search
    // Unlike recursive calls, we don't negate alpha/beta at root
    // Try each legal move and find the one(s) that lead to the best position
    for (const Move& move : legal_moves) {
        // Apply the move
        auto undo_info = pos_copy.apply_move(move.from, move.to, move.promo);
        if (!undo_info) continue; // Skip invalid moves
        
        // Track this position in the search history
        auto child_history = search_history;
        child_history[get_position_hash(pos_copy)]++;
        
        // After our move, it's opponent's turn
        // Call alphabeta with normal window and negate result to get our perspective
        int opponent_score = alphabeta(pos_copy, max_depth_ - 1, alpha, beta, child_history);
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
        return MoveEvaluation{legal_moves[0], 0};
    }
    
    // Choose one of the equally-best moves at random
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, best_moves.size() - 1);
    return MoveEvaluation{best_moves[dis(gen)], best_score};
}

int ChoppedfishEngine::evaluate(Position& position) {
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
    
    // Piece values (in centipawns for precision)
    constexpr int PAWN_VALUE = 100;
    constexpr int KNIGHT_VALUE = 320;
    constexpr int BISHOP_VALUE = 330;
    constexpr int ROOK_VALUE = 500;
    constexpr int QUEEN_VALUE = 900;
    
    // Piece value table: indexed by Piece enum (0-11)
    // WP, WN, WB, WR, WQ, WK, BP, BN, BB, BR, BQ, BK
    const int piece_values[] = {
        PAWN_VALUE,   KNIGHT_VALUE, BISHOP_VALUE, ROOK_VALUE, QUEEN_VALUE, 20000,  // White
        PAWN_VALUE,   KNIGHT_VALUE, BISHOP_VALUE, ROOK_VALUE, QUEEN_VALUE, 20000   // Black
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

int ChoppedfishEngine::alphabeta(Position& position, int depth, int alpha, int beta,
                                 std::unordered_map<uint64_t, int> position_history) {
    // Check threefold repetition at THIS depth in the search tree
    // Threefold repetition is automatic - game ends as a draw immediately
    if (!position_history.empty()) {
        uint64_t current_hash = get_position_hash(position);
        auto it = position_history.find(current_hash);
        if (it != position_history.end() && it->second >= 3) {
            return 0;  // Threefold repetition: automatic draw
        }
    }
    
    // Check 50-move rule - can result in checkmate or draw
    if (position.halfmove_clock() >= 100) {
        return this->evaluate(position);
    }
    
    auto legal_moves = chess::get_legal_moves(position);
    
    // Terminal node (checkmate or stalemate) or depth limit
    // Let evaluate() handle checkmate, stalemate, and 50-move rule
    if (legal_moves.empty()) {
        return this->evaluate(position);
    }
    
    if (depth == 0) {
        return this->evaluate(position);
    }
    
    // Negamax always maximizes from current player's perspective
    // Recursive calls are negated because they return opponent's perspective
    int max_eval = std::numeric_limits<int>::min();
    
    for (const Move& move : legal_moves) {
        auto undo_info = position.apply_move(move.from, move.to, move.promo);
        if (!undo_info) continue;
        
        // Track this position in the search history
        auto child_history = position_history;
        child_history[get_position_hash(position)]++;
        
        // Negamax with alpha-beta: negate window for opponent's perspective
        // Safely handle extreme bounds to avoid integer overflow
        int neg_alpha = (beta == std::numeric_limits<int>::max()) ? std::numeric_limits<int>::min() : -beta;
        int neg_beta = (alpha == std::numeric_limits<int>::min()) ? std::numeric_limits<int>::max() : -alpha;
        
        int eval = -alphabeta(position, depth - 1, neg_alpha, neg_beta, child_history);
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


