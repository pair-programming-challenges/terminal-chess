#include "chess/types.hpp"

#include <gtest/gtest.h>

namespace chess {
namespace {

// --- Color ---

TEST(Color, OppositeWhiteIsBlack) { EXPECT_EQ(opposite(Color::White), Color::Black); }

TEST(Color, OppositeBlackIsWhite) { EXPECT_EQ(opposite(Color::Black), Color::White); }

// --- Square ---

TEST(Square, DefaultIsValid) {
  Square s{};
  EXPECT_TRUE(s.is_valid());
  EXPECT_EQ(s.row, 0);
  EXPECT_EQ(s.col, 0);
}

TEST(Square, ValidSquares) {
  EXPECT_TRUE((Square{.row = 0, .col = 0}).is_valid());
  EXPECT_TRUE((Square{.row = 7, .col = 7}).is_valid());
  EXPECT_TRUE((Square{.row = 3, .col = 4}).is_valid());
}

TEST(Square, InvalidSquares) {
  EXPECT_FALSE((Square{.row = -1, .col = 0}).is_valid());
  EXPECT_FALSE((Square{.row = 0, .col = -1}).is_valid());
  EXPECT_FALSE((Square{.row = 8, .col = 0}).is_valid());
  EXPECT_FALSE((Square{.row = 0, .col = 8}).is_valid());
}

TEST(Square, Equality) {
  EXPECT_EQ((Square{.row = 3, .col = 4}), (Square{.row = 3, .col = 4}));
  EXPECT_NE((Square{.row = 3, .col = 4}), (Square{.row = 4, .col = 3}));
}

TEST(Square, FromAlgebraicValid) {
  auto sq = Square::from_algebraic("e4");
  ASSERT_TRUE(sq.has_value());
  EXPECT_EQ(sq->row, 3);
  EXPECT_EQ(sq->col, 4);
}

TEST(Square, FromAlgebraicCorners) {
  auto a1 = Square::from_algebraic("a1");
  ASSERT_TRUE(a1.has_value());
  EXPECT_EQ(a1->row, 0);
  EXPECT_EQ(a1->col, 0);

  auto h8 = Square::from_algebraic("h8");
  ASSERT_TRUE(h8.has_value());
  EXPECT_EQ(h8->row, 7);
  EXPECT_EQ(h8->col, 7);
}

TEST(Square, FromAlgebraicInvalid) {
  EXPECT_FALSE(Square::from_algebraic("").has_value());
  EXPECT_FALSE(Square::from_algebraic("e").has_value());
  EXPECT_FALSE(Square::from_algebraic("e44").has_value());
  EXPECT_FALSE(Square::from_algebraic("i1").has_value());
  EXPECT_FALSE(Square::from_algebraic("a0").has_value());
  EXPECT_FALSE(Square::from_algebraic("a9").has_value());
}

TEST(Square, ToAlgebraic) {
  EXPECT_EQ((Square{.row = 3, .col = 4}).to_algebraic(), "e4");
  EXPECT_EQ((Square{.row = 0, .col = 0}).to_algebraic(), "a1");
  EXPECT_EQ((Square{.row = 7, .col = 7}).to_algebraic(), "h8");
}

TEST(Square, AlgebraicRoundTrip) {
  for (int8_t row = 0; row < 8; ++row) {
    for (int8_t col = 0; col < 8; ++col) {
      Square original{.row = row, .col = col};
      auto str = original.to_algebraic();
      auto parsed = Square::from_algebraic(str);
      ASSERT_TRUE(parsed.has_value()) << "Failed to parse: " << str;
      EXPECT_EQ(*parsed, original) << "Round-trip failed for: " << str;
    }
  }
}

// --- Piece ---

TEST(Piece, Equality) {
  Piece p1{.color = Color::White, .type = PieceType::King};
  Piece p2{.color = Color::White, .type = PieceType::King};
  Piece p3{.color = Color::Black, .type = PieceType::King};
  EXPECT_EQ(p1, p2);
  EXPECT_NE(p1, p3);
}

TEST(Piece, DisplayCharWhite) {
  EXPECT_EQ((Piece{.color = Color::White, .type = PieceType::King}).display_char(), U'\u2654');    // ♔
  EXPECT_EQ((Piece{.color = Color::White, .type = PieceType::Queen}).display_char(), U'\u2655');   // ♕
  EXPECT_EQ((Piece{.color = Color::White, .type = PieceType::Rook}).display_char(), U'\u2656');    // ♖
  EXPECT_EQ((Piece{.color = Color::White, .type = PieceType::Bishop}).display_char(), U'\u2657');  // ♗
  EXPECT_EQ((Piece{.color = Color::White, .type = PieceType::Knight}).display_char(), U'\u2658');  // ♘
  EXPECT_EQ((Piece{.color = Color::White, .type = PieceType::Pawn}).display_char(), U'\u2659');    // ♙
}

TEST(Piece, DisplayCharBlack) {
  EXPECT_EQ((Piece{.color = Color::Black, .type = PieceType::King}).display_char(), U'\u265A');    // ♚
  EXPECT_EQ((Piece{.color = Color::Black, .type = PieceType::Queen}).display_char(), U'\u265B');   // ♛
  EXPECT_EQ((Piece{.color = Color::Black, .type = PieceType::Rook}).display_char(), U'\u265C');    // ♜
  EXPECT_EQ((Piece{.color = Color::Black, .type = PieceType::Bishop}).display_char(), U'\u265D');  // ♝
  EXPECT_EQ((Piece{.color = Color::Black, .type = PieceType::Knight}).display_char(), U'\u265E');  // ♞
  EXPECT_EQ((Piece{.color = Color::Black, .type = PieceType::Pawn}).display_char(), U'\u265F');    // ♟
}

TEST(Piece, DisplayUtf8WhiteKing) {
  Piece p{.color = Color::White, .type = PieceType::King};
  EXPECT_EQ(p.display_utf8(), "♔");
}

TEST(Piece, DisplayUtf8BlackKnight) {
  Piece p{.color = Color::Black, .type = PieceType::Knight};
  EXPECT_EQ(p.display_utf8(), "♞");
}

TEST(Piece, DisplayUtf8AllPieces) {
  // Verify all 12 pieces produce valid 3-byte UTF-8 strings
  for (auto color : {Color::White, Color::Black}) {
    for (auto type :
         {PieceType::King, PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight, PieceType::Pawn}) {
      Piece p{.color = color, .type = type};
      auto utf8 = p.display_utf8();
      EXPECT_EQ(utf8.size(), 3u) << "Expected 3-byte UTF-8 for " << static_cast<int>(color) << "/"
                                 << static_cast<int>(type);
    }
  }
}

}  // namespace
}  // namespace chess
