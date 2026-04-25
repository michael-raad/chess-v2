#include "uci.hpp"
#include "config.hpp"
#include "experimental_engine.hpp"
#include "choppedfish_engine.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <vector>

namespace chess {

UCI::UCI()
    : game_(std::make_unique<Game>(PlayerType::AI, PlayerType::AI)),
#ifdef UCI_ENGINE_TYPE
#if UCI_ENGINE_TYPE == 1
      uci_engine_(std::make_unique<ExperimentalEngine>()),
#elif UCI_ENGINE_TYPE == 2
      uci_engine_(std::make_unique<ChoppedfishEngine>(4)),
#else
      uci_engine_(std::make_unique<ExperimentalEngine>()),  // Default to experimental
#endif
#else
      uci_engine_(std::make_unique<ExperimentalEngine>()),  // Default to experimental
#endif
      stop_search_(false),
      search_depth_(20),
      search_movetime_(0),
      ponder_(false) {
}

void UCI::run() {
    // Initial startup - announce readiness
    std::string line;
    
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        
        // Parse command
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        
        if (cmd == "uci") {
            handle_uci();
        } else if (cmd == "isready") {
            handle_isready();
        } else if (cmd == "position") {
            handle_position(line);
        } else if (cmd == "go") {
            handle_go(line);
        } else if (cmd == "stop") {
            handle_stop();
        } else if (cmd == "setoption") {
            handle_setoption(line);
        } else if (cmd == "quit") {
            handle_quit();
            break;
        }
    }
}

void UCI::handle_uci() {
    std::cout << "id name " << uci_engine_->name() << std::endl;
    std::cout << "id author Michael" << std::endl;
    // Optional: add options here
    // std::cout << "option name Hash type spin default 32 min 1 max 1024" << std::endl;
    std::cout << "uciok" << std::endl;
}

void UCI::handle_isready() {
    std::cout << "readyok" << std::endl;
}

void UCI::handle_position(const std::string& line) {
    std::istringstream iss(line);
    std::string cmd, pos_type;
    iss >> cmd >> pos_type;
    
    if (pos_type == "startpos") {
        game_->set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    } else if (pos_type == "fen") {
        // Read FEN string (may contain spaces)
        std::string fen;
        for (int i = 0; i < 6; i++) {
            std::string part;
            iss >> part;
            if (i > 0) fen += " ";
            fen += part;
        }
        game_->set_fen(fen);
    }
    
    // Parse moves if present
    std::string moves_cmd;
    if (iss >> moves_cmd && moves_cmd == "moves") {
        std::string move_str;
        while (iss >> move_str) {
            auto move = uci_to_move(move_str);
            if (move) {
                game_->try_move(move->from, move->to, move->promo);
            }
        }
    }
}

void UCI::handle_go(const std::string& line) {
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;  // consume 'go'
    
    // Reset search parameters
    search_depth_ = 20;
    search_movetime_ = 0;
    ponder_ = false;
    
    // Parse go parameters
    std::string param;
    while (iss >> param) {
        if (param == "depth") {
            iss >> search_depth_;
        } else if (param == "movetime") {
            iss >> search_movetime_;
        } else if (param == "wtime") {
            long long wtime;
            iss >> wtime;
            // Could implement time management here
        } else if (param == "btime") {
            long long btime;
            iss >> btime;
            // Could implement time management here
        } else if (param == "infinite") {
            search_depth_ = 100;  // Very deep
        } else if (param == "ponder") {
            ponder_ = true;
        }
    }
    
    // Stop any existing search
    stop_search_ = true;
    if (search_thread_.joinable()) {
        search_thread_.join();
    }
    
    // Start new search
    stop_search_ = false;
    search_thread_ = std::thread(&UCI::search_thread, this, search_depth_, search_movetime_);
}

void UCI::handle_stop() {
    stop_search_ = true;
    if (search_thread_.joinable()) {
        search_thread_.join();
    }
}

