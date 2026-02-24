#include "chess/move.hpp"

#include <gtest/gtest.h>

namespace chess {
namespace {

// --- Normal move ---

TEST(Move, NormalFactory) {
  auto from = Square{.row = 1, .col = 4};  // e2
  auto to = Square{.row = 3, .col = 4};    // e4
  auto move = Move::normal(from, to, PieceType::Pawn);

  EXPECT_EQ(move.from, from);
  EXPECT_EQ(move.to, to);
  EXPECT_EQ(move.piece, PieceType::Pawn);
  EXPECT_FALSE(move.captured.has_value());
  EXPECT_FALSE(move.promoted_to.has_value());
  EXPECT_EQ(move.type, MoveType::Normal);
}

// --- Capture ---

TEST(Move, CaptureFactory) {
  auto from = Square{.row = 3, .col = 4};  // e4
  auto to = Square{.row = 4, .col = 3};    // d5
  auto move = Move::capture(from, to, PieceType::Pawn, PieceType::Pawn);

  EXPECT_EQ(move.from, from);
  EXPECT_EQ(move.to, to);
  EXPECT_EQ(move.piece, PieceType::Pawn);
  ASSERT_TRUE(move.captured.has_value());
  EXPECT_EQ(*move.captured, PieceType::Pawn);
  EXPECT_FALSE(move.promoted_to.has_value());
  EXPECT_EQ(move.type, MoveType::Normal);
}

TEST(Move, CaptureWithDifferentPieces) {
  auto from = Square{.row = 4, .col = 2};  // c5
  auto to = Square{.row = 2, .col = 3};    // d3
  auto move = Move::capture(from, to, PieceType::Knight, PieceType::Bishop);

  EXPECT_EQ(move.piece, PieceType::Knight);
  ASSERT_TRUE(move.captured.has_value());
  EXPECT_EQ(*move.captured, PieceType::Bishop);
}

// --- En passant ---

TEST(Move, EnPassantFactory) {
  auto from = Square{.row = 4, .col = 4};  // e5
  auto to = Square{.row = 5, .col = 3};    // d6
  auto move = Move::en_passant(from, to);

  EXPECT_EQ(move.from, from);
  EXPECT_EQ(move.to, to);
  EXPECT_EQ(move.piece, PieceType::Pawn);
  ASSERT_TRUE(move.captured.has_value());
  EXPECT_EQ(*move.captured, PieceType::Pawn);
  EXPECT_FALSE(move.promoted_to.has_value());
  EXPECT_EQ(move.type, MoveType::EnPassant);
}

// --- Castle ---

TEST(Move, CastleFactory) {
  auto king_from = Square{.row = 0, .col = 4};  // e1
  auto king_to = Square{.row = 0, .col = 6};    // g1
  auto move = Move::castle(king_from, king_to);

  EXPECT_EQ(move.from, king_from);
  EXPECT_EQ(move.to, king_to);
  EXPECT_EQ(move.piece, PieceType::King);
  EXPECT_FALSE(move.captured.has_value());
  EXPECT_FALSE(move.promoted_to.has_value());
  EXPECT_EQ(move.type, MoveType::Castle);
}

TEST(Move, CastleQueenside) {
  auto king_from = Square{.row = 0, .col = 4};  // e1
  auto king_to = Square{.row = 0, .col = 2};    // c1
  auto move = Move::castle(king_from, king_to);

  EXPECT_EQ(move.to, king_to);
  EXPECT_EQ(move.type, MoveType::Castle);
}

// --- Promotion ---

TEST(Move, PromotionWithoutCapture) {
  auto from = Square{.row = 6, .col = 0};  // a7
  auto to = Square{.row = 7, .col = 0};    // a8
  auto move = Move::promotion(from, to, PieceType::Queen);

  EXPECT_EQ(move.from, from);
  EXPECT_EQ(move.to, to);
  EXPECT_EQ(move.piece, PieceType::Pawn);
  EXPECT_FALSE(move.captured.has_value());
  ASSERT_TRUE(move.promoted_to.has_value());
  EXPECT_EQ(*move.promoted_to, PieceType::Queen);
  EXPECT_EQ(move.type, MoveType::Promotion);
}

TEST(Move, PromotionWithCapture) {
  auto from = Square{.row = 6, .col = 0};  // a7
  auto to = Square{.row = 7, .col = 1};    // b8
  auto move = Move::promotion(from, to, PieceType::Knight, PieceType::Rook);

  EXPECT_EQ(move.piece, PieceType::Pawn);
  ASSERT_TRUE(move.captured.has_value());
  EXPECT_EQ(*move.captured, PieceType::Rook);
  ASSERT_TRUE(move.promoted_to.has_value());
  EXPECT_EQ(*move.promoted_to, PieceType::Knight);
  EXPECT_EQ(move.type, MoveType::Promotion);
}

TEST(Move, PromotionAllPieceTypes) {
  auto from = Square{.row = 6, .col = 3};
  auto to = Square{.row = 7, .col = 3};
  for (auto promo : {PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight}) {
    auto move = Move::promotion(from, to, promo);
    ASSERT_TRUE(move.promoted_to.has_value());
    EXPECT_EQ(*move.promoted_to, promo);
  }
}

// --- Equality ---

TEST(Move, EqualMovesAreEqual) {
  auto a = Move::normal(Square{.row = 1, .col = 4}, Square{.row = 3, .col = 4}, PieceType::Pawn);
  auto b = Move::normal(Square{.row = 1, .col = 4}, Square{.row = 3, .col = 4}, PieceType::Pawn);
  EXPECT_EQ(a, b);
}

TEST(Move, DifferentMovesAreNotEqual) {
  auto a = Move::normal(Square{.row = 1, .col = 4}, Square{.row = 3, .col = 4}, PieceType::Pawn);
  auto b = Move::normal(Square{.row = 1, .col = 4}, Square{.row = 2, .col = 4}, PieceType::Pawn);
  EXPECT_NE(a, b);
}

TEST(Move, CaptureVsNormalNotEqual) {
  auto from = Square{.row = 3, .col = 4};
  auto to = Square{.row = 4, .col = 3};
  auto a = Move::normal(from, to, PieceType::Pawn);
  auto b = Move::capture(from, to, PieceType::Pawn, PieceType::Pawn);
  EXPECT_NE(a, b);
}

}  // namespace
}  // namespace chess
