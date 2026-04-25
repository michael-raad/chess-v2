#include "uci_client.hpp"
#include "config.hpp"
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace chess {

UIClient::UIClient(const std::string& engine_path)
    : engine_path_(engine_path.empty() ? WHITE_ENGINE_BIN_PATH : engine_path),
      engine_pipe_(nullptr),
      engine_write_fd_(-1),
      is_alive_(false),
      search_in_progress_(false) {
    std::cerr << "[UIClient] Using engine at: " << engine_path_ << std::endl;
}

UIClient::~UIClient() {
    if (engine_pipe_) {
        send_command("quit");
        fclose(engine_pipe_);
        engine_pipe_ = nullptr;
    }
    if (engine_write_fd_ != -1) {
        close(engine_write_fd_);
        engine_write_fd_ = -1;
    }
    is_alive_ = false;
}

bool UIClient::initialize() {
    std::cerr << "[UIClient] Initializing engine..." << std::endl;
    
    // Create pipes for communication with the engine
    int to_engine[2];      // parent writes, engine reads
    int from_engine[2];    // engine writes, parent reads
    
    if (pipe(to_engine) == -1 || pipe(from_engine) == -1) {
        std::cerr << "ERROR: Failed to create pipes" << std::endl;
        return false;
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "ERROR: Failed to fork" << std::endl;
        close(to_engine[0]);
        close(to_engine[1]);
        close(from_engine[0]);
        close(from_engine[1]);
        return false;
    }
    
    if (pid == 0) {
        // Child process - engine
        close(to_engine[1]);    // close write end
        close(from_engine[0]);  // close read end
        
        // Redirect stdin/stdout
        dup2(to_engine[0], STDIN_FILENO);
        dup2(from_engine[1], STDOUT_FILENO);
        
        // Close original FDs
        close(to_engine[0]);
        close(from_engine[1]);
        
        // Execute engine
        execlp(engine_path_.c_str(), engine_path_.c_str(), nullptr);
        
        // If we get here, exec failed
        std::cerr << "Failed to exec: " << engine_path_ << std::endl;
        exit(1);
    } else {
        // Parent process
        std::cerr << "[UIClient] Forked child process (PID " << pid << ")" << std::endl;
        
        close(to_engine[0]);    // close read end
        close(from_engine[1]);  // close write end
        
        engine_write_fd_ = to_engine[1];
        FILE* read_pipe = fdopen(from_engine[0], "r");
        
        if (!read_pipe) {
            std::cerr << "ERROR: Failed to create read FILE*" << std::endl;
            close(engine_write_fd_);
            return false;
        }
        
        engine_pipe_ = read_pipe;
        setvbuf(engine_pipe_, nullptr, _IOLBF, 0);
    }
    
    // Send uci command and wait for uciok
    std::cerr << "[UIClient] Sending 'uci' command..." << std::endl;
    send_command("uci");
    
    bool got_uciok = false;
    for (int i = 0; i < 100; i++) {
        auto line = read_line();
        if (!line) break;
        
        if (line->find("uciok") != std::string::npos) {
            got_uciok = true;
            break;
        }
    }
    
    if (!got_uciok) {
        std::cerr << "Engine did not respond with uciok" << std::endl;
        close(engine_write_fd_);
        fclose(engine_pipe_);
        engine_pipe_ = nullptr;
        engine_write_fd_ = -1;
        return false;
    }
    
    std::cerr << "[UIClient] Engine initialized successfully" << std::endl;
    is_alive_ = true;
    return true;
}

void UIClient::set_position(const std::optional<std::string>& fen, 
                             const std::vector<std::string>& move_history) {
    if (!is_alive_) return;
    
    // Build position command
    std::string cmd = "position ";
    
    if (fen.has_value()) {
        cmd += "fen " + fen.value();
    } else {
        cmd += "startpos";
    }
    
    // Add moves if any
    if (!move_history.empty()) {
        cmd += " moves";
        for (const auto& move : move_history) {
            cmd += " " + move;
        }
    }
    
    send_command(cmd);
}

std::optional<MoveEvaluation> UIClient::get_best_move(int depth, long long movetime_ms) {
    if (!is_alive_) {
        std::cerr << "ERROR: Engine is not alive" << std::endl;
        return std::nullopt;
    }
    
    search_in_progress_ = true;
    
    // Build go command
    std::string go_cmd = "go";
    if (depth > 0) {
        go_cmd += " depth " + std::to_string(depth);
    }
    if (movetime_ms > 0) {
        go_cmd += " movetime " + std::to_string(movetime_ms);
    }
    if (depth <= 0 && movetime_ms <= 0) {
        go_cmd += " depth 20";  // Default depth if neither specified
    }
    
    send_command(go_cmd);
    
    // Read responses until we get bestmove
    std::optional<MoveEvaluation> result;
    int max_reads = 10000;  // Prevent infinite loop
    
    for (int i = 0; i < max_reads; i++) {
        auto line = read_line();
        if (!line) {
            std::cerr << "ERROR: Engine closed pipe during search" << std::endl;
            break;
        }
        
        // Skip info lines for now (could parse them for GUI display later)
        if (line->find("info") == 0) {
            continue;
        }
        
        if (line->find("bestmove") == 0) {
            result = parse_bestmove(*line);
            if (result) {
                std::cerr << "DEBUG: Got bestmove " << result->move.from << "->" << result->move.to << std::endl;
            } else {
                std::cerr << "ERROR: Failed to parse bestmove: " << *line << std::endl;
            }
            break;
        }
    }
    
    search_in_progress_ = false;
    return result;
}

void UIClient::stop_search() {
    if (is_alive_ && search_in_progress_) {
        send_command("stop");
    }
}

void UIClient::quit() {
    if (engine_pipe_) {
        send_command("quit");
        fclose(engine_pipe_);
        engine_pipe_ = nullptr;
    }
    if (engine_write_fd_ != -1) {
        close(engine_write_fd_);
        engine_write_fd_ = -1;
    }
    is_alive_ = false;
}

bool UIClient::is_alive() const {
    return is_alive_;
}

void UIClient::send_command(const std::string& cmd) {
    if (engine_write_fd_ == -1) return;
    
    std::string full_cmd = cmd + "\n";
    ssize_t written = write(engine_write_fd_, full_cmd.c_str(), full_cmd.length());
    if (written == -1) {
        std::cerr << "ERROR: Failed to write to engine" << std::endl;
    }
}

std::optional<std::string> UIClient::read_line() {
    if (!engine_pipe_) return std::nullopt;
    
    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), engine_pipe_) != nullptr) {
        // Remove trailing newline
        std::string line(buffer);
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
        }
        return line;
    }
    
    return std::nullopt;
}

