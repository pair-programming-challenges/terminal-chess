#pragma once

#include <optional>

#include "chess/board.hpp"
#include "chess/types.hpp"

namespace chess {

struct CastlingRights {
  [[nodiscard]] constexpr bool can_kingside(Color c) const {
    return c == Color::White ? white_kingside : black_kingside;
  }

  [[nodiscard]] constexpr bool can_queenside(Color c) const {
    return c == Color::White ? white_queenside : black_queenside;
  }

  [[nodiscard]] constexpr bool operator==(const CastlingRights&) const = default;

  bool white_kingside = true;
  bool white_queenside = true;
  bool black_kingside = true;
  bool black_queenside = true;
};

struct GameState {
  [[nodiscard]] static GameState standard() {
    return GameState{
        .board = Board::standard(),
        .active_color = Color::White,
        .castling = CastlingRights{},
        .en_passant_target = std::nullopt,
        .halfmove_clock = 0,
        .fullmove_number = 1,
    };
  }

  Board board;
  Color active_color = Color::White;
  CastlingRights castling;
  std::optional<Square> en_passant_target;
  int halfmove_clock = 0;
  int fullmove_number = 1;
};

}  // namespace chess
