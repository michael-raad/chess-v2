#pragma once

#include "engine.hpp"
#include "zobrist.hpp"
#include <array>

namespace chess {

// Experimental engine - baseline copy of ChoppedfishEngine for benchmarking
// This is used to test improvements by comparing against a known baseline
class ExperimentalEngine : public Engine {
public:
    ExperimentalEngine() = default;
    
    // Get best move at the specified depth with optional time constraint
    // depth: search depth (default 1)
    // movetime_ms: time constraint in milliseconds (0 = no time limit, search until depth is reached)
    MoveEvaluation get_best_move(const Position& position, int depth = 1, long long movetime_ms = 0) override;
    
    int evaluate(Position& position) override;
    std::string name() const override { return "ExperimentalEngine"; }

private:
    // Negamax with alpha-beta pruning
    // Returns the best score from the current player's perspective
    // Always maximizes; perspective is handled by negating recursive calls
    int alphabeta(Position& position, int depth, int alpha, int beta,
                  std::unordered_map<uint64_t, int> position_history);
};

}
