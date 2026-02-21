#pragma once

#include <optional>
#include <string>
#include "position.hpp"
#include "movegen.hpp"
#include "search.hpp"
#include "attacks.hpp"

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
    
    bool set_fen(const std::string& fen);

    const Position& get_position() const { return position_; }
    PlayerType get_current_player_type() const { return players_[position_.side_to_move()]; }
    std::optional<int> get_selected_square() const { return selected_square_; }
    void set_selected_square(std::optional<int> sq) { selected_square_ = sq; }
    GameStatus get_status() const { return status_; }

    bool try_move(int from, int to);
    void make_ai_move();

private:
    void update_status();

    Position position_;
    MoveGenerator movegen_;
    PlayerType players_[2];
    std::optional<int> selected_square_;
    AttackTablesInitializer attack_tables_init_;
    GameStatus status_;
};

}