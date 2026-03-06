#pragma once

#include "engine.hpp"

namespace chess {

// Simple random move engine - selects legal moves uniformly at random
class RandomEngine : public Engine {
public:
    Move get_best_move(const Position& position) override;
    
    std::string name() const override { return "RandomEngine"; }
};

}
