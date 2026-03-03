#include <gtest/gtest.h>

#include "chess/game.hpp"

namespace chess {
namespace {

// --- Helpers ---

Square sq(std::string_view s) { return *Square::from_algebraic(s); }

GameState empty_state() {
  GameState state{
      .board = Board{},
      .active_color = Color::White,
      .castling = CastlingRights{
          .white_kingside = false,
          .white_queenside = false,
          .black_kingside = false,
          .black_queenside = false,
      },
      .en_passant_target = std::nullopt,
      .halfmove_clock = 0,
      .fullmove_number = 1,
  };
  return state;
}

// --- Active color ---

TEST(ApplyMove, ActiveColorTogglesWhiteToBlack) {
  auto state = empty_state();
  state.board.set_piece(sq("e2"), Piece{.color = Color::White, .type = PieceType::Pawn});

  apply_move(state, Move::normal(sq("e2"), sq("e3"), PieceType::Pawn));

  EXPECT_EQ(state.active_color, Color::Black);
}

TEST(ApplyMove, ActiveColorTogglesBlackToWhite) {
  auto state = empty_state();
  state.active_color = Color::Black;
  state.board.set_piece(sq("e7"), Piece{.color = Color::Black, .type = PieceType::Pawn});

  apply_move(state, Move::normal(sq("e7"), sq("e6"), PieceType::Pawn));

  EXPECT_EQ(state.active_color, Color::White);
}

// --- Halfmove clock ---

TEST(ApplyMove, HalfmoveClockResetsOnPawnMove) {
  auto state = empty_state();
  state.halfmove_clock = 5;
  state.board.set_piece(sq("e2"), Piece{.color = Color::White, .type = PieceType::Pawn});

  apply_move(state, Move::normal(sq("e2"), sq("e3"), PieceType::Pawn));

  EXPECT_EQ(state.halfmove_clock, 0);
}

TEST(ApplyMove, HalfmoveClockResetsOnCapture) {
  auto state = empty_state();
  state.halfmove_clock = 7;
  state.board.set_piece(sq("d4"), Piece{.color = Color::White, .type = PieceType::Knight});
  state.board.set_piece(sq("e6"), Piece{.color = Color::Black, .type = PieceType::Bishop});

  apply_move(state, Move::capture(sq("d4"), sq("e6"), PieceType::Knight, PieceType::Bishop));

  EXPECT_EQ(state.halfmove_clock, 0);
}

TEST(ApplyMove, HalfmoveClockIncrementsOnQuietNonPawnMove) {
  auto state = empty_state();
  state.halfmove_clock = 3;
  state.board.set_piece(sq("b1"), Piece{.color = Color::White, .type = PieceType::Knight});

  apply_move(state, Move::normal(sq("b1"), sq("c3"), PieceType::Knight));

  EXPECT_EQ(state.halfmove_clock, 4);
}

// --- Fullmove number ---

TEST(ApplyMove, FullmoveNumberIncrementsAfterBlackMoves) {
  auto state = empty_state();
  state.active_color = Color::Black;
  state.fullmove_number = 1;
  state.board.set_piece(sq("e7"), Piece{.color = Color::Black, .type = PieceType::Pawn});

  apply_move(state, Move::normal(sq("e7"), sq("e6"), PieceType::Pawn));

  EXPECT_EQ(state.fullmove_number, 2);
}

TEST(ApplyMove, FullmoveNumberDoesNotIncrementAfterWhiteMoves) {
  auto state = empty_state();
  state.fullmove_number = 1;
  state.board.set_piece(sq("e2"), Piece{.color = Color::White, .type = PieceType::Pawn});

  apply_move(state, Move::normal(sq("e2"), sq("e3"), PieceType::Pawn));

  EXPECT_EQ(state.fullmove_number, 1);
}

// --- En passant target ---

TEST(ApplyMove, EnPassantTargetSetOnWhitePawnDoublePush) {
  auto state = empty_state();
  state.board.set_piece(sq("e2"), Piece{.color = Color::White, .type = PieceType::Pawn});

  apply_move(state, Move::normal(sq("e2"), sq("e4"), PieceType::Pawn));

  ASSERT_TRUE(state.en_passant_target.has_value());
  EXPECT_EQ(*state.en_passant_target, sq("e3"));
}

TEST(ApplyMove, EnPassantTargetSetOnBlackPawnDoublePush) {
  auto state = empty_state();
  state.active_color = Color::Black;
  state.board.set_piece(sq("d7"), Piece{.color = Color::Black, .type = PieceType::Pawn});

  apply_move(state, Move::normal(sq("d7"), sq("d5"), PieceType::Pawn));

  ASSERT_TRUE(state.en_passant_target.has_value());
  EXPECT_EQ(*state.en_passant_target, sq("d6"));
}

TEST(ApplyMove, EnPassantTargetClearedOnNonDoublePush) {
  auto state = empty_state();
  state.en_passant_target = sq("e3");
  state.board.set_piece(sq("b1"), Piece{.color = Color::White, .type = PieceType::Knight});

  apply_move(state, Move::normal(sq("b1"), sq("c3"), PieceType::Knight));

  EXPECT_FALSE(state.en_passant_target.has_value());
}

// --- Castling rights ---

TEST(ApplyMove, CastlingRightsRevokedWhenWhiteKingMoves) {
  auto state = empty_state();
  state.castling.white_kingside = true;
  state.castling.white_queenside = true;
  state.board.set_piece(sq("e1"), Piece{.color = Color::White, .type = PieceType::King});

  apply_move(state, Move::normal(sq("e1"), sq("d1"), PieceType::King));

  EXPECT_FALSE(state.castling.white_kingside);
  EXPECT_FALSE(state.castling.white_queenside);
}

TEST(ApplyMove, CastlingRightsRevokedWhenBlackKingMoves) {
  auto state = empty_state();
  state.active_color = Color::Black;
  state.castling.black_kingside = true;
  state.castling.black_queenside = true;
  state.board.set_piece(sq("e8"), Piece{.color = Color::Black, .type = PieceType::King});

  apply_move(state, Move::normal(sq("e8"), sq("d8"), PieceType::King));

  EXPECT_FALSE(state.castling.black_kingside);
  EXPECT_FALSE(state.castling.black_queenside);
}

TEST(ApplyMove, CastlingRightsRevokedWhenKingsideRookMoves) {
  auto state = empty_state();
  state.castling.white_kingside = true;
  state.castling.white_queenside = true;
  state.board.set_piece(sq("h1"), Piece{.color = Color::White, .type = PieceType::Rook});

  apply_move(state, Move::normal(sq("h1"), sq("h3"), PieceType::Rook));

  EXPECT_FALSE(state.castling.white_kingside);
  EXPECT_TRUE(state.castling.white_queenside);
}

TEST(ApplyMove, CastlingRightsRevokedWhenQueensideRookMoves) {
  auto state = empty_state();
  state.castling.white_kingside = true;
  state.castling.white_queenside = true;
  state.board.set_piece(sq("a1"), Piece{.color = Color::White, .type = PieceType::Rook});

  apply_move(state, Move::normal(sq("a1"), sq("a3"), PieceType::Rook));

  EXPECT_TRUE(state.castling.white_kingside);
  EXPECT_FALSE(state.castling.white_queenside);
}

TEST(ApplyMove, CastlingRightsRevokedWhenRookCapturedOnStartingSquare) {
  auto state = empty_state();
  state.active_color = Color::Black;
  state.castling.white_kingside = true;
  state.board.set_piece(sq("h1"), Piece{.color = Color::White, .type = PieceType::Rook});
  state.board.set_piece(sq("h8"), Piece{.color = Color::Black, .type = PieceType::Rook});

  apply_move(state, Move::capture(sq("h8"), sq("h1"), PieceType::Rook, PieceType::Rook));

  EXPECT_FALSE(state.castling.white_kingside);
}

TEST(ApplyMove, CastlingRightsRevokedOnCastlingMoveItself) {
  auto state = empty_state();
  state.castling.white_kingside = true;
  state.castling.white_queenside = true;
  state.board.set_piece(sq("e1"), Piece{.color = Color::White, .type = PieceType::King});
  state.board.set_piece(sq("h1"), Piece{.color = Color::White, .type = PieceType::Rook});

  apply_move(state, Move::castle(sq("e1"), sq("g1")));

  EXPECT_FALSE(state.castling.white_kingside);
  EXPECT_FALSE(state.castling.white_queenside);
}

// --- Board delegation ---

TEST(ApplyMove, BoardUpdatedCorrectly) {
  auto state = empty_state();
  state.board.set_piece(sq("e2"), Piece{.color = Color::White, .type = PieceType::Pawn});

  apply_move(state, Move::normal(sq("e2"), sq("e4"), PieceType::Pawn));

  EXPECT_FALSE(state.board.piece_at(sq("e2")).has_value());
  ASSERT_TRUE(state.board.piece_at(sq("e4")).has_value());
  EXPECT_EQ(state.board.piece_at(sq("e4"))->type, PieceType::Pawn);
  EXPECT_EQ(state.board.piece_at(sq("e4"))->color, Color::White);
}

}  // namespace
}  // namespace chess
