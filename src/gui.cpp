#include "gui.hpp"
#include "config.hpp"
#include "move_notation.hpp"
#include <iostream>
#include <algorithm>

namespace chess {

GUI::GUI(PlayerType white_player, PlayerType black_player) 
    : game_(std::make_unique<Game>(white_player, black_player)),
      white_engine_(std::make_unique<UIClient>(WHITE_ENGINE_BIN_PATH)),
      black_engine_(std::make_unique<UIClient>(BLACK_ENGINE_BIN_PATH)),
      selected_square_(std::nullopt),
      state_(GUIState::MENU),
      fen_input_("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"),
      // fen_input_("r1bqk1nr/pppp4/5p2/1BQ3pp/4P3/2N5/PPP2PPP/R1B2RK1 w kq h6 0 10"),
      selected_player_white_(white_player == PlayerType::HUMAN ? 0 : 1),
      selected_player_black_(black_player == PlayerType::HUMAN ? 0 : 1)
{
    if (!font_.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
        std::cerr << "[GUI] ERROR: Failed to load font\n";
    }
    if (!load_piece_textures()) {
        std::cerr << "[GUI] ERROR: Failed to load piece textures\n";
    }
    
    // Initialize UCI engine connections
    bool white_ok = white_engine_->initialize();
    bool black_ok = black_engine_->initialize();
    
    if (!white_ok || !black_ok) {
        std::cerr << "[GUI] ERROR: Failed to initialize one or both UCI engines\n";
        if (!white_ok) std::cerr << "[GUI]  White engine failed\n";
        if (!black_ok) std::cerr << "[GUI]  Black engine failed\n";
        // Continue anyway - will fall back to embedded engines if needed
    }
}

void GUI::run() {
    sf::RenderWindow window(sf::VideoMode(window_width_, window_height_), "Chess");
    window.setFramerateLimit(60);

    while (window.isOpen()) {
        handle_events(window);
        
        window.clear(sf::Color::White);
        
        if (state_ == GUIState::MENU) {
            draw_menu(window);
        } else if (state_ == GUIState::PLAYING) {
            draw_board(window);
            draw_pieces(window);
            draw_game_status(window);
        } else if (state_ == GUIState::PROMOTION) {
            draw_board(window);
            draw_pieces(window);
            draw_promotion_dialog(window);
        } else if (state_ == GUIState::GAME_OVER) {
            draw_board(window);
            draw_pieces(window);
            draw_game_status(window);
            draw_text(window, "Press SPACE to return to menu", 480, 805, 20, sf::Color::Black);
        }
        
        window.display();
        
        // Handle AI moves AFTER rendering so human move displays before AI computes
        if (state_ == GUIState::PLAYING && game_->get_status() == GameStatus::PLAYING) {
            if (game_->get_current_player_type() == PlayerType::AI) {
                // Choose the appropriate engine based on whose turn it is
                UIClient* current_engine = (game_->get_position().side_to_move() == Color::WHITE) 
                    ? white_engine_.get() 
                    : black_engine_.get();
                
                // Use UIClient to get best move from engine
                if (current_engine && current_engine->is_alive()) {
                    Color side = game_->get_position().side_to_move();
                    std::cerr << "[GUI] Requesting move from " << (side == Color::WHITE ? "white" : "black") << " engine..." << std::endl;
                    
                    // Send position to engine (FEN + empty move list)
                    current_engine->set_position(game_->get_position().get_fen());
                    
                    // Get best move from engine
                    auto best_move_eval = current_engine->get_best_move(0, 4000); // Up to depth d, stops at s seconds
                    
                    if (best_move_eval) {
                        std::cerr << "[GUI] Engine returned move " << format_move_for_log(best_move_eval->move) << std::endl;
                        
                        // Apply the move
                        bool moved = game_->apply_move(best_move_eval->move.from, best_move_eval->move.to, best_move_eval->move.promo);
                        if (!moved) {
                            std::cerr << "[GUI] ERROR: Failed to apply move from engine!" << std::endl;
                        }
                    } else {
                        std::cerr << "[GUI] ERROR: Engine returned no move" << std::endl;
                    }
                } else {
                    std::cerr << "[GUI] ERROR: Engine not alive" << std::endl;
                }
                
                // Check if game ended after AI move
                if (game_->get_status() != GameStatus::PLAYING) {
                    state_ = GUIState::GAME_OVER;
                }
            }
        }
        
        // Additional check: ensure we transition to GAME_OVER when status changes to any end state
        if (state_ == GUIState::PLAYING && game_->get_status() != GameStatus::PLAYING) {
            state_ = GUIState::GAME_OVER;
        }
    }
}

void GUI::draw_menu(sf::RenderWindow& window) {
    draw_text(window, "Chess", 5, 2, 20, sf::Color::Black, sf::Text::Style::Bold);
    draw_text(window, "Game Setup", window_width_ / 2 - 160, 50, 50, sf::Color::Black, sf::Text::Style::Bold);

    // draw_text(window, "CHESS", window_width_ / 2 - 100, 50, 60, sf::Color::Black);
    
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
    
    // Collect valid destination squares if a piece is selected
    std::vector<int> valid_destinations;
    if (selected_square_) {
        const auto& legal_moves = game_->get_legal_moves();
        for (const auto& move : legal_moves) {
            if (move.from == *selected_square_) {
                valid_destinations.push_back(move.to);
            }
        }
    }
    
    const auto& position = game_->get_position();
    auto last_move_from = game_->get_last_move_from();
    auto last_move_to = game_->get_last_move_to();
    
    for (int rank = 0; rank < 8; ++rank) {
        for (int file = 0; file < 8; ++file) {
            sf::RectangleShape square(sf::Vector2f(square_size, square_size));
            square.setPosition(file * square_size, (7 - rank) * square_size);
            square.setFillColor((rank + file) % 2 == 0 ? sf::Color(181, 136, 99) : sf::Color(240, 217, 181));
            int sq = rank * 8 + file;
            
            // Highlight last move squares in pale yellow
            if ((last_move_from && *last_move_from == sq) || (last_move_to && *last_move_to == sq)) {
                square.setFillColor((rank + file) % 2 == 0 ? sf::Color(210, 200, 140) : sf::Color(220, 210, 160));
            }
            
            // Highlight selected square in green
            if (selected_square_ && *selected_square_ == sq) {
                square.setFillColor(sf::Color(107, 181, 99));
            }
             
            window.draw(square);

            // Draw circle overlay for valid destination squares
            if (std::find(valid_destinations.begin(), valid_destinations.end(), sq) != valid_destinations.end()) {
                sf::CircleShape empty_highlight(square_size * 0.125f);
                sf::CircleShape capture_highlight(square_size * 0.4f);
                empty_highlight.setPosition(file * square_size + square_size / 2 - empty_highlight.getRadius(), (7 - rank) * square_size + square_size / 2 - empty_highlight.getRadius());
                capture_highlight.setPosition(file * square_size + square_size / 2 - capture_highlight.getRadius(), (7 - rank) * square_size + square_size / 2 - capture_highlight.getRadius());
                
                // large circle if capturing, small circle otherwise
                if (position.piece_on_square(sq) != NO_PIECE) {
                    capture_highlight.setOutlineThickness(10.f);
                    capture_highlight.setOutlineColor(sf::Color(107, 181, 99));
                    capture_highlight.setFillColor(sf::Color::Transparent);
                    window.draw(capture_highlight);
                } else {
                    empty_highlight.setFillColor(sf::Color(107, 181, 99));
                    window.draw(empty_highlight);
                }
            }
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
            sprite.setScale(0.80f, 0.80f);
            
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
    int y = 805;
    GameStatus status = game_->get_status();

    std::string status_text = "Game Status: ";
    sf::Color color = sf::Color::Black;
    
    if (status == GameStatus::PLAYING) {
        Color turn = game_->get_position().side_to_move();
        status_text += (turn == WHITE) ? "White to move" : "Black to move";
    } else if (status == GameStatus::WHITE_CHECKMATE) {
        status_text += "Checkmate - White wins!";
        color = sf::Color::Red;
    } else if (status == GameStatus::BLACK_CHECKMATE) {
        status_text += "Checkmate - Black wins!";
        color = sf::Color::Red;
    } else if (status == GameStatus::STALEMATE) {
        status_text += "Stalemate - Draw!";
        color = sf::Color::Blue;
    } else if (status == GameStatus::FIFTY_MOVE_DRAW) {
        status_text += "50-move rule - Draw!";
        color = sf::Color::Blue;
    } else if (status == GameStatus::THREEFOLD_REPETITION_DRAW) {
        status_text += "Threefold repetition - Draw!";
        color = sf::Color::Blue;
    } else {
        status_text += "Unknown status";
    }
    
    draw_text(window, status_text, 10, y, 20, color);
}

void GUI::draw_text(sf::RenderWindow& window, const std::string& text, int x, int y, int size, sf::Color color, sf::Text::Style style) {
    sf::Text sf_text;
    sf_text.setFont(font_);
    sf_text.setString(text);
    sf_text.setCharacterSize(size);
    sf_text.setFillColor(color);
    sf_text.setStyle(style);
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
                    // Check if this is a promotion move
                    if (game_->is_promotion_move(*selected_square_, sq)) {
                        // Enter promotion state and wait for piece selection
                        promotion_from_ = *selected_square_;
                        promotion_to_ = sq;
                        selected_square_ = std::nullopt;
                        state_ = GUIState::PROMOTION;
                    } else {
                        // Regular move
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
            } else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::H) {
                // Evaluate position on demand and display score in console
                // (This is a hidden feature for testing the evaluation function)
                if (game_->get_current_player_type() == PlayerType::HUMAN) {
                    int score = game_->evaluate_position();
                    std::cout << "Evaluation score from perspective to move: " << score << std::endl;
                } else {
                    std::cout << "H key only works during human's turn" << std::endl;
                }
            } else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::N) {
                // Get best move and score suggestion from evaluation engine
                // (This is a hint feature for debugging and board analysis)
                if (game_->get_current_player_type() == PlayerType::HUMAN) {
                    MoveEvaluation eval = game_->get_best_move_suggestion();
                    std::cout << "Best move suggestion: " 
                              << char('a' + (eval.move.from % 8)) << (1 + eval.move.from / 8) << "-"
                              << char('a' + (eval.move.to % 8)) << (1 + eval.move.to / 8);
                    if (eval.move.promo) {
                        std::cout << " (promo to " << eval.move.promo << ")";
                    }
                    std::cout << ", Score: " << eval.score << std::endl;
                } else {
                    std::cout << "N key only works during human's turn" << std::endl;
                }
            }
        } else if (state_ == GUIState::PROMOTION) {
            if (event.type == sf::Event::KeyPressed) {
                int promo = 0;
                if (event.key.code == sf::Keyboard::Q) {
                    promo = QUEEN;
                } else if (event.key.code == sf::Keyboard::R) {
                    promo = ROOK;
                } else if (event.key.code == sf::Keyboard::B) {
                    promo = BISHOP;
                } else if (event.key.code == sf::Keyboard::N) {
                    promo = KNIGHT;
                }
                
                if (promo != 0 && promotion_from_ && promotion_to_) {
                    // Complete the promotion move
                    if (game_->try_move(*promotion_from_, *promotion_to_, promo)) {
                        // Move succeeded
                    }
                    promotion_from_ = std::nullopt;
                    promotion_to_ = std::nullopt;
                    state_ = GUIState::PLAYING;
                    
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
            
            // Reinitialize UCI engines for new game
            white_engine_ = std::make_unique<UIClient>(WHITE_ENGINE_BIN_PATH);
            black_engine_ = std::make_unique<UIClient>(BLACK_ENGINE_BIN_PATH);
            
            bool white_ok = white_engine_->initialize();
            bool black_ok = black_engine_->initialize();
            
            if (!white_ok || !black_ok) {
                std::cerr << "[GUI] ERROR: Failed to initialize one or both UCI engines\n";
            }
            
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

void GUI::draw_promotion_dialog(sf::RenderWindow& window) {
    // Draw semi-transparent overlay
    sf::RectangleShape overlay(sf::Vector2f(window_width_, window_height_));
    overlay.setFillColor(sf::Color(0, 0, 0, 200));
    window.draw(overlay);
    
    // Draw dialog box
    sf::RectangleShape dialog(sf::Vector2f(400, 300));
    dialog.setPosition(200, 300);
    dialog.setFillColor(sf::Color::White);
    window.draw(dialog);
    
    // Draw dialog border
    sf::RectangleShape border(sf::Vector2f(400, 300));
    border.setPosition(200, 300);
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineColor(sf::Color::Black);
    border.setOutlineThickness(3);
    window.draw(border);
    
    // Draw title
    draw_text(window, "Promote to:", 250, 320, 24, sf::Color::Black);
    
    // Draw promotion options
    draw_text(window, "Q - Queen", 250, 370, 20, sf::Color::Black);
    
    // Rook option
    draw_text(window, "R - Rook", 250, 410, 20, sf::Color::Black);
    
    // Bishop option
    draw_text(window, "B - Bishop", 250, 450, 20, sf::Color::Black);
    
    // Knight option
    draw_text(window, "N - Knight", 250, 490, 20, sf::Color::Black);
    
    draw_text(window, "Press a key to select", 240, 540, 16, sf::Color(100, 100, 100));
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
   
    // Load piece textures from images directory (configured at build time)
    for (int i = 0; i < 12; ++i) {
        std::string path = IMAGES_DIR + filenames[i];
        if (!piece_textures_[i].loadFromFile(path)) {
            std::cerr << "[GUI] ERROR: Failed to load " << path << "\n";
            return false;
        }
    }
    return true;
}

}