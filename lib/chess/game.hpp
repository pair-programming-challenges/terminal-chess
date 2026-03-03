#pragma once

#include "chess/move.hpp"
#include "chess/move_generator.hpp"

namespace chess {

// Rook starting squares for castling-rights bookkeeping.
inline constexpr Square kWhiteKingsideRook{.row = 0, .col = 7};
inline constexpr Square kWhiteQueensideRook{.row = 0, .col = 0};
inline constexpr Square kBlackKingsideRook{.row = 7, .col = 7};
inline constexpr Square kBlackQueensideRook{.row = 7, .col = 0};

inline void revoke_castling_for_square(CastlingRights& castling, Square sq) {
  if (sq == kWhiteKingsideRook) castling.white_kingside = false;
  if (sq == kWhiteQueensideRook) castling.white_queenside = false;
  if (sq == kBlackKingsideRook) castling.black_kingside = false;
  if (sq == kBlackQueensideRook) castling.black_queenside = false;
}

inline void apply_move(GameState& state, const Move& move) {
  // 1. Board — delegate piece movement
  MoveGenerator::apply_move_to_board(state.board, move);

  // 2. Castling rights
  // King moves → revoke both sides for that color
  if (move.piece == PieceType::King) {
    if (state.active_color == Color::White) {
      state.castling.white_kingside = false;
      state.castling.white_queenside = false;
    } else {
      state.castling.black_kingside = false;
      state.castling.black_queenside = false;
    }
  }
  // Rook moves from starting square → revoke that side
  revoke_castling_for_square(state.castling, move.from);
  // Piece captured on a rook starting square → revoke that side
  if (move.captured) {
    revoke_castling_for_square(state.castling, move.to);
  }

  // 3. En passant target
  if (move.piece == PieceType::Pawn) {
    auto row_diff = move.to.row - move.from.row;
    if (row_diff == 2 || row_diff == -2) {
      state.en_passant_target =
          Square{.row = static_cast<int8_t>((move.from.row + move.to.row) / 2), .col = move.from.col};
    } else {
      state.en_passant_target = std::nullopt;
    }
  } else {
    state.en_passant_target = std::nullopt;
  }

  // 4. Halfmove clock
  if (move.piece == PieceType::Pawn || move.captured) {
    state.halfmove_clock = 0;
  } else {
    ++state.halfmove_clock;
  }

  // 5. Fullmove number — increment after Black moves
  if (state.active_color == Color::Black) {
    ++state.fullmove_number;
  }

  // 6. Active color — toggle last
  state.active_color = opposite(state.active_color);
}

}  // namespace chess
