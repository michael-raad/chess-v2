#include <catch2/catch_test_macros.hpp>
#include "position.hpp"
#include "search.hpp"
#include "attacks.hpp"

using namespace chess;

// Initialize attack tables before running tests
static struct Initializer {
  Initializer() { init_attack_tables(); }
} initializer;

// Test perft on initial position
TEST_CASE("perft initial position depth 1-4", "[perft]") {
  Position pos;
  for (int depth = 1; depth <= 5; ++depth) {
    pos.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    PerftStats stats = perft(pos, depth);
    stats.print("Initial Position", depth);
    REQUIRE(stats.nodes > 0);
  }
}

TEST_CASE("perft kiwipete position depth 1-4", "[perft]") {
  Position pos;
  for (int depth = 1; depth <= 4; ++depth) {
    pos.set_from_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    PerftStats stats = perft(pos, depth);
    stats.print("Kiwipete Position", depth);
    REQUIRE(stats.nodes > 0);
  }
}

// TEST_CASE("perft by move breakdown", "[perft]") {
//   Position pos;
//   for (int depth = 1; depth <= 5; ++depth) {
//     pos.set_from_fen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
//     perft_by_move(pos, depth);
//   }
// }

// Smoke test: FEN round-trip
// TEST_CASE("position FEN round-trip", "[position]") {
//   Position pos;
//   std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
//   REQUIRE(pos.set_from_fen(fen));
//   auto fen_out = pos.get_fen();
//   REQUIRE(fen_out.has_value());
//   // Re-parse the output and verify it's consistent
//   Position pos2;
//   REQUIRE(pos2.set_from_fen(*fen_out));
// }
