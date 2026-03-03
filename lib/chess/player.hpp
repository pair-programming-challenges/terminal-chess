#pragma once

#include <random>
#include <span>
#include <stdexcept>

#include "chess/game_state.hpp"
#include "chess/move.hpp"

namespace chess {

class Player {
 public:
  virtual ~Player() = default;

  [[nodiscard]] virtual Move choose_move(const GameState& state, std::span<const Move> legal_moves) = 0;

 protected:
  Player() = default;
  Player(const Player&) = default;
  Player& operator=(const Player&) = default;
  Player(Player&&) = default;
  Player& operator=(Player&&) = default;
};

class RandomPlayer final : public Player {
 public:
  explicit RandomPlayer(uint32_t seed = std::random_device{}()) : rng_(seed) {}

  [[nodiscard]] Move choose_move(const GameState& /*state*/, std::span<const Move> legal_moves) override final {
    if (legal_moves.empty()) {
      throw std::runtime_error("RandomPlayer::choose_move called with empty move list");
    }
    auto dist = std::uniform_int_distribution<size_t>(0, legal_moves.size() - 1);
    return legal_moves[dist(rng_)];
  }

 private:
  std::mt19937 rng_;
};

}  // namespace chess
