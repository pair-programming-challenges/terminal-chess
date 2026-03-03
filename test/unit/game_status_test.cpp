#include <gtest/gtest.h>

#include "chess/game_status.hpp"

namespace chess {
namespace {

// --- Helpers ---

Square sq(std::string_view s) { return *Square::from_algebraic(s); }

constexpr Piece WK{.color = Color::White, .type = PieceType::King};
constexpr Piece WP{.color = Color::White, .type = PieceType::Pawn};
constexpr Piece WR{.color = Color::White, .type = PieceType::Rook};
constexpr Piece WQ{.color = Color::White, .type = PieceType::Queen};
constexpr Piece BK{.color = Color::Black, .type = PieceType::King};
constexpr Piece BR{.color = Color::Black, .type = PieceType::Rook};
constexpr Piece BN{.color = Color::Black, .type = PieceType::Knight};

constexpr CastlingRights kNoCastling{
    .white_kingside = false, .white_queenside = false, .black_kingside = false, .black_queenside = false};

GameState make_state(Color active = Color::White) {
  GameState state;
  state.active_color = active;
  state.castling = kNoCastling;
  state.en_passant_target = std::nullopt;
  state.halfmove_clock = 0;
  state.fullmove_number = 1;
  return state;
}

// ============================================================
// is_in_check
// ============================================================

TEST(IsInCheck, NotInCheck) {
  auto state = make_state();
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("e8"), BK);

  EXPECT_FALSE(is_in_check(state));
}

TEST(IsInCheck, InCheckByRook) {
  auto state = make_state();
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("e8"), BR);  // Attacks e1 along e-file
  state.board.set_piece(sq("a8"), BK);

  EXPECT_TRUE(is_in_check(state));
}

TEST(IsInCheck, InCheckByKnight) {
  auto state = make_state();
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("f3"), BN);  // Knight attacks e1
  state.board.set_piece(sq("a8"), BK);

  EXPECT_TRUE(is_in_check(state));
}

TEST(IsInCheck, BlockedCheckIsNotCheck) {
  auto state = make_state();
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("e4"), WR);  // Blocks the file
  state.board.set_piece(sq("e8"), BR);
  state.board.set_piece(sq("a8"), BK);

  EXPECT_FALSE(is_in_check(state));
}

// ============================================================
// is_fifty_move_draw
// ============================================================

TEST(IsFiftyMoveDraw, DrawAt100Halfmoves) {
  auto state = make_state();
  state.halfmove_clock = 100;

  EXPECT_TRUE(is_fifty_move_draw(state));
}

TEST(IsFiftyMoveDraw, NotDrawAt99Halfmoves) {
  auto state = make_state();
  state.halfmove_clock = 99;

  EXPECT_FALSE(is_fifty_move_draw(state));
}

TEST(IsFiftyMoveDraw, DrawAbove100) {
  auto state = make_state();
  state.halfmove_clock = 150;

  EXPECT_TRUE(is_fifty_move_draw(state));
}

// ============================================================
// compute_status
// ============================================================

TEST(ComputeStatus, OngoingGameReturnsNoResult) {
  auto state = make_state();
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("e8"), BK);

  MoveGenerator gen(state);
  auto legal_moves = gen.generate_all_legal_moves();
  auto status = compute_status(state, legal_moves);

  EXPECT_FALSE(status.in_check);
  EXPECT_FALSE(status.result.has_value());
}

TEST(ComputeStatus, CheckmateDetected) {
  // Back-rank mate: White Kg1, pawns f2/g2/h2. Black Rc1 gives check.
  // King can't escape: f1/h1 still on rank 1, f2/g2/h2 blocked by own pawns.
  auto state = make_state();
  state.board.set_piece(sq("g1"), WK);
  state.board.set_piece(sq("f2"), WP);
  state.board.set_piece(sq("g2"), WP);
  state.board.set_piece(sq("h2"), WP);
  state.board.set_piece(sq("c1"), BR);  // Checks along rank 1
  state.board.set_piece(sq("a8"), BK);

  MoveGenerator gen(state);
  auto legal_moves = gen.generate_all_legal_moves();
  auto status = compute_status(state, legal_moves);

  EXPECT_TRUE(status.in_check);
  ASSERT_TRUE(status.result.has_value());
  EXPECT_EQ(status.result->outcome, Outcome::Checkmate);
  ASSERT_TRUE(status.result->winner.has_value());
  EXPECT_EQ(*status.result->winner, Color::Black);
}

TEST(ComputeStatus, StalemateDetected) {
  // Classic stalemate: Black Kh8, White Kf7 + Qg6. Black to move.
  // h8 not in check. Escapes: g8(Kf7+Qg6), g7(Kf7), h7(Qg6 diag). All blocked.
  auto state = make_state(Color::Black);
  state.board.set_piece(sq("h8"), BK);
  state.board.set_piece(sq("f7"), WK);
  state.board.set_piece(sq("g6"), WQ);

  MoveGenerator gen(state);
  auto legal_moves = gen.generate_all_legal_moves();
  auto status = compute_status(state, legal_moves);

  EXPECT_FALSE(status.in_check);
  ASSERT_TRUE(status.result.has_value());
  EXPECT_EQ(status.result->outcome, Outcome::Stalemate);
  EXPECT_FALSE(status.result->winner.has_value());
}

TEST(ComputeStatus, FiftyMoveDrawDetected) {
  auto state = make_state();
  state.halfmove_clock = 100;
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("e8"), BK);

  MoveGenerator gen(state);
  auto legal_moves = gen.generate_all_legal_moves();
  auto status = compute_status(state, legal_moves);

  ASSERT_TRUE(status.result.has_value());
  EXPECT_EQ(status.result->outcome, Outcome::Draw);
  EXPECT_FALSE(status.result->winner.has_value());
}

TEST(ComputeStatus, InCheckButNotCheckmate) {
  // White king on e1, Black rook on e8 — in check but can escape
  auto state = make_state();
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("e8"), BR);
  state.board.set_piece(sq("a8"), BK);

  MoveGenerator gen(state);
  auto legal_moves = gen.generate_all_legal_moves();
  auto status = compute_status(state, legal_moves);

  EXPECT_TRUE(status.in_check);
  EXPECT_FALSE(status.result.has_value());
}

}  // namespace
}  // namespace chess
