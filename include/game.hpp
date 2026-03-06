#pragma once

#include <optional>
#include <string>
#include <vector>
#include <memory>
#include "position.hpp"
#include "movegen.hpp"
#include "search.hpp"
#include "attacks.hpp"
#include "engine.hpp"

namespace chess {

enum class GameStatus {
    PLAYING,
    WHITE_CHECKMATE,
    BLACK_CHECKMATE,
    STALEMATE,
    FIFTY_MOVE_DRAW
};

class Game {
public:
    Game(PlayerType white_player, PlayerType black_player);
    
    // Constructor allowing custom engines (for testing)
    Game(std::unique_ptr<Engine> white_engine, std::unique_ptr<Engine> black_engine,
         PlayerType white_player, PlayerType black_player);
    
    bool set_fen(const std::string& fen);

    const Position& get_position() const { return position_; }
    PlayerType get_current_player_type() const { return players_[position_.side_to_move()]; }
    std::optional<int> get_selected_square() const { return selected_square_; }
    void set_selected_square(std::optional<int> sq) { selected_square_ = sq; }
    GameStatus get_status() const { return status_; }
    const std::vector<Move>& get_legal_moves() const { return legal_moves_; }
    std::optional<int> get_last_move_from() const { return last_move_from_; }
    std::optional<int> get_last_move_to() const { return last_move_to_; }

    bool try_move(int from, int to, int promo = 0);
    void make_ai_move();
    bool is_promotion_move(int from, int to) const;

private:
    void update_status();
    void update_legal_moves();
    void init_default_engines();

    Position position_;
    MoveGenerator movegen_;
    PlayerType players_[2];
    std::optional<int> selected_square_;
    AttackTablesInitializer attack_tables_init_;
    GameStatus status_;
    std::vector<Move> legal_moves_;
    std::optional<int> last_move_from_;
    std::optional<int> last_move_to_;
    std::unique_ptr<Engine> white_engine_;
    std::unique_ptr<Engine> black_engine_;
};

}