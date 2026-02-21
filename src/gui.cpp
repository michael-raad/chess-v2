#include "gui.hpp"
#include <iostream>

namespace chess {

GUI::GUI(PlayerType white_player, PlayerType black_player) 
    : game_(std::make_unique<Game>(white_player, black_player)),
      selected_square_(std::nullopt),
      state_(GUIState::MENU),
      fen_input_("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"),
      selected_player_white_(white_player == PlayerType::HUMAN ? 0 : 1),
      selected_player_black_(black_player == PlayerType::HUMAN ? 0 : 1)
{
    if (!font_.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
        std::cerr << "ERROR: Failed to load font\n";
    }
    if (!load_piece_textures()) {
        std::cerr << "ERROR: Failed to load piece textures\n";
    }
}

void GUI::run() {
    sf::RenderWindow window(sf::VideoMode(1000, 900), "Chess");
    window.setFramerateLimit(60);

    while (window.isOpen()) {
        handle_events(window);
        
        // Handle AI moves
        if (state_ == GUIState::PLAYING && game_->get_status() == GameStatus::PLAYING) {
            if (game_->get_current_player_type() == PlayerType::AI) {
                game_->make_ai_move();
                // Check if game ended after AI move
                if (game_->get_status() != GameStatus::PLAYING) {
                    state_ = GUIState::GAME_OVER;
                }
            }
        }
        
        window.clear(sf::Color::White);
        
        if (state_ == GUIState::MENU) {
            draw_menu(window);
        } else if (state_ == GUIState::PLAYING) {
            draw_board(window);
            draw_pieces(window);
            draw_game_status(window);
        } else if (state_ == GUIState::GAME_OVER) {
            draw_board(window);
            draw_pieces(window);
            draw_game_status(window);
            draw_text(window, "Press SPACE to return to menu", 350, 820, 20, sf::Color::Black);
        }
        
        window.display();
    }
}

void GUI::draw_menu(sf::RenderWindow& window) {
    draw_text(window, "CHESS", 400, 50, 60, sf::Color::Black);
    
    // FEN input
    draw_text(window, "FEN:", 50, 150, 20, sf::Color::Black);
    draw_text(window, fen_input_, 50, 180, 16, sf::Color::Blue);
    draw_text(window, "(Type FEN or press ENTER for default starting position)", 50, 210, 14, sf::Color(128, 128, 128));
    
    // Player selection
    draw_text(window, "White Player:", 50, 280, 20, sf::Color::Black);
    std::string white_type = (selected_player_white_ == 0) ? "[HUMAN]" : "[ AI ]";
    draw_text(window, white_type, 300, 280, 20, selected_player_white_ == 0 ? sf::Color::Green : sf::Color::Red);
    draw_text(window, "(Z to toggle)", 500, 280, 14, sf::Color(128, 128, 128));
    
    draw_text(window, "Black Player:", 50, 330, 20, sf::Color::Black);
    std::string black_type = (selected_player_black_ == 0) ? "[HUMAN]" : "[ AI ]";
    draw_text(window, black_type, 300, 330, 20, selected_player_black_ == 0 ? sf::Color::Green : sf::Color::Red);
    draw_text(window, "(X to toggle)", 500, 330, 14, sf::Color(128, 128, 128));
    
    // Instructions
    draw_text(window, "Press ENTER to start game", 50, 420, 20, sf::Color::Black);
    draw_text(window, "Type FEN string to load custom position", 50, 460, 14, sf::Color(128, 128, 128));
}

void GUI::draw_board(sf::RenderWindow& window) {
    const int square_size = 100;
    for (int rank = 0; rank < 8; ++rank) {
        for (int file = 0; file < 8; ++file) {
            sf::RectangleShape square(sf::Vector2f(square_size, square_size));
            square.setPosition(file * square_size, (7 - rank) * square_size);
            square.setFillColor((rank + file) % 2 == 0 ? sf::Color(240, 217, 181) : sf::Color(181, 136, 99));
            int sq = rank * 8 + file;
            if (selected_square_ && *selected_square_ == sq) {
                square.setFillColor(sf::Color::Yellow);
            }
            window.draw(square);
        }
    }
}

void GUI::draw_pieces(sf::RenderWindow& window) {
    const int square_size = 100;
    const auto& position = game_->get_position();
    for (int sq = 0; sq < 64; ++sq) {
        int piece_int = position.piece_on_square(sq);
        if (piece_int != NO_PIECE) {
            // Create sprite from texture
            sf::Sprite sprite(piece_textures_[piece_int]);
            
            // Scale sprite to fit in square (assuming textures are around 100x100)
            sprite.setScale(0.95f, 0.95f);
            
            int file = sq % 8;
            int rank = sq / 8;
            // Center piece in square
            float x = file * square_size + (square_size - sprite.getGlobalBounds().width) / 2;
            float y = (7 - rank) * square_size + (square_size - sprite.getGlobalBounds().height) / 2;
            sprite.setPosition(x, y);
            
            window.draw(sprite);
        }
    }
}

void GUI::draw_game_status(sf::RenderWindow& window) {
    int y = 820;
    GameStatus status = game_->get_status();
    
    std::string status_text;
    sf::Color color = sf::Color::Black;
    
    if (status == GameStatus::PLAYING) {
        Color turn = game_->get_position().side_to_move();
        status_text = (turn == WHITE) ? "White to move" : "Black to move";
    } else if (status == GameStatus::WHITE_CHECKMATE) {
        status_text = "White is checkmated - Black wins!";
        color = sf::Color::Red;
    } else if (status == GameStatus::BLACK_CHECKMATE) {
        status_text = "Black is checkmated - White wins!";
        color = sf::Color::Red;
    } else if (status == GameStatus::STALEMATE) {
        status_text = "Stalemate - Draw!";
        color = sf::Color::Blue;
    } else if (status == GameStatus::FIFTY_MOVE_DRAW) {
        status_text = "50-move rule - Draw!";
        color = sf::Color::Blue;
    }
    
    draw_text(window, status_text, 100, y, 20, color);
}

void GUI::draw_text(sf::RenderWindow& window, const std::string& text, int x, int y, int size, sf::Color color) {
    sf::Text sf_text;
    sf_text.setFont(font_);
    sf_text.setString(text);
    sf_text.setCharacterSize(size);
    sf_text.setFillColor(color);
    sf_text.setPosition(x, y);
    window.draw(sf_text);
}

void GUI::handle_events(sf::RenderWindow& window) {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
        } else if (state_ == GUIState::MENU) {
            handle_menu_input(event);
        } else if (state_ == GUIState::PLAYING) {
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                if (game_->get_current_player_type() != PlayerType::HUMAN) return;
                int x = event.mouseButton.x / 100;
                int y = 7 - (event.mouseButton.y / 100);
                if (x < 0 || x >= 8 || y < 0 || y >= 8) return;
                int sq = y * 8 + x;
                
                if (!selected_square_) {
                    const auto& position = game_->get_position();
                    if (position.piece_on_square(sq) != NO_PIECE && piece_color(static_cast<Piece>(position.piece_on_square(sq))) == position.side_to_move()) {
                        selected_square_ = sq;
                    }
                } else {
                    if (game_->try_move(*selected_square_, sq)) {
                        // Move succeeded
                    }
                    selected_square_ = std::nullopt;
                    
                    // Check if game ended
                    if (game_->get_status() != GameStatus::PLAYING) {
                        state_ = GUIState::GAME_OVER;
                    }
                }
            }
        } else if (state_ == GUIState::GAME_OVER) {
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space) {
                state_ = GUIState::MENU;
                fen_input_ = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
                selected_player_white_ = 0;
                selected_player_black_ = 1;
            }
        }
    }
}

