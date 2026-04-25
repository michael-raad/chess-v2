#include "random_engine.hpp"
#include "search.hpp"
#include <random>

namespace chess {

MoveEvaluation RandomEngine::get_best_move(const Position& position, int depth, long long movetime_ms) {
    // RandomEngine ignores all parameters and just returns a random move
    (void)depth;
    (void)movetime_ms;
    Position pos_copy = position;  // Copy for move legality checking
    auto legal_moves = chess::get_legal_moves(pos_copy);
    if (legal_moves.empty()) return MoveEvaluation{Move(-1, -1, 0), 0}; // Invalid move if no legal moves
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, legal_moves.size() - 1);
    return MoveEvaluation{legal_moves[dis(gen)], 0};
}

int RandomEngine::evaluate(Position& position) {
    (void)position;  // Unused
    // Random engine doesn't use evaluation - it just picks random moves
    return 0;
}

}
