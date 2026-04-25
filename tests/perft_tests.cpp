#include <catch2/catch_test_macros.hpp>
#include "position.hpp"
#include "search.hpp"
#include "attacks.hpp"

using namespace chess;

// Initialize attack tables before running tests using RAII
static struct Initializer {
  Initializer() { 
    AttackTablesInitializer init;
  }
} initializer;

// Test perft on initial position
TEST_CASE("perft initial position depth 1-4", "[perft]") {
  Position pos;
  PerftStats expected[] = {
    {20, 0, 0, 0, 0, 0, 0}, // depth 1
    {400, 0, 0, 0, 0, 0, 0}, // depth 2
    {8902, 34, 0, 0, 0, 12, 0}, // depth 3
    {197281, 1576, 0, 0, 0, 469, 8}, // depth 4
    {4865609, 82719, 258, 0, 0, 27351, 347} // depth 5
  };
  for (int depth = 1; depth <= 5; ++depth) {
    pos.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    PerftStats stats = perft(pos, depth);
    stats.print("Initial Position", depth);
    REQUIRE(stats.nodes == expected[depth - 1].nodes);
    REQUIRE(stats.captures == expected[depth - 1].captures);
    REQUIRE(stats.en_passants == expected[depth - 1].en_passants);
    REQUIRE(stats.castles == expected[depth - 1].castles);  
    REQUIRE(stats.promotions == expected[depth - 1].promotions);
    REQUIRE(stats.checks == expected[depth - 1].checks);
    REQUIRE(stats.checkmates == expected[depth - 1].checkmates);
  }
}

TEST_CASE("perft kiwipete position depth 1-4", "[perft]") {
  Position pos;
  PerftStats expected[] = {
    {48, 8, 0, 2, 0, 0, 0}, // depth 1
    {2039, 351, 1, 91, 0, 3, 0}, // depth 2
    {97862, 17102, 45, 3162, 0, 993, 1}, // depth 3
    {4085603, 757163, 1929, 128013, 15172, 25523, 43} // depth 4
  };
  for (int depth = 1; depth <= 4; ++depth) {
    pos.set_from_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    PerftStats stats = perft(pos, depth);
    stats.print("Kiwipete Position", depth);
    REQUIRE(stats.nodes == expected[depth - 1].nodes);
    REQUIRE(stats.captures == expected[depth - 1].captures);
    REQUIRE(stats.en_passants == expected[depth - 1].en_passants);
    REQUIRE(stats.castles == expected[depth - 1].castles);  
    REQUIRE(stats.promotions == expected[depth - 1].promotions);
    REQUIRE(stats.checks == expected[depth - 1].checks);
    REQUIRE(stats.checkmates == expected[depth - 1].checkmates);
  }
}

TEST_CASE("perft position 3 depth 1-4", "[perft]") {
  Position pos;
  PerftStats expected[] = {
    {14, 1, 0, 0, 0, 2, 0}, // depth 1
    {191, 14, 0, 0, 0, 10, 0}, // depth 2
    {2812, 209, 2, 0, 0, 267, 0}, // depth 3
    {43238, 3348, 123, 0, 0, 1680, 17} // depth 4
  };
  for (int depth = 1; depth <= 4; ++depth) {
    pos.set_from_fen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    PerftStats stats = perft(pos, depth);
    stats.print("Position 3", depth);
    REQUIRE(stats.nodes == expected[depth - 1].nodes);
    REQUIRE(stats.captures == expected[depth - 1].captures);
    REQUIRE(stats.en_passants == expected[depth - 1].en_passants);
    REQUIRE(stats.castles == expected[depth - 1].castles);  
    REQUIRE(stats.promotions == expected[depth - 1].promotions);
    REQUIRE(stats.checks == expected[depth - 1].checks);
    REQUIRE(stats.checkmates == expected[depth - 1].checkmates);
  }
}

TEST_CASE("perft position 4 depth 1-4", "[perft]") {
  Position pos;
  PerftStats expected[] = {
    {6, 0, 0, 0, 0, 0, 0}, // depth 1
    {264, 87, 0, 6, 48, 10, 0}, // depth 2
    {9467, 1021, 4, 0, 120, 38, 22}, // depth 3
    {422333, 131393, 0, 7795, 60032, 15492, 5} // depth 4
  };
  for (int depth = 1; depth <= 4; ++depth) {
    pos.set_from_fen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    PerftStats stats = perft(pos, depth);
    stats.print("Position 4", depth);
    REQUIRE(stats.nodes == expected[depth - 1].nodes);
    REQUIRE(stats.captures == expected[depth - 1].captures);
    REQUIRE(stats.en_passants == expected[depth - 1].en_passants);
    REQUIRE(stats.castles == expected[depth - 1].castles);  
    REQUIRE(stats.promotions == expected[depth - 1].promotions);
    REQUIRE(stats.checks == expected[depth - 1].checks);
    REQUIRE(stats.checkmates == expected[depth - 1].checkmates);
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