void UCI::handle_quit() {
    stop_search_ = true;
    if (search_thread_.joinable()) {
        search_thread_.join();
    }
}

void UCI::handle_setoption(const std::string& /* line */) {
    // Parse: setoption name <name> value <value>
    // For now, we'll just acknowledge it
    // Could implement hash size, etc. here
}

std::string UCI::square_to_uci(int from, int to, int promo) const {
    char from_file = 'a' + (from % 8);
    char from_rank = '1' + (from / 8);
    char to_file = 'a' + (to % 8);
    char to_rank = '1' + (to / 8);
    
    std::string result;
    result += from_file;
    result += from_rank;
    result += to_file;
    result += to_rank;
    
    // Add promotion piece if applicable
    if (promo != 0) {
        switch (promo) {
            case 1: result += 'n'; break;  // Knight
            case 2: result += 'b'; break;  // Bishop
            case 3: result += 'r'; break;  // Rook
            case 4: result += 'q'; break;  // Queen
        }
    }
    
    return result;
}

std::optional<Move> UCI::uci_to_move(const std::string& uci_move) const {
    if (uci_move.length() < 4) {
        return std::nullopt;
    }
    
    int from_file = uci_move[0] - 'a';
    int from_rank = uci_move[1] - '1';
    int to_file = uci_move[2] - 'a';
    int to_rank = uci_move[3] - '1';
    
    // Validate coordinates
    if (from_file < 0 || from_file > 7 || from_rank < 0 || from_rank > 7 ||
        to_file < 0 || to_file > 7 || to_rank < 0 || to_rank > 7) {
        return std::nullopt;
    }
    
    int from = from_rank * 8 + from_file;
    int to = to_rank * 8 + to_file;
    
    int promo = 0;
    if (uci_move.length() > 4) {
        switch (uci_move[4]) {
            case 'n': promo = 1; break;  // Knight
            case 'b': promo = 2; break;  // Bishop
            case 'r': promo = 3; break;  // Rook
            case 'q': promo = 4; break;  // Queen
            default: break;
        }
    }
    
    return Move(from, to, promo);
}

void UCI::search_thread(int depth, long long movetime_ms) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Set up search parameters for the engine (empty history for now)
    std::unordered_map<uint64_t, int> history;
    uci_engine_->set_position_history(history);
    
    std::cerr << "[UCI] Starting search at depth " << depth << " with time " << movetime_ms << " ms" << std::endl;
    
    // Perform search with iterative deepening
    MoveEvaluation best_eval{Move(0, 0, 0), 0};
    
    for (int d = 1; d <= depth; d++) {
        // Check time limit if specified
        if (movetime_ms > 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - start_time).count();
            if (elapsed >= movetime_ms) {
                std::cerr << "[UCI] Time limit reached at depth " << d << std::endl;
                break;
            }
        }
        
        std::cerr << "[UCI] Searching depth " << d << std::endl;
        
        // Get evaluation at this depth - pass depth and remaining time to the engine
        long long remaining_time = movetime_ms > 0 ? movetime_ms - std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start_time).count() : 0;
        
        MoveEvaluation eval = uci_engine_->get_best_move(game_->get_position(), d, remaining_time);
        
        std::cerr << "[UCI] Got move: " << eval.move.from << "->" << eval.move.to << " score: " << eval.score << std::endl;
        
        best_eval = eval;
        
        // Send info to GUI (flush after each line)
        std::cout << "info depth " << d << " score cp " << eval.score << std::endl;
        std::cout.flush();
        
        // Check stop flag after each depth (allows stopping between depths)
        if (stop_search_) {
            std::cerr << "[UCI] Search stopped at depth " << d << std::endl;
            break;
        }
    }
    
    std::cerr << "[UCI] Returning bestmove: " << best_eval.move.from << "->" << best_eval.move.to << std::endl;
    // Send best move
    std::cout << "bestmove " << square_to_uci(best_eval.move.from, best_eval.move.to, best_eval.move.promo);
    std::cout << std::endl;
    std::cout.flush();
}

}  // namespace chess
