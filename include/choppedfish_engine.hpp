#pragma once

#include "engine.hpp"
#include <array>

namespace chess {

// Minimax search engine with alpha-beta pruning
class ChoppedfishEngine : public Engine {
public:
    explicit ChoppedfishEngine(int max_depth = 4);
    
    Move get_best_move(const Position& position) override;
    std::string name() const override { return "ChoppedfishEngine"; }

private:
    int max_depth_;
    
    // Negamax with alpha-beta pruning
    // Returns the best score from the current player's perspective
    // Always maximizes; perspective is handled by negating recursive calls
    int alphabeta(Position& position, int depth, int alpha, int beta);
};

}
