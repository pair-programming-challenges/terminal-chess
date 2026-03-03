#include <algorithm>
#include <set>

#include <gtest/gtest.h>

#include "chess/player.hpp"

namespace chess {
namespace {

// --- Helpers ---

Square sq(std::string_view s) { return *Square::from_algebraic(s); }

// ============================================================
// RandomPlayer
// ============================================================

TEST(RandomPlayer, AlwaysReturnsALegalMove) {
  RandomPlayer player(42);
  GameState state = GameState::standard();

  std::vector<Move> legal_moves = {
      Move::normal(sq("e2"), sq("e4"), PieceType::Pawn),
      Move::normal(sq("d2"), sq("d4"), PieceType::Pawn),
      Move::normal(sq("g1"), sq("f3"), PieceType::Knight),
  };

  for (int i = 0; i < 100; ++i) {
    auto chosen = player.choose_move(state, legal_moves);
    auto it = std::find(legal_moves.begin(), legal_moves.end(), chosen);
    EXPECT_NE(it, legal_moves.end()) << "Chose a move not in the legal list";
  }
}

TEST(RandomPlayer, DeterministicWithSameSeed) {
  GameState state = GameState::standard();
  std::vector<Move> legal_moves = {
      Move::normal(sq("e2"), sq("e3"), PieceType::Pawn),
      Move::normal(sq("e2"), sq("e4"), PieceType::Pawn),
      Move::normal(sq("d2"), sq("d3"), PieceType::Pawn),
      Move::normal(sq("d2"), sq("d4"), PieceType::Pawn),
  };

  RandomPlayer player1(123);
  RandomPlayer player2(123);

  for (int i = 0; i < 50; ++i) {
    auto m1 = player1.choose_move(state, legal_moves);
    auto m2 = player2.choose_move(state, legal_moves);
    EXPECT_EQ(m1, m2) << "Seed-determinism broken at iteration " << i;
  }
}

TEST(RandomPlayer, ThrowsOnEmptyMoveList) {
  RandomPlayer player(42);
  GameState state = GameState::standard();
  std::vector<Move> empty;

  EXPECT_THROW(auto _ = player.choose_move(state, empty), std::runtime_error);
}

TEST(RandomPlayer, ChoosesVariedMoves) {
  RandomPlayer player(99);
  GameState state = GameState::standard();
  std::vector<Move> legal_moves = {
      Move::normal(sq("e2"), sq("e3"), PieceType::Pawn),
      Move::normal(sq("e2"), sq("e4"), PieceType::Pawn),
      Move::normal(sq("d2"), sq("d3"), PieceType::Pawn),
      Move::normal(sq("d2"), sq("d4"), PieceType::Pawn),
      Move::normal(sq("g1"), sq("f3"), PieceType::Knight),
  };

  std::set<size_t> chosen_indices;
  for (int i = 0; i < 200; ++i) {
    auto chosen = player.choose_move(state, legal_moves);
    auto it = std::find(legal_moves.begin(), legal_moves.end(), chosen);
    chosen_indices.insert(static_cast<size_t>(std::distance(legal_moves.begin(), it)));
  }
  // With 200 draws from 5 options, we expect all to be hit
  EXPECT_EQ(chosen_indices.size(), legal_moves.size());
}

}  // namespace
}  // namespace chess
