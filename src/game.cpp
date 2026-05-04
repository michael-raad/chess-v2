#include "game.hpp"
#include "position_engine.hpp"
#include <random>
#include <vector>

namespace chess {

Game::Game(PlayerType white_player, PlayerType black_player) : position_(), movegen_(position_), selected_square_(std::nullopt), attack_tables_init_(), status_(GameStatus::PLAYING), evaluation_engine_(std::make_unique<PositionEngine>()) {
    players_[0] = white_player;
    players_[1] = black_player;
    update_repetition_history();  // Initialize with starting position
    update_status();
    update_legal_moves();
    // Don't make AI move here - let GUI render board first
}

bool Game::set_fen(const std::string& fen) {
    if (!position_.set_from_fen(fen)) {
        return false;
    }
    selected_square_ = std::nullopt;
    last_move_from_ = std::nullopt;
    last_move_to_ = std::nullopt;
    position_history_.clear();  // Reset repetition history for new position
    move_history_.clear();  // Reset move history for new position
    update_repetition_history();  // Initialize with this position
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
            
            // Convert move to algebraic notation for move history
            char from_file = 'a' + (from % 8);
            char from_rank = '1' + (from / 8);
            char to_file = 'a' + (to % 8);
            char to_rank = '1' + (to / 8);
            
            std::string move_notation;
            move_notation += from_file;
            move_notation += from_rank;
            move_notation += to_file;
            move_notation += to_rank;
            
            // Add promotion piece if applicable
            if (m.promo != 0) {
                switch (m.promo) {
                    case 1: move_notation += 'n'; break;  // Knight
                    case 2: move_notation += 'b'; break;  // Bishop
                    case 3: move_notation += 'r'; break;  // Rook
                    case 4: move_notation += 'q'; break;  // Queen
                }
            }
            
            move_history_.push_back(move_notation);
            
            // Handle position history for repetition detection
            // Reset if capture, pawn move, or castling rights change
            if (info->captured_piece != NO_PIECE || info->was_ep_capture || 
                info->old_castling != position_.castling_rights() ||
                (static_cast<Piece>(position_.piece_on_square(to)) == WP || 
                 static_cast<Piece>(position_.piece_on_square(to)) == BP)) {
                position_history_.clear();
            }
            update_repetition_history();
            
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

bool Game::apply_move(int from, int to, int promo) {
    if (status_ != GameStatus::PLAYING) return false;

    // Check if move is in cached legal moves
    for (const auto& m : legal_moves_) {
        if (m.from == from && m.to == to && (promo == 0 || m.promo == promo)) {
            // Move is legal, apply it
            auto info = position_.apply_move(m.from, m.to, m.promo);
            if (!info) return false;
            
            last_move_from_ = from;
            last_move_to_ = to;
            
            // Convert move to algebraic notation for move history
            char from_file = 'a' + (from % 8);
            char from_rank = '1' + (from / 8);
            char to_file = 'a' + (to % 8);
            char to_rank = '1' + (to / 8);
            
            std::string move_notation;
            move_notation += from_file;
            move_notation += from_rank;
            move_notation += to_file;
            move_notation += to_rank;
            
            // Add promotion piece if applicable
            if (m.promo != 0) {
                switch (m.promo) {
                    case 1: move_notation += 'n'; break;  // Knight
                    case 2: move_notation += 'b'; break;  // Bishop
                    case 3: move_notation += 'r'; break;  // Rook
                    case 4: move_notation += 'q'; break;  // Queen
                }
            }
            
            move_history_.push_back(move_notation);
            
            // Handle position history for repetition detection
            // Reset if capture, pawn move, or castling rights change
            if (info->captured_piece != NO_PIECE || info->was_ep_capture || 
                info->old_castling != position_.castling_rights() ||
                (static_cast<Piece>(position_.piece_on_square(to)) == WP || 
                 static_cast<Piece>(position_.piece_on_square(to)) == BP)) {
                position_history_.clear();
            }
            update_repetition_history();
            
            update_status();
            update_legal_moves();
            return true;
        }
    }
    return false;
}



void Game::update_status() {
    if (is_checkmate(position_, WHITE)) {
        status_ = GameStatus::BLACK_CHECKMATE; // Black won
    } else if (is_checkmate(position_, BLACK)) {
        status_ = GameStatus::WHITE_CHECKMATE; // White won
    } else if (is_threefold_repetition(position_history_)) {
        status_ = GameStatus::THREEFOLD_REPETITION_DRAW;
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

void Game::update_repetition_history() {
    // Get hash of current position and increment count
    uint64_t position_hash = get_position_hash(position_);
    position_history_[position_hash]++;
}

}