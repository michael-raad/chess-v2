#pragma once

#include "game.hpp"
#include "engine.hpp"
#include <string>
#include <memory>
#include <thread>
#include <atomic>

namespace chess {

// UCI (Universal Chess Interface) protocol implementation
// Communicates with external GUIs via stdin/stdout
class UCI {
public:
    UCI();
    
    // Main loop: reads commands from stdin and processes them
    void run();
    
private:
    void handle_uci();
    void handle_isready();
    void handle_position(const std::string& line);
    void handle_go(const std::string& line);
    void handle_stop();
    void handle_quit();
    void handle_setoption(const std::string& line);
    
    // Helper methods
    std::string square_to_uci(int from, int to, int promo = 0) const;
    std::optional<Move> uci_to_move(const std::string& uci_move) const;
    
    // Search in background thread
    void search_thread(int depth, long long movetime_ms);
    
    std::unique_ptr<Game> game_;
    std::unique_ptr<Engine> uci_engine_;
    std::atomic<bool> stop_search_;
    std::thread search_thread_;
    
    // Search parameters
    int search_depth_;
    long long search_movetime_;
    bool ponder_;
};

}  // namespace chess