void GUI::handle_menu_input(const sf::Event& event) {
    if (event.type == sf::Event::TextEntered) {
        if (event.text.unicode == '\b') { // backspace
            if (!fen_input_.empty()) {
                fen_input_.pop_back();
            }
        } else if (event.text.unicode == '\r') { // enter
            // Start game
            PlayerType white_type = (selected_player_white_ == 0) ? PlayerType::HUMAN : PlayerType::AI;
            PlayerType black_type = (selected_player_black_ == 0) ? PlayerType::HUMAN : PlayerType::AI;
            game_ = std::make_unique<Game>(white_type, black_type);
            if (!fen_input_.empty()) {
                game_->set_fen(fen_input_);
            }
            selected_square_ = std::nullopt;
            state_ = GUIState::PLAYING;
        } else if (event.text.unicode < 128 && event.text.unicode != 'z' && event.text.unicode != 'Z' && event.text.unicode != 'x' && event.text.unicode != 'X') { // printable ASCII (excluding Z/X toggles)
            fen_input_ += static_cast<char>(event.text.unicode);
        }
    } else if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Z) {
            selected_player_white_ = 1 - selected_player_white_;
        } else if (event.key.code == sf::Keyboard::X) {
            selected_player_black_ = 1 - selected_player_black_;
        }
    }
}

bool GUI::load_piece_textures() {
    // Piece indices: WP=0, WN=1, WB=2, WR=3, WQ=4, WK=5, BP=6, BN=7, BB=8, BR=9, BQ=10, BK=11
    const std::array<std::string, 12> filenames = {
        "white-pawn.png",
        "white-knight.png",
        "white-bishop.png",
        "white-rook.png",
        "white-queen.png",
        "white-king.png",
        "black-pawn.png",
        "black-knight.png",
        "black-bishop.png",
        "black-rook.png",
        "black-queen.png",
        "black-king.png"
    };
    
    for (int i = 0; i < 12; ++i) {
        std::string path = "images/" + filenames[i];
        if (!piece_textures_[i].loadFromFile(path)) {
            std::cerr << "ERROR: Failed to load " << path << "\n";
            return false;
        }
    }
    return true;
}

}