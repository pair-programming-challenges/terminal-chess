#pragma once

#include <optional>

#include "chess/types.hpp"

namespace chess {

enum class MoveType : uint8_t { Normal, Castle, EnPassant, Promotion };

struct Move {
  [[nodiscard]] constexpr bool operator==(const Move&) const = default;

  // --- Static factories ---

  [[nodiscard]] static constexpr Move normal(Square from, Square to, PieceType piece) {
    return Move{.from = from, .to = to, .piece = piece, .captured = std::nullopt, .promoted_to = std::nullopt, .type = MoveType::Normal};
  }

  [[nodiscard]] static constexpr Move capture(Square from, Square to, PieceType piece, PieceType captured) {
    return Move{.from = from, .to = to, .piece = piece, .captured = captured, .promoted_to = std::nullopt, .type = MoveType::Normal};
  }

  [[nodiscard]] static constexpr Move en_passant(Square from, Square to) {
    return Move{.from = from, .to = to, .piece = PieceType::Pawn, .captured = PieceType::Pawn, .promoted_to = std::nullopt, .type = MoveType::EnPassant};
  }

  [[nodiscard]] static constexpr Move castle(Square king_from, Square king_to) {
    return Move{.from = king_from, .to = king_to, .piece = PieceType::King, .captured = std::nullopt, .promoted_to = std::nullopt, .type = MoveType::Castle};
  }

  [[nodiscard]] static constexpr Move promotion(Square from, Square to, PieceType promote_to,
                                                 std::optional<PieceType> captured = std::nullopt) {
    return Move{.from = from, .to = to, .piece = PieceType::Pawn, .captured = captured, .promoted_to = promote_to, .type = MoveType::Promotion};
  }

  Square from{};
  Square to{};
  PieceType piece = PieceType::Pawn;
  std::optional<PieceType> captured = std::nullopt;
  std::optional<PieceType> promoted_to = std::nullopt;
  MoveType type = MoveType::Normal;
};

}  // namespace chess
