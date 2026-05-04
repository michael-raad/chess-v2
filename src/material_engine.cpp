#include "material_engine.hpp"
#include "search.hpp"
#include <limits>
#include <optional>
#include <random>
#include <vector>
#include <chrono>
#include <iostream>

namespace chess {

static std::string move_to_uci(const Move& move) {
    std::string out;
    out += static_cast<char>('a' + (move.from % 8));
    out += static_cast<char>('1' + (move.from / 8));
    out += static_cast<char>('a' + (move.to % 8));
    out += static_cast<char>('1' + (move.to / 8));
    if (move.promo != 0) {
        switch (move.promo) {
            case 1: out += 'n'; break;
            case 2: out += 'b'; break;
            case 3: out += 'r'; break;
            case 4: out += 'q'; break;
            default: break;
        }
    }
    return out;
}

bool MaterialEngine::should_stop_search() {
    if (stop_flag_ && stop_flag_->load()) {
        timed_out_ = true;
        return true;
    }

    if (!use_time_limit_) {
        return false;
    }

    ++node_counter_;
    // Amortize clock reads to avoid heavy overhead.
    if ((node_counter_ & 1023ULL) != 0) {
        return false;
    }

    if (std::chrono::steady_clock::now() >= deadline_) {
        timed_out_ = true;
        return true;
    }

    return false;
}

MoveEvaluation MaterialEngine::get_best_move(const Position& position, int depth, long long movetime_ms) {
    int max_depth = std::max(1, depth);

    use_time_limit_ = movetime_ms > 0;
    timed_out_ = false;
    node_counter_ = 0;
    if (use_time_limit_) {
        deadline_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(movetime_ms);
    }

    Position root_copy = position;  // Copy for move legality checking
    auto legal_moves = chess::get_legal_moves(root_copy);
    if (legal_moves.empty()) {
        return MoveEvaluation{Move{0, 0, 0}, 0};
    }

    // Keep last fully completed depth result as the return value.
    MoveEvaluation best_completed{legal_moves[0], 0};
    int reached_depth = 0;

    // Iterative deepening is engine-owned.
    for (int d = 1; d <= max_depth; ++d) {
        if (should_stop_search()) {
            break;
        }

        std::vector<Move> best_moves;
        int best_score = std::numeric_limits<int>::min();
        int alpha = std::numeric_limits<int>::min();
        int beta = std::numeric_limits<int>::max();

        Position pos_copy = position;
        auto search_history = position_history_;
        search_history[get_position_hash(pos_copy)]++;

        bool completed_depth = true;
        for (const Move& move : legal_moves) {
            if (should_stop_search()) {
                completed_depth = false;
                break;
            }

            auto undo_info = pos_copy.apply_move(move.from, move.to, move.promo);
            if (!undo_info) continue;

            auto child_history = search_history;
            child_history[get_position_hash(pos_copy)]++;

            int opponent_score = alphabeta(pos_copy, d - 1, alpha, beta, child_history);
            int score = -opponent_score;

            pos_copy.undo_move(undo_info.value());

            if (timed_out_) {
                completed_depth = false;
                break;
            }

            if (score > best_score) {
                best_score = score;
                best_moves.clear();
                best_moves.push_back(move);
            } else if (score == best_score && !best_moves.empty()) {
                best_moves.push_back(move);
            }

            alpha = std::max(alpha, score);
            if (alpha >= beta) {
                break;
            }
        }

        if (!completed_depth || best_moves.empty()) {
            break;
        }

        // Choose one of the equally-best moves at random for this completed depth.
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, best_moves.size() - 1);
        best_completed = MoveEvaluation{best_moves[dis(gen)], best_score};
        reached_depth = d;
    }

    std::cerr << "[" << name() << "] Reached depth " << reached_depth
              << ", returning move " << move_to_uci(best_completed.move) 
              << " with score " << best_completed.score << std::endl;

    return best_completed;
}

int MaterialEngine::evaluate(Position& position) {
    // Piece value constants (centipawns)
    constexpr int PAWN_VALUE = 100;
    constexpr int KNIGHT_VALUE = 320;
    constexpr int BISHOP_VALUE = 330;
    constexpr int ROOK_VALUE = 500;
    constexpr int QUEEN_VALUE = 900;
    
    Color side_to_move = position.side_to_move();
    
    // Check for terminal positions
    if (is_checkmate(position, side_to_move)) {
        return std::numeric_limits<int>::min() + 1;
    }
    
    if (is_stalemate(position, side_to_move)) {
        return 0;
    }

    if (position.halfmove_clock() >= 100) {
        return 0;
    }
    
    int score = 0;

    const int piece_values[] = {
        PAWN_VALUE,   KNIGHT_VALUE, BISHOP_VALUE, ROOK_VALUE, QUEEN_VALUE, 20000,
        PAWN_VALUE,   KNIGHT_VALUE, BISHOP_VALUE, ROOK_VALUE, QUEEN_VALUE, 20000
    };

    // Sum material only for all non-king pieces.
    for (int piece = 0; piece < 12; piece++) {
        U64 bb = position.bitboard(static_cast<Piece>(piece));
        if (bb == 0) continue;

        if (piece != WK && piece != BK) {
            int piece_count = __builtin_popcountll(bb);
            int piece_value = piece_values[piece];

            if (piece < 6) {
                score += piece_count * piece_value;
            } else {
                score -= piece_count * piece_value;
            }
        }
    }
    
    if (side_to_move == BLACK) {
        score = -score;
    }
    
    return score;
}

int MaterialEngine::alphabeta(Position& position, int depth, int alpha, int beta,
                              std::unordered_map<uint64_t, int> position_history) {
    if (should_stop_search()) {
        return this->evaluate(position);
    }

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
        if (should_stop_search()) {
            break;
        }

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

        if (timed_out_) {
            break;
        }
        
        max_eval = std::max(max_eval, eval);
        alpha = std::max(alpha, eval);
        
        if (beta <= alpha) {
            break; // Beta cutoff
        }
    }
    
    return max_eval;
}

}
