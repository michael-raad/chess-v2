#pragma once

#include <optional>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "position.hpp"
#include "movegen.hpp"
#include "search.hpp"
#include "attacks.hpp"
#include "engine.hpp"
#include "zobrist.hpp"

namespace chess {

enum class GameStatus {
    PLAYING,
    WHITE_CHECKMATE,
    BLACK_CHECKMATE,
    STALEMATE,
    FIFTY_MOVE_DRAW,
    THREEFOLD_REPETITION_DRAW
};

// Engine matchup configurations for benchmarking
class Game {
public:
    Game(PlayerType white_player, PlayerType black_player);
    

    
    bool set_fen(const std::string& fen);

    const Position& get_position() const { return position_; }
    PlayerType get_current_player_type() const { return players_[position_.side_to_move()]; }
    std::optional<int> get_selected_square() const { return selected_square_; }
    void set_selected_square(std::optional<int> sq) { selected_square_ = sq; }
    GameStatus get_status() const { return status_; }
    const std::vector<Move>& get_legal_moves() const { return legal_moves_; }
    std::optional<int> get_last_move_from() const { return last_move_from_; }
    std::optional<int> get_last_move_to() const { return last_move_to_; }
    
    // Get move history in algebraic notation (e.g., "e2e4", "e7e8q")
    const std::vector<std::string>& get_move_history() const { return move_history_; }

    bool try_move(int from, int to, int promo = 0);
    bool is_promotion_move(int from, int to) const;
    
    // Apply a move directly (for AI moves from external engine)
    bool apply_move(int from, int to, int promo = 0);
    
    // Get the evaluation engine (used for board analysis)
    const Engine* get_evaluation_engine() const {
        return evaluation_engine_.get();
    }
    
    // Get best move suggestion for current position (for hints)
    MoveEvaluation get_best_move_suggestion() const {
        if (!evaluation_engine_) {
            return MoveEvaluation{Move{0, 0, 0}, 0};
        }
        // Provide position history for threefold repetition awareness
        evaluation_engine_->set_position_history(position_history_);
        return evaluation_engine_->get_best_move(position_);
    }
    
    // Evaluate current position (for analysis)
    int evaluate_position() const {
        if (!evaluation_engine_) return 0;
        Position pos_copy = position_;
        return evaluation_engine_->evaluate(pos_copy);
    }

private:
    void update_status();
    void update_legal_moves();
    void update_repetition_history();  // Track position for threefold repetition
    bool should_reset_repetition_history(const Move& move) const;  // Check if move breaks repetition chain

    Position position_;
    MoveGenerator movegen_;
    PlayerType players_[2];
    std::optional<int> selected_square_;
    AttackTablesInitializer attack_tables_init_;
    GameStatus status_;
    std::vector<Move> legal_moves_;
    std::optional<int> last_move_from_;
    std::optional<int> last_move_to_;
    std::unique_ptr<Engine> evaluation_engine_;  // Engine for board analysis
    std::unordered_map<uint64_t, int> position_history_;  // Hash -> count for threefold repetition
    std::vector<std::string> move_history_;  // Move history in algebraic notation (e.g., "e2e4")
};

}