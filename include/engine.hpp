#pragma once

#include <string>
#include "position.hpp"
#include "movegen.hpp"

namespace chess {

// Abstract base class for chess engines
class Engine {
public:
    virtual ~Engine() = default;
    
    // Get the best move according to this engine's strategy
    virtual Move get_best_move(const Position& position) = 0;
    
    // Optional: Get engine name for debugging/logging
    virtual std::string name() const { return "UnnamedEngine"; }
};

}
