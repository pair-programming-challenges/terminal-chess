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

[[nodiscard]] inline bool is_insufficient_material(const Board& board) {
  int white_bishops = 0, black_bishops = 0;
  int white_knights = 0, black_knights = 0;
  int white_bishop_sq_color = -1, black_bishop_sq_color = -1;

  for (int8_t row = 0; row < 8; ++row) {
    for (int8_t col = 0; col < 8; ++col) {
      auto piece = board.piece_at(Square{.row = row, .col = col});
      if (!piece) continue;
      if (piece->type == PieceType::King) continue;

      // Any pawn, rook, or queen means sufficient material
      if (piece->type == PieceType::Pawn || piece->type == PieceType::Rook || piece->type == PieceType::Queen) {
        return false;
      }

      if (piece->type == PieceType::Bishop) {
        int sq_color = (row + col) % 2;
        if (piece->color == Color::White) {
          ++white_bishops;
          white_bishop_sq_color = sq_color;
        } else {
          ++black_bishops;
          black_bishop_sq_color = sq_color;
        }
      } else if (piece->type == PieceType::Knight) {
        if (piece->color == Color::White) {
          ++white_knights;
        } else {
          ++black_knights;
        }
      }
    }
  }

  int white_minor = white_bishops + white_knights;
  int black_minor = black_bishops + black_knights;

  // K vs K
  if (white_minor == 0 && black_minor == 0) return true;

  // K+B vs K or K+N vs K (either side)
  if (white_minor == 0 && black_minor == 1) return true;
  if (white_minor == 1 && black_minor == 0) return true;

  // K+B vs K+B with same-color bishops
  if (white_bishops == 1 && white_knights == 0 && black_bishops == 1 && black_knights == 0) {
    return white_bishop_sq_color == black_bishop_sq_color;
  }

  return false;
}

[[nodiscard]] inline GameStatus compute_status(const GameState& state, const std::vector<Move>& legal_moves) {
  bool in_check = is_in_check(state);

  if (legal_moves.empty()) {
    if (in_check) {
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

  if (is_fifty_move_draw(state) || is_insufficient_material(state.board)) {
    return GameStatus{
        .in_check = in_check,
        .result = GameResult{.outcome = Outcome::Draw, .winner = std::nullopt},
    };
  }

  return GameStatus{.in_check = in_check, .result = std::nullopt};
}

}  // namespace chess
