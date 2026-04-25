#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>
#include "position.hpp"
#include "movegen.hpp"

namespace chess {

// Struct to hold both the best move and its evaluation score
struct MoveEvaluation {
    Move move;
    int score;
};

// Abstract base class for chess engines
class Engine {
public:
    virtual ~Engine() = default;
    
    // Get the best move and its evaluation score
    // depth: search depth (default 1 for quick evaluation)
    // movetime_ms: time constraint in milliseconds (default 0 = no time limit)
    virtual MoveEvaluation get_best_move(const Position& position, int depth = 1, long long movetime_ms = 0) = 0;
    
    // Evaluate a position from the perspective of the side to move
    // Each engine can use its own evaluation strategy
    virtual int evaluate(Position& position) = 0;
    
    // Optional: Get engine name for debugging/logging
    virtual std::string name() const { return "UnnamedEngine"; }
    
    // Set the position history for threefold repetition awareness
    void set_position_history(const std::unordered_map<uint64_t, int>& history) {
        position_history_ = history;
    }
    
protected:
    std::unordered_map<uint64_t, int> position_history_;
};

}
