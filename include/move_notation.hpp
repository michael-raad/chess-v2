#pragma once

#include <string>
#include "movegen.hpp"

namespace chess {

inline char promotion_piece_char(int promo) {
    switch (promo) {
        case 1: return 'n';
        case 2: return 'b';
        case 3: return 'r';
        case 4: return 'q';
        default: return '\0';
    }
}

inline std::string square_to_notation(int sq) {
    std::string out;
    out += static_cast<char>('a' + (sq % 8));
    out += static_cast<char>('1' + (sq / 8));
    return out;
}

inline std::string format_move_for_log(const Move& move) {
    std::string out = square_to_notation(move.from) + " -> " + square_to_notation(move.to);
    char promo = promotion_piece_char(move.promo);
    if (promo != '\0') {
        out += " ";
        out += promo;
    }
    return out;
}

}  // namespace chess
