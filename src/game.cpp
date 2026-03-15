#include "game.hpp"
#include "random_engine.hpp"
#include "choppedfish_engine.hpp"
#include <random>
#include <vector>

namespace chess {

Game::Game(PlayerType white_player, PlayerType black_player) : position_(), movegen_(position_), selected_square_(std::nullopt), attack_tables_init_(), status_(GameStatus::PLAYING), white_engine_(nullptr), black_engine_(nullptr) {
    players_[0] = white_player;
    players_[1] = black_player;
    init_default_engines();
    update_status();
    update_legal_moves();
    // Don't make AI move here - let GUI render board first
}

Game::Game(std::unique_ptr<Engine> white_engine, std::unique_ptr<Engine> black_engine,
           PlayerType white_player, PlayerType black_player) 
    : position_(), movegen_(position_), selected_square_(std::nullopt), attack_tables_init_(), 
      status_(GameStatus::PLAYING), white_engine_(std::move(white_engine)), black_engine_(std::move(black_engine)) {
    players_[0] = white_player;
    players_[1] = black_player;
    update_status();
    update_legal_moves();
    // Don't make AI move here - let GUI render board first
}

void Game::init_default_engines() {
    white_engine_ = std::make_unique<ChoppedfishEngine>(4);
    black_engine_ = std::make_unique<ChoppedfishEngine>(4);
}

bool Game::set_fen(const std::string& fen) {
    if (!position_.set_from_fen(fen)) {
        return false;
    }
    selected_square_ = std::nullopt;
    last_move_from_ = std::nullopt;
    last_move_to_ = std::nullopt;
    update_status();
    update_legal_moves();
    // Don't make AI move here - let GUI render board first
    return true;
}

bool Game::try_move(int from, int to, int promo) {
    if (status_ != GameStatus::PLAYING) return false;
    if (get_current_player_type() != PlayerType::HUMAN) return false;

    // Check if move is in cached legal moves
    for (const auto& m : legal_moves_) {
        if (m.from == from && m.to == to && (promo == 0 || m.promo == promo)) {
            // Move is legal, apply it
            auto info = position_.apply_move(m.from, m.to, m.promo);
            if (!info) return false;
            
            last_move_from_ = from;
            last_move_to_ = to;
            update_status();
            update_legal_moves();
            // Don't call make_ai_move here - let GUI loop handle it for proper rendering
            return true;
        }
    }
    return false;
}

bool Game::is_promotion_move(int from, int to) const {
    // Check if this move is a pawn promotion
    for (const auto& m : legal_moves_) {
        if (m.from == from && m.to == to && m.promo != 0) {
            return true;
        }
    }
    return false;
}

void Game::make_ai_move() {
    if (status_ != GameStatus::PLAYING) return;
    if (get_current_player_type() != PlayerType::AI) return;

    // Get the appropriate engine for current player
    Engine* current_engine = (position_.side_to_move() == WHITE) ? white_engine_.get() : black_engine_.get();
    
    if (!current_engine) return;
    
    Move ai_move = current_engine->get_best_move(position_);
    
    // Verify the move is legal (check from, to, AND promo for promotion moves)
    bool move_found = false;
    for (const auto& m : legal_moves_) {
        if (m.from == ai_move.from && m.to == ai_move.to && m.promo == ai_move.promo) {
            move_found = true;
            break;
        }
    }
    
    if (!move_found) return;
    
    auto info = position_.apply_move(ai_move.from, ai_move.to, ai_move.promo);
    if (info) {
        last_move_from_ = ai_move.from;
        last_move_to_ = ai_move.to;
        update_status();
        update_legal_moves();
        // Note: GUI main loop will call make_ai_move() again if next player is AI
        // This allows rendering between moves instead of computing the entire game tree recursively
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

void Game::update_legal_moves() {
    legal_moves_ = chess::get_legal_moves(position_);
}

}