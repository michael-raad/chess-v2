#pragma once

#include "engine.hpp"
#include "zobrist.hpp"
#include <array>
#include <chrono>
#include <cstdint>

namespace chess {

// PV engine - position evaluation with iterative deepening + root PV ordering.
class PVEngine : public Engine {
public:
    PVEngine() = default;
    
    // Get best move at the specified depth with optional time constraint
    // depth: search depth (default 1)
    // movetime_ms: time constraint in milliseconds (0 = no time limit, search until depth is reached)
    MoveEvaluation get_best_move(const Position& position, int depth = 1, long long movetime_ms = 0) override;
    
    int evaluate(Position& position) override;
    std::string name() const override { return "PVEngine"; }

private:
    bool should_stop_search();

    // Negamax with alpha-beta pruning
    // Returns the best score from the current player's perspective
    // Always maximizes; perspective is handled by negating recursive calls
    int alphabeta(Position& position, int depth, int alpha, int beta,
                  std::unordered_map<uint64_t, int> position_history);

    bool use_time_limit_ = false;
    std::chrono::steady_clock::time_point deadline_{};
    uint64_t node_counter_ = 0;
    bool timed_out_ = false;
};

}
