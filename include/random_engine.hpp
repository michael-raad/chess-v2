#pragma once

#include "engine.hpp"

namespace chess {

// Simple random move engine - selects legal moves uniformly at random
class RandomEngine : public Engine {
public:
    MoveEvaluation get_best_move(const Position& position, int depth = 1, long long movetime_ms = 0) override;
    int evaluate(Position& position) override;
    
    std::string name() const override { return "RandomEngine"; }
};

}
