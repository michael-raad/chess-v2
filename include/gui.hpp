#pragma once

#include <SFML/Graphics.hpp>
#include <optional>
#include <string>
#include <memory>
#include <array>
#include "game.hpp"

namespace chess {

enum class GUIState {
    MENU,
    PLAYING,
    GAME_OVER
};

class GUI {
public:
    GUI(PlayerType white_player = PlayerType::HUMAN, PlayerType black_player = PlayerType::AI);
    void run();

private:
    void draw_menu(sf::RenderWindow& window);
    void draw_board(sf::RenderWindow& window);
    void draw_pieces(sf::RenderWindow& window);
    void draw_game_status(sf::RenderWindow& window);
    void draw_text(sf::RenderWindow& window, const std::string& text, int x, int y, int size, sf::Color color);
    void handle_events(sf::RenderWindow& window);
    void handle_menu_input(const sf::Event& event);
    bool load_piece_textures();

    sf::Font font_;
    std::array<sf::Texture, 12> piece_textures_;  // WP, WN, WB, WR, WQ, WK, BP, BN, BB, BR, BQ, BK
    std::unique_ptr<Game> game_;
    std::optional<int> selected_square_;
    GUIState state_;
    std::string fen_input_;
    int selected_player_white_;
    int selected_player_black_;
};

}