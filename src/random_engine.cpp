#include "random_engine.hpp"
#include "search.hpp"
#include <random>

namespace chess {

Move RandomEngine::get_best_move(const Position& position) {
    auto legal_moves = chess::get_legal_moves(const_cast<Position&>(position));
    if (legal_moves.empty()) return Move(-1, -1, 0); // Invalid move if no legal moves
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, legal_moves.size() - 1);
    return legal_moves[dis(gen)];
}

}
