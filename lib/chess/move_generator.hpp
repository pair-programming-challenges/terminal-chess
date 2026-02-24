#pragma once

#include <stdexcept>
#include <vector>

#include "chess/board.hpp"
#include "chess/game_state.hpp"
#include "chess/move.hpp"
#include "chess/types.hpp"

namespace chess {

// --- Direction type for move offsets ---

struct Direction {
  int8_t row_delta = 0;
  int8_t col_delta = 0;
};

// --- Direction constants ---

inline constexpr Direction kBishopDirs[] = {
    {.row_delta = 1, .col_delta = 1},
    {.row_delta = 1, .col_delta = -1},
    {.row_delta = -1, .col_delta = 1},
    {.row_delta = -1, .col_delta = -1},
};

inline constexpr Direction kRookDirs[] = {
    {.row_delta = 1, .col_delta = 0},
    {.row_delta = -1, .col_delta = 0},
    {.row_delta = 0, .col_delta = 1},
    {.row_delta = 0, .col_delta = -1},
};

inline constexpr Direction kKnightOffsets[] = {
    {.row_delta = 2, .col_delta = 1},  {.row_delta = 2, .col_delta = -1},
    {.row_delta = -2, .col_delta = 1}, {.row_delta = -2, .col_delta = -1},
    {.row_delta = 1, .col_delta = 2},  {.row_delta = 1, .col_delta = -2},
    {.row_delta = -1, .col_delta = 2}, {.row_delta = -1, .col_delta = -2},
};

inline constexpr Direction kKingDirs[] = {
    {.row_delta = 1, .col_delta = 0},  {.row_delta = -1, .col_delta = 0},
    {.row_delta = 0, .col_delta = 1},  {.row_delta = 0, .col_delta = -1},
    {.row_delta = 1, .col_delta = 1},  {.row_delta = 1, .col_delta = -1},
    {.row_delta = -1, .col_delta = 1}, {.row_delta = -1, .col_delta = -1},
};

// --- Helpers ---

[[nodiscard]] inline constexpr Square offset_square(Square base, int8_t dr, int8_t dc) {
  return Square{.row = static_cast<int8_t>(base.row + dr), .col = static_cast<int8_t>(base.col + dc)};
}

// --- Public API: attack detection and king finding ---

[[nodiscard]] inline bool is_square_attacked(const Board& board, Square target, Color attacker) {
  // Knight attacks
  for (auto [dr, dc] : kKnightOffsets) {
    auto sq = offset_square(target, dr, dc);
    if (sq.is_valid()) {
      auto piece = board.piece_at(sq);
      if (piece && piece->color == attacker && piece->type == PieceType::Knight) return true;
    }
  }

  // Diagonal attacks (bishop/queen)
  for (auto [dr, dc] : kBishopDirs) {
    auto row = target.row + dr;
    auto col = target.col + dc;
    while (row >= 0 && row <= 7 && col >= 0 && col <= 7) {
      auto piece = board.piece_at(Square{.row = static_cast<int8_t>(row), .col = static_cast<int8_t>(col)});
      if (piece) {
        if (piece->color == attacker && (piece->type == PieceType::Bishop || piece->type == PieceType::Queen))
          return true;
        break;
      }
      row += dr;
      col += dc;
    }
  }

  // Orthogonal attacks (rook/queen)
  for (auto [dr, dc] : kRookDirs) {
    auto row = target.row + dr;
    auto col = target.col + dc;
    while (row >= 0 && row <= 7 && col >= 0 && col <= 7) {
      auto piece = board.piece_at(Square{.row = static_cast<int8_t>(row), .col = static_cast<int8_t>(col)});
      if (piece) {
        if (piece->color == attacker && (piece->type == PieceType::Rook || piece->type == PieceType::Queen))
          return true;
        break;
      }
      row += dr;
      col += dc;
    }
  }

  // King attacks
  for (auto [dr, dc] : kKingDirs) {
    auto sq = offset_square(target, dr, dc);
    if (sq.is_valid()) {
      auto piece = board.piece_at(sq);
      if (piece && piece->color == attacker && piece->type == PieceType::King) return true;
    }
  }

  // Pawn attacks (check squares from which an attacker pawn could reach target)
  int8_t pawn_dir = (attacker == Color::White) ? 1 : -1;
  for (int dc : {-1, 1}) {
    auto sq = offset_square(target, static_cast<int8_t>(-pawn_dir), static_cast<int8_t>(dc));
    if (sq.is_valid()) {
      auto piece = board.piece_at(sq);
      if (piece && piece->color == attacker && piece->type == PieceType::Pawn) return true;
    }
  }

  return false;
}

[[nodiscard]] inline Square find_king(const Board& board, Color color) {
  for (int8_t row = 0; row < 8; ++row) {
    for (int8_t col = 0; col < 8; ++col) {
      Square sq{.row = row, .col = col};
      auto piece = board.piece_at(sq);
      if (piece && piece->color == color && piece->type == PieceType::King) {
        return sq;
      }
    }
  }
  throw std::runtime_error("King not found on board");
}

// --- Internal implementation details ---

namespace detail {

inline void apply_move_to_board(Board& board, const Move& move) {
  auto moving_piece = board.piece_at(move.from);
  board.clear_square(move.from);

  switch (move.type) {
    case MoveType::EnPassant: {
      board.clear_square(Square{.row = move.from.row, .col = move.to.col});
      board.set_piece(move.to, *moving_piece);
      break;
    }
    case MoveType::Castle: {
      board.set_piece(move.to, *moving_piece);
      int8_t row = move.from.row;
      if (move.to.col == 6) {  // Kingside
        auto rook = board.piece_at(Square{.row = row, .col = 7});
        board.clear_square(Square{.row = row, .col = 7});
        board.set_piece(Square{.row = row, .col = 5}, *rook);
      } else {  // Queenside (col == 2)
        auto rook = board.piece_at(Square{.row = row, .col = 0});
        board.clear_square(Square{.row = row, .col = 0});
        board.set_piece(Square{.row = row, .col = 3}, *rook);
      }
      break;
    }
    case MoveType::Promotion: {
      board.set_piece(move.to, Piece{.color = moving_piece->color, .type = *move.promoted_to});
      break;
    }
    case MoveType::Normal: {
      board.set_piece(move.to, *moving_piece);
      break;
    }
  }
}

inline void generate_sliding_moves(const Board& board, Square from, Color color, PieceType piece_type,
                                   const Direction* dirs, size_t num_dirs, std::vector<Move>& moves) {
  for (size_t i = 0; i < num_dirs; ++i) {
    auto [dr, dc] = dirs[i];
    auto row = from.row + dr;
    auto col = from.col + dc;
    while (row >= 0 && row <= 7 && col >= 0 && col <= 7) {
      Square to{.row = static_cast<int8_t>(row), .col = static_cast<int8_t>(col)};
      auto target = board.piece_at(to);
      if (target) {
        if (target->color != color) {
          moves.push_back(Move::capture(from, to, piece_type, target->type));
        }
        break;
      }
      moves.push_back(Move::normal(from, to, piece_type));
      row += dr;
      col += dc;
    }
  }
}

inline void generate_pawn_moves(const Board& board, Square from, Color color,
                                std::optional<Square> en_passant_target, std::vector<Move>& moves) {
  int8_t direction = (color == Color::White) ? 1 : -1;
  int8_t start_row = (color == Color::White) ? 1 : 6;
  int8_t promo_row = (color == Color::White) ? 7 : 0;

  auto add_pawn_move = [&](Square to, std::optional<PieceType> captured) {
    if (to.row == promo_row) {
      for (auto promo : {PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight}) {
        moves.push_back(Move::promotion(from, to, promo, captured));
      }
    } else if (captured) {
      moves.push_back(Move::capture(from, to, PieceType::Pawn, *captured));
    } else {
      moves.push_back(Move::normal(from, to, PieceType::Pawn));
    }
  };

  // Single push
  Square one_ahead = offset_square(from, direction, 0);
  if (one_ahead.is_valid() && !board.piece_at(one_ahead)) {
    add_pawn_move(one_ahead, std::nullopt);

    // Double push (only if single push was possible)
    if (from.row == start_row) {
      Square two_ahead = offset_square(from, static_cast<int8_t>(2 * direction), 0);
      if (!board.piece_at(two_ahead)) {
        moves.push_back(Move::normal(from, two_ahead, PieceType::Pawn));
      }
    }
  }

  // Captures (including en passant)
  for (int dc : {-1, 1}) {
    Square diag = offset_square(from, direction, static_cast<int8_t>(dc));
    if (!diag.is_valid()) continue;

    auto target = board.piece_at(diag);
    if (target && target->color != color) {
      add_pawn_move(diag, target->type);
    }

    if (en_passant_target && diag == *en_passant_target) {
      moves.push_back(Move::en_passant(from, diag));
    }
  }
}

inline void generate_knight_moves(const Board& board, Square from, Color color, std::vector<Move>& moves) {
  for (auto [dr, dc] : kKnightOffsets) {
    auto to = offset_square(from, dr, dc);
    if (!to.is_valid()) continue;

    auto target = board.piece_at(to);
    if (target) {
      if (target->color != color) {
        moves.push_back(Move::capture(from, to, PieceType::Knight, target->type));
      }
    } else {
      moves.push_back(Move::normal(from, to, PieceType::Knight));
    }
  }
}

inline void generate_king_moves(const Board& board, Square from, Color color, const CastlingRights& castling,
                                std::vector<Move>& moves) {
  // Normal king moves
  for (auto [dr, dc] : kKingDirs) {
    auto to = offset_square(from, dr, dc);
    if (!to.is_valid()) continue;

    auto target = board.piece_at(to);
    if (target) {
      if (target->color != color) {
        moves.push_back(Move::capture(from, to, PieceType::King, target->type));
      }
    } else {
      moves.push_back(Move::normal(from, to, PieceType::King));
    }
  }

  // Castling
  Color opponent = opposite(color);
  int8_t row = (color == Color::White) ? 0 : 7;

  // Only attempt castling if king is on its starting square
  if (from.row != row || from.col != 4) return;

  // Kingside
  if (castling.can_kingside(color)) {
    Square f_sq{.row = row, .col = 5};
    Square g_sq{.row = row, .col = 6};
    if (!board.piece_at(f_sq) && !board.piece_at(g_sq)) {
      if (!is_square_attacked(board, from, opponent) && !is_square_attacked(board, f_sq, opponent) &&
          !is_square_attacked(board, g_sq, opponent)) {
        auto rook = board.piece_at(Square{.row = row, .col = 7});
        if (rook && rook->color == color && rook->type == PieceType::Rook) {
          moves.push_back(Move::castle(from, g_sq));
        }
      }
    }
  }

  // Queenside
  if (castling.can_queenside(color)) {
    Square d_sq{.row = row, .col = 3};
    Square c_sq{.row = row, .col = 2};
    Square b_sq{.row = row, .col = 1};
    if (!board.piece_at(d_sq) && !board.piece_at(c_sq) && !board.piece_at(b_sq)) {
      // b-file does not need to be safe, only the king's path (e->d->c)
      if (!is_square_attacked(board, from, opponent) && !is_square_attacked(board, d_sq, opponent) &&
          !is_square_attacked(board, c_sq, opponent)) {
        auto rook = board.piece_at(Square{.row = row, .col = 0});
        if (rook && rook->color == color && rook->type == PieceType::Rook) {
          moves.push_back(Move::castle(from, c_sq));
        }
      }
    }
  }
}

inline std::vector<Move> generate_pseudo_legal_moves(const GameState& state) {
  std::vector<Move> moves;
  moves.reserve(64);

  for (int8_t row = 0; row < 8; ++row) {
    for (int8_t col = 0; col < 8; ++col) {
      Square from{.row = row, .col = col};
      auto piece = state.board.piece_at(from);
      if (!piece || piece->color != state.active_color) continue;

      switch (piece->type) {
        case PieceType::Pawn:
          generate_pawn_moves(state.board, from, state.active_color, state.en_passant_target, moves);
          break;
        case PieceType::Knight:
          generate_knight_moves(state.board, from, state.active_color, moves);
          break;
        case PieceType::Bishop:
          generate_sliding_moves(state.board, from, state.active_color, PieceType::Bishop, kBishopDirs,
                                 std::size(kBishopDirs), moves);
          break;
        case PieceType::Rook:
          generate_sliding_moves(state.board, from, state.active_color, PieceType::Rook, kRookDirs,
                                 std::size(kRookDirs), moves);
          break;
        case PieceType::Queen:
          generate_sliding_moves(state.board, from, state.active_color, PieceType::Queen, kBishopDirs,
                                 std::size(kBishopDirs), moves);
          generate_sliding_moves(state.board, from, state.active_color, PieceType::Queen, kRookDirs,
                                 std::size(kRookDirs), moves);
          break;
        case PieceType::King:
          generate_king_moves(state.board, from, state.active_color, state.castling, moves);
          break;
      }
    }
  }

  return moves;
}

}  // namespace detail

// --- Public API: legal move generation ---

[[nodiscard]] inline std::vector<Move> generate_legal_moves(const GameState& state) {
  auto pseudo_legal = detail::generate_pseudo_legal_moves(state);

  std::vector<Move> legal;
  legal.reserve(pseudo_legal.size());

  Color opponent = opposite(state.active_color);

  for (const auto& move : pseudo_legal) {
    Board board_copy = state.board;
    detail::apply_move_to_board(board_copy, move);
    Square king_sq = find_king(board_copy, state.active_color);
    if (!is_square_attacked(board_copy, king_sq, opponent)) {
      legal.push_back(move);
    }
  }

  return legal;
}

}  // namespace chess
