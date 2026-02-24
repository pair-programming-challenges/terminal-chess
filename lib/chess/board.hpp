#pragma once

#include <array>
#include <optional>
#include <stdexcept>

#include "chess/types.hpp"

namespace chess {

class Board {
 public:
  [[nodiscard]] std::optional<Piece> piece_at(Square sq) const {
    if (!sq.is_valid()) throw std::runtime_error("Board::piece_at: invalid square");
    return squares_[index(sq)];
  }

  void set_piece(Square sq, Piece piece) {
    if (!sq.is_valid()) throw std::runtime_error("Board::set_piece: invalid square");
    squares_[index(sq)] = piece;
  }

  void clear_square(Square sq) {
    if (!sq.is_valid()) throw std::runtime_error("Board::clear_square: invalid square");
    squares_[index(sq)] = std::nullopt;
  }

  [[nodiscard]] static Board standard() {
    Board board;

    // White back rank (row 0)
    constexpr std::array<PieceType, 8> kBackRank = {PieceType::Rook,   PieceType::Knight, PieceType::Bishop,
                                                    PieceType::Queen,  PieceType::King,   PieceType::Bishop,
                                                    PieceType::Knight, PieceType::Rook};
    for (int8_t col = 0; col < 8; ++col) {
      board.set_piece(Square{.row = 0, .col = col}, Piece{.color = Color::White, .type = kBackRank[col]});
      board.set_piece(Square{.row = 1, .col = col}, Piece{.color = Color::White, .type = PieceType::Pawn});
      board.set_piece(Square{.row = 6, .col = col}, Piece{.color = Color::Black, .type = PieceType::Pawn});
      board.set_piece(Square{.row = 7, .col = col}, Piece{.color = Color::Black, .type = kBackRank[col]});
    }

    return board;
  }

  [[nodiscard]] bool operator==(const Board&) const = default;

 private:
  [[nodiscard]] static constexpr size_t index(Square sq) { return static_cast<size_t>(sq.row * 8 + sq.col); }

  std::array<std::optional<Piece>, 64> squares_{};
};

}  // namespace chess
