#pragma once

#include <memory>
#include <span>
#include <vector>

#include "chess/game.hpp"
#include "chess/game_state.hpp"
#include "chess/game_status.hpp"
#include "chess/move.hpp"
#include "chess/move_generator.hpp"
#include "chess/player.hpp"
#include "chess/types.hpp"

namespace chess {

class Game {
 public:
  Game(std::unique_ptr<Player> white, std::unique_ptr<Player> black,
       GameState initial_state = GameState::standard())
      : state_(initial_state), players_{std::move(white), std::move(black)} {}

  Game(const Game&) = delete;
  Game& operator=(const Game&) = delete;

  // Executes one turn: compute legal moves, detect game-end, ask the active
  // player for a move, apply it, and return the resulting status.
  GameStatus step() {
    // 1. Check 50-move draw before generating moves
    if (is_fifty_move_draw(state_)) {
      return GameStatus{
          .in_check = is_in_check(state_),
          .result = GameResult{.outcome = Outcome::Draw, .winner = std::nullopt},
      };
    }

    // 2. Generate all legal moves for the active side
    MoveGenerator gen(state_);
    auto legal_moves = gen.generate_all_legal_moves();

    // 3. Check for checkmate / stalemate (no legal moves)
    auto status = compute_status(state_, legal_moves);
    if (status.result.has_value()) {
      return status;
    }

    // 4. Ask the active player to choose a move
    Move chosen = active_player().choose_move(state_, legal_moves);

    // 5. Record the move and apply it
    history_.push_back(chosen);
    apply_move(state_, chosen);

    // 6. Recompute status after the move
    MoveGenerator post_gen(state_);
    auto post_legal_moves = post_gen.generate_all_legal_moves();
    return compute_status(state_, post_legal_moves);
  }

  [[nodiscard]] const GameState& state() const { return state_; }

  [[nodiscard]] std::span<const Move> history() const { return history_; }

 private:
  [[nodiscard]] Player& active_player() const {
    return *players_[state_.active_color == Color::White ? 0 : 1];
  }

  GameState state_;
  std::unique_ptr<Player> players_[2];  // [0]=White, [1]=Black
  std::vector<Move> history_;
};

}  // namespace chess
