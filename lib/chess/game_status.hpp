#pragma once

#include <optional>
#include <vector>

#include "chess/game_state.hpp"
#include "chess/move.hpp"
#include "chess/move_generator.hpp"
#include "chess/types.hpp"

namespace chess {

enum class Outcome : uint8_t { Checkmate, Stalemate, Draw };

struct GameResult {
  Outcome outcome;
  std::optional<Color> winner;
};

struct GameStatus {
  bool in_check = false;
  std::optional<GameResult> result;
};

[[nodiscard]] inline bool is_in_check(const GameState& state) {
  Square king_sq = MoveGenerator::find_king(state.board, state.active_color);
  return MoveGenerator::is_square_attacked(state.board, king_sq, opposite(state.active_color));
}

[[nodiscard]] inline bool is_fifty_move_draw(const GameState& state) { return state.halfmove_clock >= 100; }

[[nodiscard]] inline GameStatus compute_status(const GameState& state, const std::vector<Move>& legal_moves) {
  bool in_check = is_in_check(state);

  if (legal_moves.empty()) {
    if (in_check) {
      // Checkmate — the opponent (who just moved) wins
      return GameStatus{
          .in_check = true,
          .result = GameResult{.outcome = Outcome::Checkmate, .winner = opposite(state.active_color)},
      };
    }
    return GameStatus{
        .in_check = false,
        .result = GameResult{.outcome = Outcome::Stalemate, .winner = std::nullopt},
    };
  }

  if (is_fifty_move_draw(state)) {
    return GameStatus{
        .in_check = in_check,
        .result = GameResult{.outcome = Outcome::Draw, .winner = std::nullopt},
    };
  }

  return GameStatus{.in_check = in_check, .result = std::nullopt};
}

}  // namespace chess
