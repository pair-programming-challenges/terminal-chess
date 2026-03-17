#pragma once

#include <algorithm>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

#include "chess/display.hpp"
#include "chess/move.hpp"
#include "chess/player.hpp"

namespace chess {

class HumanPlayer final : public Player {
 public:
  explicit HumanPlayer(Display& display) : display_(display) {}

  [[nodiscard]] Move choose_move(const GameState& state, std::span<const Move> legal_moves) override final {
    Square cursor{.row = 0, .col = 0};
    std::optional<Square> selected;
    std::vector<Square> highlights;

    display_.show_message("Your turn. Select a piece.");

    while (true) {
      display_.render_board(state, cursor, selected, highlights);
      auto event = display_.get_input();

      switch (event.type) {
        case InputType::ArrowUp:
          if (cursor.row < 7) cursor.row++;
          break;
        case InputType::ArrowDown:
          if (cursor.row > 0) cursor.row--;
          break;
        case InputType::ArrowLeft:
          if (cursor.col > 0) cursor.col--;
          break;
        case InputType::ArrowRight:
          if (cursor.col < 7) cursor.col++;
          break;
        case InputType::Enter:
        case InputType::MouseClick: {
          Square target = (event.type == InputType::MouseClick) ? event.square : cursor;
          if (event.type == InputType::MouseClick) cursor = target;

          // Try to complete a move if a piece is already selected
          if (selected) {
            auto move = find_move(legal_moves, *selected, target);
            if (move) return *move;
          }

          // Try to select a piece at the target square
          auto piece = state.board.piece_at(target);
          if (piece && piece->color == state.active_color && has_moves_from(legal_moves, target)) {
            selected = target;
            highlights = get_targets(legal_moves, target);
            display_.show_message("Select destination.");
          } else {
            selected = std::nullopt;
            highlights.clear();
            display_.show_message("Select a piece.");
          }
          break;
        }
        case InputType::Quit:
          throw std::runtime_error("Player quit the game");
      }
    }
  }

 private:
  static std::optional<Move> find_move(std::span<const Move> moves, Square from, Square to) {
    const Move* first_match = nullptr;
    for (const auto& m : moves) {
      if (m.from == from && m.to == to) {
        if (m.type != MoveType::Promotion) return m;
        if (m.promoted_to == PieceType::Queen) return m;
        if (!first_match) first_match = &m;
      }
    }
    return first_match ? std::optional(*first_match) : std::nullopt;
  }

  static bool has_moves_from(std::span<const Move> moves, Square from) {
    return std::any_of(moves.begin(), moves.end(), [&](const Move& m) { return m.from == from; });
  }

  static std::vector<Square> get_targets(std::span<const Move> moves, Square from) {
    std::vector<Square> targets;
    for (const auto& m : moves) {
      if (m.from == from && std::find(targets.begin(), targets.end(), m.to) == targets.end()) {
        targets.push_back(m.to);
      }
    }
    return targets;
  }

  Display& display_;
};

}  // namespace chess
