#include "uci.hpp"
#include "config.hpp"
#include "position_engine.hpp"
#include "pv_engine.hpp"
#include "material_engine.hpp"
#include "move_notation.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <cctype>

namespace chess {

UCI::UCI()
    : game_(std::make_unique<Game>(PlayerType::AI, PlayerType::AI)),
#ifdef UCI_ENGINE_TYPE
#if UCI_ENGINE_TYPE == 1
    uci_engine_(std::make_unique<PositionEngine>()),
#elif UCI_ENGINE_TYPE == 2
    uci_engine_(std::make_unique<MaterialEngine>()),
#elif UCI_ENGINE_TYPE == 3
    uci_engine_(std::make_unique<PVEngine>()),
#else
    uci_engine_(std::make_unique<PositionEngine>()),  // Default to position engine
#endif
#else
    uci_engine_(std::make_unique<PositionEngine>()),  // Default to position engine
#endif
      stop_search_(false),
      search_depth_(20),
      search_movetime_(0),
    ponder_(false),
    hash_mb_(32),
    threads_(1) {
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
        } else if (cmd == "ucinewgame") {
            handle_ucinewgame();
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
    std::cout << "option name Hash type spin default 32 min 1 max 4096" << std::endl;
    std::cout << "option name Threads type spin default 1 min 1 max 128" << std::endl;
    std::cout << "uciok" << std::endl;
}

void UCI::handle_isready() {
    std::cout << "readyok" << std::endl;
}

void UCI::handle_ucinewgame() {
    // Stop any in-flight search and reset per-game state.
    stop_search_ = true;
    if (search_thread_.joinable()) {
        search_thread_.join();
    }

    game_ = std::make_unique<Game>(PlayerType::AI, PlayerType::AI);
    stop_search_ = false;
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
                // UCI must apply moves regardless of player type.
                game_->apply_move(move->from, move->to, move->promo);
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

    long long wtime = -1;
    long long btime = -1;
    long long winc = 0;
    long long binc = 0;
    int movestogo = 0;
    bool infinite = false;
    
    // Parse go parameters
    std::string param;
    while (iss >> param) {
        if (param == "depth") {
            iss >> search_depth_;
        } else if (param == "movetime") {
            iss >> search_movetime_;
        } else if (param == "wtime") {
            iss >> wtime;
        } else if (param == "btime") {
            iss >> btime;
        } else if (param == "winc") {
            iss >> winc;
        } else if (param == "binc") {
            iss >> binc;
        } else if (param == "movestogo") {
            iss >> movestogo;
        } else if (param == "infinite") {
            infinite = true;
            search_depth_ = 100;  // Very deep
        } else if (param == "ponder") {
            ponder_ = true;
        }
    }

    // If movetime wasn't provided, derive a practical per-move budget from clock time.
    if (!infinite && search_movetime_ <= 0 && (wtime >= 0 || btime >= 0)) {
        search_movetime_ = compute_time_budget_ms(wtime, btime, winc, binc, movestogo);
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

void UCI::handle_setoption(const std::string& line) {
    std::istringstream iss(line);
    std::string token;
    iss >> token; // setoption

    std::string name;
    std::string value;

    while (iss >> token) {
        if (token == "name") {
            name.clear();
            while (iss >> token && token != "value") {
                if (!name.empty()) name += " ";
                name += token;
            }
            if (token != "value") {
                value.clear();
                break;
            }
            std::getline(iss, value);
            if (!value.empty() && value.front() == ' ') {
                value.erase(value.begin());
            }
            break;
        }
    }

    std::string name_lower = name;
    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (name_lower == "hash") {
        try {
            int parsed = std::stoi(value);
            hash_mb_ = std::max(1, std::min(parsed, 4096));
        } catch (...) {
            // Ignore malformed value, keep previous setting.
        }
    } else if (name_lower == "threads") {
        try {
            int parsed = std::stoi(value);
            threads_ = std::max(1, std::min(parsed, 128));
        } catch (...) {
            // Ignore malformed value, keep previous setting.
        }
    }
}

long long UCI::compute_time_budget_ms(long long wtime, long long btime,
                                      long long winc, long long binc,
                                      int movestogo) const {
    const Color stm = game_->get_position().side_to_move();
    long long remain = (stm == WHITE) ? wtime : btime;
    long long inc = (stm == WHITE) ? winc : binc;

    if (remain < 0) {
        return 0;
    }

    // Conservative heuristic: divide time by remaining moves and add most of increment.
    int moves = movestogo > 0 ? movestogo : 30;
    long long alloc = remain / std::max(1, moves);
    alloc += (inc * 8) / 10;

    // Keep a small safety buffer and clamp to sane bounds.
    alloc = std::max(1LL, alloc - 20);
    alloc = std::min(alloc, std::max(1LL, remain - 10));
    return alloc;
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
    // Provide real game repetition history so the engine can detect imminent draws.
    uci_engine_->set_position_history(game_->get_position_history());
    uci_engine_->set_stop_flag(&stop_search_);
    
    std::cerr << "[UCI] Starting search at depth " << depth << " with time " << movetime_ms << " ms" << std::endl;

    MoveEvaluation best_eval = uci_engine_->get_best_move(game_->get_position(), depth, movetime_ms);
    std::cerr << "[UCI] Returning bestmove: " << format_move_for_log(best_eval.move) << std::endl;

    // Send one info line for compatibility/logging.
    std::cout << "info depth " << depth << " score cp " << best_eval.score << std::endl;
    std::cout.flush();

    // Send best move
    std::cout << "bestmove " << square_to_uci(best_eval.move.from, best_eval.move.to, best_eval.move.promo);
    std::cout << std::endl;
    std::cout.flush();
}

}  // namespace chess
