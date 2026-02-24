#include "chess/board.hpp"

#include <gtest/gtest.h>

namespace chess {
namespace {

// --- Empty Board ---

TEST(Board, DefaultBoardIsEmpty) {
  Board board;
  for (int8_t row = 0; row < 8; ++row) {
    for (int8_t col = 0; col < 8; ++col) {
      EXPECT_FALSE(board.piece_at(Square{.row = row, .col = col}).has_value())
          << "Expected empty at row=" << static_cast<int>(row) << " col=" << static_cast<int>(col);
    }
  }
}

// --- Set / Clear ---

TEST(Board, SetAndGetPiece) {
  Board board;
  auto sq = Square{.row = 3, .col = 4};
  auto piece = Piece{.color = Color::White, .type = PieceType::Queen};
  board.set_piece(sq, piece);

  auto result = board.piece_at(sq);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, piece);
}

TEST(Board, ClearSquare) {
  Board board;
  auto sq = Square{.row = 0, .col = 0};
  board.set_piece(sq, Piece{.color = Color::White, .type = PieceType::Rook});
  board.clear_square(sq);

  EXPECT_FALSE(board.piece_at(sq).has_value());
}

TEST(Board, SetPieceOverwrites) {
  Board board;
  auto sq = Square{.row = 4, .col = 4};
  board.set_piece(sq, Piece{.color = Color::White, .type = PieceType::Pawn});
  board.set_piece(sq, Piece{.color = Color::Black, .type = PieceType::Knight});

  auto result = board.piece_at(sq);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->color, Color::Black);
  EXPECT_EQ(result->type, PieceType::Knight);
}

// --- Invalid square ---

TEST(Board, PieceAtInvalidSquareThrows) {
  Board board;
  EXPECT_THROW((void)board.piece_at(Square{.row = -1, .col = 0}), std::runtime_error);
  EXPECT_THROW((void)board.piece_at(Square{.row = 0, .col = 8}), std::runtime_error);
}

TEST(Board, SetPieceInvalidSquareThrows) {
  Board board;
  EXPECT_THROW(board.set_piece(Square{.row = 8, .col = 0}, Piece{.color = Color::White, .type = PieceType::Pawn}),
               std::runtime_error);
}

TEST(Board, ClearSquareInvalidSquareThrows) {
  Board board;
  EXPECT_THROW(board.clear_square(Square{.row = 0, .col = -1}), std::runtime_error);
}

// --- Standard Position ---

class BoardStandardTest : public ::testing::Test {
 protected:
  void SetUp() override { board_ = Board::standard(); }

  void expect_piece(std::string_view algebraic, Color color, PieceType type) {
    auto sq = Square::from_algebraic(algebraic);
    ASSERT_TRUE(sq.has_value()) << "Bad square: " << algebraic;
    auto piece = board_.piece_at(*sq);
    ASSERT_TRUE(piece.has_value()) << "No piece at " << algebraic;
    EXPECT_EQ(piece->color, color) << "Wrong color at " << algebraic;
    EXPECT_EQ(piece->type, type) << "Wrong type at " << algebraic;
  }

  void expect_empty(std::string_view algebraic) {
    auto sq = Square::from_algebraic(algebraic);
    ASSERT_TRUE(sq.has_value()) << "Bad square: " << algebraic;
    EXPECT_FALSE(board_.piece_at(*sq).has_value()) << "Expected empty at " << algebraic;
  }

  Board board_;
};

TEST_F(BoardStandardTest, WhiteBackRank) {
  expect_piece("a1", Color::White, PieceType::Rook);
  expect_piece("b1", Color::White, PieceType::Knight);
  expect_piece("c1", Color::White, PieceType::Bishop);
  expect_piece("d1", Color::White, PieceType::Queen);
  expect_piece("e1", Color::White, PieceType::King);
  expect_piece("f1", Color::White, PieceType::Bishop);
  expect_piece("g1", Color::White, PieceType::Knight);
  expect_piece("h1", Color::White, PieceType::Rook);
}

TEST_F(BoardStandardTest, WhitePawns) {
  for (char file = 'a'; file <= 'h'; ++file) {
    std::string sq = {file, '2'};
    expect_piece(sq, Color::White, PieceType::Pawn);
  }
}

TEST_F(BoardStandardTest, BlackBackRank) {
  expect_piece("a8", Color::Black, PieceType::Rook);
  expect_piece("b8", Color::Black, PieceType::Knight);
  expect_piece("c8", Color::Black, PieceType::Bishop);
  expect_piece("d8", Color::Black, PieceType::Queen);
  expect_piece("e8", Color::Black, PieceType::King);
  expect_piece("f8", Color::Black, PieceType::Bishop);
  expect_piece("g8", Color::Black, PieceType::Knight);
  expect_piece("h8", Color::Black, PieceType::Rook);
}

TEST_F(BoardStandardTest, BlackPawns) {
  for (char file = 'a'; file <= 'h'; ++file) {
    std::string sq = {file, '7'};
    expect_piece(sq, Color::Black, PieceType::Pawn);
  }
}

TEST_F(BoardStandardTest, MiddleRanksEmpty) {
  for (char rank = '3'; rank <= '6'; ++rank) {
    for (char file = 'a'; file <= 'h'; ++file) {
      std::string sq = {file, rank};
      expect_empty(sq);
    }
  }
}

TEST_F(BoardStandardTest, PieceCount) {
  int count = 0;
  for (int8_t row = 0; row < 8; ++row) {
    for (int8_t col = 0; col < 8; ++col) {
      if (board_.piece_at(Square{.row = row, .col = col}).has_value()) ++count;
    }
  }
  EXPECT_EQ(count, 32);
}

// --- Equality ---

TEST(Board, EqualityIdentical) {
  auto a = Board::standard();
  auto b = Board::standard();
  EXPECT_EQ(a, b);
}

TEST(Board, EqualityDifferent) {
  auto a = Board::standard();
  auto b = Board::standard();
  b.clear_square(Square{.row = 0, .col = 0});
  EXPECT_NE(a, b);
}

}  // namespace
}  // namespace chess
