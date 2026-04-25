#pragma once

#include "position.hpp"
#include "engine.hpp"
#include <string>
#include <memory>
#include <optional>
#include <vector>
#include <thread>
#include <atomic>

namespace chess {

// Communicates with a UCI engine subprocess
// Handles command sending and response parsing
class UIClient {
public:
    explicit UIClient(const std::string& engine_path = "");
    ~UIClient();
    
    // Initialize connection (sends "uci", waits for "uciok")
    bool initialize();
    
    // Set position for the engine
    // fen: optional FEN string. If provided, sets position from FEN. If not provided, uses startpos.
    // move_history: optional moves to apply after the position (in algebraic notation, e.g., "e2e4", "e7e8q")
    // Examples:
    //   set_position()  ->  "position startpos"
    //   set_position(some_fen)  ->  "position fen <fen>"
    //   set_position({}, moves)  ->  "position startpos moves ..."
    //   set_position(some_fen, moves)  ->  "position fen <fen> moves ..."
    void set_position(const std::optional<std::string>& fen = std::nullopt, 
                      const std::vector<std::string>& move_history = {});
    
    // Request best move from engine
    // depth: search depth (-1 for time-based)
    // movetime_ms: time to search in milliseconds (0 = infinite)
    // Returns: move and evaluation score
    std::optional<MoveEvaluation> get_best_move(int depth, long long movetime_ms = 0);
    
    // Stop current search
    void stop_search();
    
    // Graceful shutdown
    void quit();
    
    // Check if engine is running
    bool is_alive() const;
    
private:
    std::string engine_path_;
    FILE* engine_pipe_;
    int engine_write_fd_;  // File descriptor for writing to engine
    bool is_alive_;
    std::atomic<bool> search_in_progress_;
    
    // Send raw command to engine
    void send_command(const std::string& cmd);
    
    // Read one line from engine output
    std::optional<std::string> read_line();
    
    // Parse "bestmove" response
    std::optional<MoveEvaluation> parse_bestmove(const std::string& line);
    
    // Convert move in algebraic notation to Move struct
    std::optional<Move> parse_uci_move(const std::string& uci_str) const;
};

}  // namespace chess