std::optional<MoveEvaluation> UIClient::parse_bestmove(const std::string& line) {
    // Parse: "bestmove e2e4" or "bestmove e7e8q ponder e8f7"
    std::istringstream iss(line);
    std::string bestmove_cmd, move_str;
    
    iss >> bestmove_cmd >> move_str;
    
    if (bestmove_cmd != "bestmove" || move_str.empty()) {
        return std::nullopt;
    }
    
    auto move = parse_uci_move(move_str);
    if (!move) {
        return std::nullopt;
    }
    
    // For now, return score of 0 (could enhance to track score from info lines)
    return MoveEvaluation{*move, 0};
}

std::optional<Move> UIClient::parse_uci_move(const std::string& uci_str) const {
    if (uci_str.length() < 4) {
        return std::nullopt;
    }
    
    int from_file = uci_str[0] - 'a';
    int from_rank = uci_str[1] - '1';
    int to_file = uci_str[2] - 'a';
    int to_rank = uci_str[3] - '1';
    
    // Validate coordinates
    if (from_file < 0 || from_file > 7 || from_rank < 0 || from_rank > 7 ||
        to_file < 0 || to_file > 7 || to_rank < 0 || to_rank > 7) {
        return std::nullopt;
    }
    
    int from = from_rank * 8 + from_file;
    int to = to_rank * 8 + to_file;
    
    int promo = 0;
    if (uci_str.length() > 4) {
        switch (uci_str[4]) {
            case 'n': promo = 1; break;  // Knight
            case 'b': promo = 2; break;  // Bishop
            case 'r': promo = 3; break;  // Rook
            case 'q': promo = 4; break;  // Queen
            default: break;
        }
    }
    
    return Move(from, to, promo);
}

}  // namespace chess
