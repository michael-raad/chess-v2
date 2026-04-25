#include "zobrist.hpp"
#include <random>

namespace chess {

// Zobrist hash tables
// These should be initialized once with pseudo-random numbers
// For simplicity, we'll use a seeded random generator with fixed seed
namespace {
    // Zobrist tables: 12 pieces * 64 squares
    uint64_t piece_hash[12][64];
    
    // Side to move
    uint64_t side_hash;
    
    // Castling rights: 16 combinations (2^4)
    uint64_t castling_hash[16];
    
    bool zobrist_initialized = false;
    
    void init_zobrist() {
        if (zobrist_initialized) return;
        
        // Use a fixed seed for reproducibility
        std::mt19937_64 rng(0xDEADBEEFCAFEBABEULL);
        std::uniform_int_distribution<uint64_t> dist;
        
        // Initialize piece hash table
        for (int piece = 0; piece < 12; piece++) {
            for (int square = 0; square < 64; square++) {
                piece_hash[piece][square] = dist(rng);
            }
        }
        
        // Initialize side to move hash
        side_hash = dist(rng);
        
        // Initialize castling hash
        for (int i = 0; i < 16; i++) {
            castling_hash[i] = dist(rng);
        }
        
        zobrist_initialized = true;
    }
}

uint64_t get_position_hash(const Position& pos) {
    // Ensure zobrist tables are initialized
    init_zobrist();
    
    uint64_t hash = 0;
    
    // Hash all pieces
    for (int piece = 0; piece < 12; piece++) {
        uint64_t bb = pos.bitboard(static_cast<Piece>(piece));
        while (bb) {
            int square = __builtin_ctzll(bb);
            bb &= bb - 1;
            hash ^= piece_hash[piece][square];
        }
    }
    
    // Hash side to move (only if BLACK, WHITE is default/0)
    if (pos.side_to_move() == BLACK) {
        hash ^= side_hash;
    }
    
    // Hash castling rights
    hash ^= castling_hash[pos.castling_rights()];
    
    // NOTE: We intentionally do NOT hash en passant square
    // Per FIDE rules, en passant availability doesn't affect position repetition
    
    return hash;
}

} // namespace chess
