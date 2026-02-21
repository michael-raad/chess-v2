#include "game.hpp"
#include <random>
#include <vector>

namespace chess {

Game::Game(PlayerType white_player, PlayerType black_player) : position_(), movegen_(position_), selected_square_(std::nullopt), attack_tables_init_(), status_(GameStatus::PLAYING) {
    players_[0] = white_player;
    players_[1] = black_player;
    update_status();
    if (players_[position_.side_to_move()] == PlayerType::AI) {
        make_ai_move();
    }
}

bool Game::set_fen(const std::string& fen) {
    if (!position_.set_from_fen(fen)) {
        return false;
    }
    selected_square_ = std::nullopt;
    update_status();
    if (players_[position_.side_to_move()] == PlayerType::AI) {
        make_ai_move();
    }
    return true;
}

bool Game::try_move(int from, int to) {
    if (status_ != GameStatus::PLAYING) return false;
    if (get_current_player_type() != PlayerType::HUMAN) return false;

    auto legal_moves = get_legal_moves(position_);
    
    for (const auto& m : legal_moves) {
        if (m.from == from && m.to == to) {
            // Move is legal, apply it
            auto info = position_.apply_move(m.from, m.to, m.promo);
            if (!info) return false;
            
            update_status();
            if (status_ == GameStatus::PLAYING && players_[position_.side_to_move()] == PlayerType::AI) {
                make_ai_move();
            }
            return true;
        }
    }
    return false;
}

void Game::make_ai_move() {
    if (status_ != GameStatus::PLAYING) return;
    if (get_current_player_type() != PlayerType::AI) return;

    auto legal_moves = get_legal_moves(position_);
    if (legal_moves.empty()) return;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, legal_moves.size() - 1);
    Move ai_move = legal_moves[dis(gen)];
    
    auto info = position_.apply_move(ai_move.from, ai_move.to, ai_move.promo);
    if (info) {
        update_status();
        if (status_ == GameStatus::PLAYING && players_[position_.side_to_move()] == PlayerType::AI) {
            make_ai_move(); // Recurse for AI vs AI
        }
    }
}

void Game::update_status() {
    if (is_checkmate(position_, WHITE)) {
        status_ = GameStatus::BLACK_CHECKMATE; // Black won
    } else if (is_checkmate(position_, BLACK)) {
        status_ = GameStatus::WHITE_CHECKMATE; // White won
    } else if (is_draw_by_50_move_rule(position_)) {
        status_ = GameStatus::FIFTY_MOVE_DRAW;
    } else if (is_stalemate(position_, WHITE) || is_stalemate(position_, BLACK)) {
        status_ = GameStatus::STALEMATE;
    } else {
        status_ = GameStatus::PLAYING;
    }
}

}