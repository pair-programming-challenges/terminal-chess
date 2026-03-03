#include "chess/move_generator.hpp"

#include <algorithm>

#include <gtest/gtest.h>

namespace chess {
namespace {

// --- Helpers ---

Square sq(std::string_view s) { return *Square::from_algebraic(s); }

// Piece shorthand constants
constexpr Piece WK{.color = Color::White, .type = PieceType::King};
constexpr Piece WQ{.color = Color::White, .type = PieceType::Queen};
constexpr Piece WR{.color = Color::White, .type = PieceType::Rook};
constexpr Piece WB{.color = Color::White, .type = PieceType::Bishop};
constexpr Piece WN{.color = Color::White, .type = PieceType::Knight};
constexpr Piece WP{.color = Color::White, .type = PieceType::Pawn};
constexpr Piece BK{.color = Color::Black, .type = PieceType::King};
constexpr Piece BR{.color = Color::Black, .type = PieceType::Rook};
constexpr Piece BB{.color = Color::Black, .type = PieceType::Bishop};
constexpr Piece BP{.color = Color::Black, .type = PieceType::Pawn};

constexpr CastlingRights kNoCastling{
    .white_kingside = false, .white_queenside = false, .black_kingside = false, .black_queenside = false};

bool has_move(const std::vector<Move>& moves, Square from, Square to) {
  return std::any_of(moves.begin(), moves.end(), [&](const Move& m) { return m.from == from && m.to == to; });
}

bool has_move_type(const std::vector<Move>& moves, Square from, Square to, MoveType type) {
  return std::any_of(moves.begin(), moves.end(),
                     [&](const Move& m) { return m.from == from && m.to == to && m.type == type; });
}

int count_moves_from(const std::vector<Move>& moves, Square from) {
  return static_cast<int>(std::count_if(moves.begin(), moves.end(), [&](const Move& m) { return m.from == from; }));
}

int count_promotions(const std::vector<Move>& moves, Square from, Square to) {
  return static_cast<int>(std::count_if(moves.begin(), moves.end(), [&](const Move& m) {
    return m.from == from && m.to == to && m.type == MoveType::Promotion;
  }));
}

GameState make_state(Color active = Color::White) {
  GameState state;
  state.active_color = active;
  state.castling = kNoCastling;
  state.en_passant_target = std::nullopt;
  state.halfmove_clock = 0;
  state.fullmove_number = 1;
  return state;
}

// Equivalent to the old generate_legal_moves free function:
// iterates all squares, generates pseudo-legal moves, filters by legality.
std::vector<Move> all_legal_moves(const GameState& state) {
  MoveGenerator gen(state);
  std::vector<Move> result;
  for (int8_t r = 0; r < 8; ++r) {
    for (int8_t c = 0; c < 8; ++c) {
      for (const auto& m : gen.generate_moves({.row = r, .col = c})) {
        if (gen.is_move_legal(m)) result.push_back(m);
      }
    }
  }
  return result;
}

// ============================================================
// Attack detection
// ============================================================

TEST(IsSquareAttacked, EmptyBoardNotAttacked) {
  Board board;
  EXPECT_FALSE(MoveGenerator::is_square_attacked(board, sq("e4"), Color::White));
  EXPECT_FALSE(MoveGenerator::is_square_attacked(board, sq("e4"), Color::Black));
}

TEST(IsSquareAttacked, AttackedByKnight) {
  Board board;
  board.set_piece(sq("c3"), WN);
  EXPECT_TRUE(MoveGenerator::is_square_attacked(board, sq("e4"), Color::White));
  EXPECT_TRUE(MoveGenerator::is_square_attacked(board, sq("d5"), Color::White));
  EXPECT_FALSE(MoveGenerator::is_square_attacked(board, sq("c4"), Color::White));
}

TEST(IsSquareAttacked, AttackedByBishop) {
  Board board;
  board.set_piece(sq("b1"), WB);
  EXPECT_TRUE(MoveGenerator::is_square_attacked(board, sq("e4"), Color::White));
  EXPECT_FALSE(MoveGenerator::is_square_attacked(board, sq("e1"), Color::White));
}

TEST(IsSquareAttacked, AttackedByRook) {
  Board board;
  board.set_piece(sq("e1"), WR);
  EXPECT_TRUE(MoveGenerator::is_square_attacked(board, sq("e4"), Color::White));
  EXPECT_TRUE(MoveGenerator::is_square_attacked(board, sq("a1"), Color::White));
  EXPECT_FALSE(MoveGenerator::is_square_attacked(board, sq("d2"), Color::White));
}

TEST(IsSquareAttacked, AttackedByQueen) {
  Board board;
  board.set_piece(sq("e1"), WQ);
  // Orthogonal
  EXPECT_TRUE(MoveGenerator::is_square_attacked(board, sq("e8"), Color::White));
  // Diagonal
  EXPECT_TRUE(MoveGenerator::is_square_attacked(board, sq("h4"), Color::White));
}

TEST(IsSquareAttacked, AttackedByPawn) {
  Board board;
  board.set_piece(sq("d3"), WP);
  // White pawn attacks diagonally forward (toward higher rows)
  EXPECT_TRUE(MoveGenerator::is_square_attacked(board, sq("c4"), Color::White));
  EXPECT_TRUE(MoveGenerator::is_square_attacked(board, sq("e4"), Color::White));
  // Does not attack forward or backward
  EXPECT_FALSE(MoveGenerator::is_square_attacked(board, sq("d4"), Color::White));
  EXPECT_FALSE(MoveGenerator::is_square_attacked(board, sq("d2"), Color::White));

  // Black pawn attacks toward lower rows
  board.set_piece(sq("d6"), BP);
  EXPECT_TRUE(MoveGenerator::is_square_attacked(board, sq("c5"), Color::Black));
  EXPECT_TRUE(MoveGenerator::is_square_attacked(board, sq("e5"), Color::Black));
}

TEST(IsSquareAttacked, AttackedByKing) {
  Board board;
  board.set_piece(sq("e4"), WK);
  EXPECT_TRUE(MoveGenerator::is_square_attacked(board, sq("e5"), Color::White));
  EXPECT_TRUE(MoveGenerator::is_square_attacked(board, sq("f5"), Color::White));
  EXPECT_FALSE(MoveGenerator::is_square_attacked(board, sq("e6"), Color::White));
}

TEST(IsSquareAttacked, SlidingPieceBlockedByFriendly) {
  Board board;
  board.set_piece(sq("e1"), WR);
  board.set_piece(sq("e3"), WP);
  // Rook blocked by own pawn
  EXPECT_TRUE(MoveGenerator::is_square_attacked(board, sq("e2"), Color::White));
  EXPECT_FALSE(MoveGenerator::is_square_attacked(board, sq("e4"), Color::White));
}

// ============================================================
// Knight moves
// ============================================================

TEST(MoveGenerator, KnightCenterHas8Moves) {
  auto state = make_state();
  state.board.set_piece(sq("e4"), WN);
  state.board.set_piece(sq("a1"), WK);
  state.board.set_piece(sq("h8"), BK);

  auto moves = all_legal_moves(state);
  EXPECT_EQ(count_moves_from(moves, sq("e4")), 8);
}

TEST(MoveGenerator, KnightCornerHas2Moves) {
  auto state = make_state();
  state.board.set_piece(sq("a1"), WN);
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("e8"), BK);

  auto moves = all_legal_moves(state);
  EXPECT_EQ(count_moves_from(moves, sq("a1")), 2);
  EXPECT_TRUE(has_move(moves, sq("a1"), sq("b3")));
  EXPECT_TRUE(has_move(moves, sq("a1"), sq("c2")));
}

TEST(MoveGenerator, KnightBlockedByFriendly) {
  auto state = make_state();
  state.board.set_piece(sq("a1"), WN);
  state.board.set_piece(sq("b3"), WP);  // Blocks one destination
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("e8"), BK);

  auto moves = all_legal_moves(state);
  EXPECT_EQ(count_moves_from(moves, sq("a1")), 1);
  EXPECT_TRUE(has_move(moves, sq("a1"), sq("c2")));
}

TEST(MoveGenerator, KnightCapturesEnemy) {
  auto state = make_state();
  state.board.set_piece(sq("e4"), WN);
  state.board.set_piece(sq("f6"), BP);  // Enemy on one destination
  state.board.set_piece(sq("a1"), WK);
  state.board.set_piece(sq("h8"), BK);

  auto moves = all_legal_moves(state);
  EXPECT_TRUE(has_move(moves, sq("e4"), sq("f6")));

  // Verify it's a capture
  auto it = std::find_if(moves.begin(), moves.end(),
                         [](const Move& m) { return m.from == sq("e4") && m.to == sq("f6"); });
  ASSERT_NE(it, moves.end());
  ASSERT_TRUE(it->captured.has_value());
  EXPECT_EQ(*it->captured, PieceType::Pawn);
}

// ============================================================
// Bishop moves
// ============================================================

TEST(MoveGenerator, BishopOpenBoard) {
  auto state = make_state();
  state.board.set_piece(sq("d4"), WB);
  state.board.set_piece(sq("a1"), WK);
  state.board.set_piece(sq("h8"), BK);

  auto moves = all_legal_moves(state);
  // d4 bishop: diagonals reach a1,b2,c3 + e5,f6,g7 + a7,b6,c5 + e3,f2,g1 = 13 squares
  // But a1 is occupied by WK, so that diagonal stops at b2,c3 = 2
  // e5,f6,g7 = 3 (h8 has BK, so g7 is last empty, but h8 is BK = capture? h8 has BK)
  // Actually h8 has enemy king. That's a pseudo-legal capture but wouldn't be legal.
  // Hmm, but capturing the king isn't really valid in standard move gen... actually it IS generated
  // as pseudo-legal but the legality filter won't remove it because the moving side's king isn't in check.
  // Hmm, but capturing the opponent's king shouldn't be possible in a legal position.
  // Let me just move the kings out of the way.

  // Let me redesign this test with kings elsewhere
  auto state2 = make_state();
  state2.board.set_piece(sq("d4"), WB);
  state2.board.set_piece(sq("a2"), WK);
  state2.board.set_piece(sq("h1"), BK);

  auto moves2 = all_legal_moves(state2);
  // NE: e5,f6,g7,h8 = 4; NW: c5,b6,a7 = 3; SE: e3,f2 (g1 empty, but check h1?) = e3,f2,g1 = 3
  // Wait, h1 has BK. g1 is empty. SE diagonal from d4: e3,f2,g1. Then h0 is off-board. So 3.
  // SW: c3,b2,a1 = 3
  // Total: 4 + 3 + 3 + 3 = 13
  EXPECT_EQ(count_moves_from(moves2, sq("d4")), 13);
}

TEST(MoveGenerator, BishopBlockedByFriendly) {
  auto state = make_state();
  state.board.set_piece(sq("d4"), WB);
  state.board.set_piece(sq("e5"), WP);  // Blocks NE diagonal
  state.board.set_piece(sq("a2"), WK);
  state.board.set_piece(sq("h1"), BK);

  auto moves = all_legal_moves(state);
  // NE blocked at e5 (friendly) = 0; NW: c5,b6,a7 = 3; SE: e3,f2,g1 = 3; SW: c3,b2,a1 = 3
  // Total: 0 + 3 + 3 + 3 = 9
  EXPECT_EQ(count_moves_from(moves, sq("d4")), 9);
}

TEST(MoveGenerator, BishopCapturesEnemy) {
  auto state = make_state();
  state.board.set_piece(sq("d4"), WB);
  state.board.set_piece(sq("f6"), BP);  // Enemy on NE diagonal
  state.board.set_piece(sq("a2"), WK);
  state.board.set_piece(sq("h1"), BK);

  auto moves = all_legal_moves(state);
  EXPECT_TRUE(has_move(moves, sq("d4"), sq("f6")));
  // NE: e5, f6(capture) = 2; NW: c5,b6,a7 = 3; SE: e3,f2,g1 = 3; SW: c3,b2,a1 = 3
  EXPECT_EQ(count_moves_from(moves, sq("d4")), 11);
}

// ============================================================
// Rook moves
// ============================================================

TEST(MoveGenerator, RookOpenBoard) {
  auto state = make_state();
  state.board.set_piece(sq("d4"), WR);
  state.board.set_piece(sq("a1"), WK);
  state.board.set_piece(sq("h8"), BK);

  auto moves = all_legal_moves(state);
  // d4 rook: up d5,d6,d7,d8=4; down d3,d2,d1=3; right e4,f4,g4,h4=4; left c4,b4,a4=3
  // Total: 4+3+4+3 = 14
  EXPECT_EQ(count_moves_from(moves, sq("d4")), 14);
}

TEST(MoveGenerator, RookBlockedByFriendly) {
  auto state = make_state();
  state.board.set_piece(sq("d4"), WR);
  state.board.set_piece(sq("d6"), WP);  // Blocks upward
  state.board.set_piece(sq("a1"), WK);
  state.board.set_piece(sq("h8"), BK);

  auto moves = all_legal_moves(state);
  // up d5(blocked at d6)=1; down d3,d2,d1=3; right e4,f4,g4,h4=4; left c4,b4,a4=3
  EXPECT_EQ(count_moves_from(moves, sq("d4")), 11);
}

// ============================================================
// Queen moves
// ============================================================

TEST(MoveGenerator, QueenCombinesBishopAndRook) {
  auto state = make_state();
  state.board.set_piece(sq("d4"), WQ);
  state.board.set_piece(sq("a1"), WK);
  state.board.set_piece(sq("h8"), BK);

  auto moves = all_legal_moves(state);
  // Rook-like: 14 (same as rook test above minus king positions)
  // Bishop-like: NE e5,f6,g7(h8 has BK=can capture)=4; NW c5,b6,a7=3; SE e3,f2,g1=3; SW c3,b2(a1 has WK)=2
  // Rook-like: up d5,d6,d7,d8=4; down d3,d2,d1=3; right e4,f4,g4,h4=4; left c4,b4,a4=3
  // Total: 4+3+3+2 + 4+3+4+3 = 26
  // But wait, g7->h8 is capturing BK which may cause issues with legality...
  // Let me recalculate with kings placed safely.
  auto state2 = make_state();
  state2.board.set_piece(sq("d4"), WQ);
  state2.board.set_piece(sq("a2"), WK);
  state2.board.set_piece(sq("h1"), BK);

  auto moves2 = all_legal_moves(state2);
  // Bishop-like: NE e5,f6,g7,h8=4; NW c5,b6,a7=3; SE e3,f2,g1=3; SW c3,b2,a1=3 = 13
  // Rook-like: up d5,d6,d7,d8=4; down d3,d2,d1=3; right e4,f4,g4,h4=4; left c4,b4,a4=3 = 14
  // Total: 27
  EXPECT_EQ(count_moves_from(moves2, sq("d4")), 27);
}

// ============================================================
// Pawn moves
// ============================================================

TEST(MoveGenerator, WhitePawnSingleAndDoublePush) {
  auto state = make_state();
  state.board.set_piece(sq("e2"), WP);
  state.board.set_piece(sq("a1"), WK);
  state.board.set_piece(sq("h8"), BK);

  auto moves = all_legal_moves(state);
  EXPECT_TRUE(has_move(moves, sq("e2"), sq("e3")));
  EXPECT_TRUE(has_move(moves, sq("e2"), sq("e4")));
  EXPECT_EQ(count_moves_from(moves, sq("e2")), 2);
}

TEST(MoveGenerator, PawnNotOnStartRankOnlySinglePush) {
  auto state = make_state();
  state.board.set_piece(sq("e3"), WP);
  state.board.set_piece(sq("a1"), WK);
  state.board.set_piece(sq("h8"), BK);

  auto moves = all_legal_moves(state);
  EXPECT_TRUE(has_move(moves, sq("e3"), sq("e4")));
  EXPECT_FALSE(has_move(moves, sq("e3"), sq("e5")));
  EXPECT_EQ(count_moves_from(moves, sq("e3")), 1);
}

TEST(MoveGenerator, PawnBlockedCannotPush) {
  auto state = make_state();
  state.board.set_piece(sq("e2"), WP);
  state.board.set_piece(sq("e3"), BP);  // Blocked
  state.board.set_piece(sq("a1"), WK);
  state.board.set_piece(sq("h8"), BK);

  auto moves = all_legal_moves(state);
  EXPECT_EQ(count_moves_from(moves, sq("e2")), 0);
}

TEST(MoveGenerator, PawnDoubleBlockedCannotDoublePush) {
  auto state = make_state();
  state.board.set_piece(sq("e2"), WP);
  state.board.set_piece(sq("e4"), BP);  // Blocks double push
  state.board.set_piece(sq("a1"), WK);
  state.board.set_piece(sq("h8"), BK);

  auto moves = all_legal_moves(state);
  // Single push possible, double push blocked
  EXPECT_TRUE(has_move(moves, sq("e2"), sq("e3")));
  EXPECT_FALSE(has_move(moves, sq("e2"), sq("e4")));
}

TEST(MoveGenerator, PawnCapture) {
  auto state = make_state();
  state.board.set_piece(sq("e4"), WP);
  state.board.set_piece(sq("d5"), BP);
  state.board.set_piece(sq("f5"), BP);
  state.board.set_piece(sq("a1"), WK);
  state.board.set_piece(sq("h8"), BK);

  auto moves = all_legal_moves(state);
  EXPECT_TRUE(has_move(moves, sq("e4"), sq("d5")));
  EXPECT_TRUE(has_move(moves, sq("e4"), sq("f5")));
  EXPECT_TRUE(has_move(moves, sq("e4"), sq("e5")));  // Single push
  EXPECT_EQ(count_moves_from(moves, sq("e4")), 3);
}

TEST(MoveGenerator, PawnEnPassant) {
  auto state = make_state();
  state.board.set_piece(sq("e5"), WP);
  state.board.set_piece(sq("d5"), BP);
  state.en_passant_target = sq("d6");
  state.board.set_piece(sq("a1"), WK);
  state.board.set_piece(sq("h8"), BK);

  auto moves = all_legal_moves(state);
  EXPECT_TRUE(has_move_type(moves, sq("e5"), sq("d6"), MoveType::EnPassant));
}

TEST(MoveGenerator, PawnPromotion) {
  auto state = make_state();
  state.board.set_piece(sq("e7"), WP);
  state.board.set_piece(sq("a1"), WK);
  state.board.set_piece(sq("h8"), BK);

  auto moves = all_legal_moves(state);
  // 4 promotion moves for push to e8
  EXPECT_EQ(count_promotions(moves, sq("e7"), sq("e8")), 4);
}

TEST(MoveGenerator, PawnPromotionCapture) {
  auto state = make_state();
  state.board.set_piece(sq("e7"), WP);
  state.board.set_piece(sq("d8"), BR);  // Enemy piece to capture
  state.board.set_piece(sq("a1"), WK);
  state.board.set_piece(sq("h8"), BK);

  auto moves = all_legal_moves(state);
  // 4 promotion moves for push + 4 for capture = 8
  EXPECT_EQ(count_promotions(moves, sq("e7"), sq("e8")), 4);
  EXPECT_EQ(count_promotions(moves, sq("e7"), sq("d8")), 4);
}

TEST(MoveGenerator, BlackPawnMovesSouth) {
  auto state = make_state(Color::Black);
  state.board.set_piece(sq("e7"), BP);
  state.board.set_piece(sq("a1"), WK);
  state.board.set_piece(sq("h8"), BK);

  auto moves = all_legal_moves(state);
  EXPECT_TRUE(has_move(moves, sq("e7"), sq("e6")));
  EXPECT_TRUE(has_move(moves, sq("e7"), sq("e5")));
  EXPECT_EQ(count_moves_from(moves, sq("e7")), 2);
}

TEST(MoveGenerator, BlackPawnPromotion) {
  auto state = make_state(Color::Black);
  state.board.set_piece(sq("e2"), BP);
  state.board.set_piece(sq("a1"), WK);
  state.board.set_piece(sq("h8"), BK);

  auto moves = all_legal_moves(state);
  EXPECT_EQ(count_promotions(moves, sq("e2"), sq("e1")), 4);
}

// ============================================================
// King moves
// ============================================================

TEST(MoveGenerator, KingBasicMoves) {
  auto state = make_state();
  state.board.set_piece(sq("e4"), WK);
  state.board.set_piece(sq("a8"), BK);

  auto moves = all_legal_moves(state);
  EXPECT_EQ(count_moves_from(moves, sq("e4")), 8);
}

TEST(MoveGenerator, KingCannotMoveIntoCheck) {
  auto state = make_state();
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("e8"), BK);
  state.board.set_piece(sq("d8"), BR);  // Controls d-file

  auto moves = all_legal_moves(state);
  // King can't go to d1 or d2 (controlled by rook)
  EXPECT_FALSE(has_move(moves, sq("e1"), sq("d1")));
  EXPECT_FALSE(has_move(moves, sq("e1"), sq("d2")));
  // Can go to f1, f2
  EXPECT_TRUE(has_move(moves, sq("e1"), sq("f1")));
  EXPECT_TRUE(has_move(moves, sq("e1"), sq("f2")));
}

// ============================================================
// Castling
// ============================================================

TEST(MoveGenerator, CastlingKingsideWhite) {
  auto state = make_state();
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("h1"), WR);
  state.board.set_piece(sq("e8"), BK);
  state.castling = CastlingRights{
      .white_kingside = true, .white_queenside = false, .black_kingside = false, .black_queenside = false};

  auto moves = all_legal_moves(state);
  EXPECT_TRUE(has_move_type(moves, sq("e1"), sq("g1"), MoveType::Castle));
}

TEST(MoveGenerator, CastlingQueensideWhite) {
  auto state = make_state();
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("a1"), WR);
  state.board.set_piece(sq("e8"), BK);
  state.castling = CastlingRights{
      .white_kingside = false, .white_queenside = true, .black_kingside = false, .black_queenside = false};

  auto moves = all_legal_moves(state);
  EXPECT_TRUE(has_move_type(moves, sq("e1"), sq("c1"), MoveType::Castle));
}

TEST(MoveGenerator, CastlingKingsideBlack) {
  auto state = make_state(Color::Black);
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("e8"), BK);
  state.board.set_piece(sq("h8"), BR);
  state.castling = CastlingRights{
      .white_kingside = false, .white_queenside = false, .black_kingside = true, .black_queenside = false};

  auto moves = all_legal_moves(state);
  EXPECT_TRUE(has_move_type(moves, sq("e8"), sq("g8"), MoveType::Castle));
}

TEST(MoveGenerator, CastlingBlockedByPiece) {
  auto state = make_state();
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("h1"), WR);
  state.board.set_piece(sq("f1"), WB);  // Piece in the way
  state.board.set_piece(sq("e8"), BK);
  state.castling = CastlingRights{
      .white_kingside = true, .white_queenside = false, .black_kingside = false, .black_queenside = false};

  auto moves = all_legal_moves(state);
  EXPECT_FALSE(has_move_type(moves, sq("e1"), sq("g1"), MoveType::Castle));
}

TEST(MoveGenerator, CastlingBlockedByCheck) {
  auto state = make_state();
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("h1"), WR);
  state.board.set_piece(sq("e8"), BK);
  state.board.set_piece(sq("e7"), BR);  // Puts White king in check via e-file
  state.castling = CastlingRights{
      .white_kingside = true, .white_queenside = false, .black_kingside = false, .black_queenside = false};

  auto moves = all_legal_moves(state);
  EXPECT_FALSE(has_move_type(moves, sq("e1"), sq("g1"), MoveType::Castle));
}

TEST(MoveGenerator, CastlingThroughAttackedSquare) {
  auto state = make_state();
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("h1"), WR);
  state.board.set_piece(sq("a8"), BK);
  state.board.set_piece(sq("f8"), BR);  // Controls f-file, attacking f1
  state.castling = CastlingRights{
      .white_kingside = true, .white_queenside = false, .black_kingside = false, .black_queenside = false};

  auto moves = all_legal_moves(state);
  EXPECT_FALSE(has_move_type(moves, sq("e1"), sq("g1"), MoveType::Castle));
}

TEST(MoveGenerator, CastlingNoRights) {
  auto state = make_state();
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("h1"), WR);
  state.board.set_piece(sq("a1"), WR);
  state.board.set_piece(sq("e8"), BK);
  state.castling = kNoCastling;

  auto moves = all_legal_moves(state);
  EXPECT_FALSE(has_move_type(moves, sq("e1"), sq("g1"), MoveType::Castle));
  EXPECT_FALSE(has_move_type(moves, sq("e1"), sq("c1"), MoveType::Castle));
}

// ============================================================
// Legality: pins, checks
// ============================================================

TEST(MoveGenerator, PinnedPieceCannotMoveOffPinLine) {
  // White rook on e4 pinned to king on e1 by black rook on e8
  auto state = make_state();
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("e4"), WR);  // Pinned on e-file
  state.board.set_piece(sq("e8"), BR);  // Pins the rook
  state.board.set_piece(sq("a8"), BK);

  auto moves = all_legal_moves(state);

  // The white rook on e4 can only move along the e-file (staying on the pin line)
  // It can move to e2,e3,e5,e6,e7,e8(capture)
  auto rook_moves =
      std::count_if(moves.begin(), moves.end(), [](const Move& m) { return m.from == sq("e4") && m.to.col == 4; });
  EXPECT_EQ(count_moves_from(moves, sq("e4")), rook_moves);  // All rook moves stay on e-file
}

TEST(MoveGenerator, PinnedPieceCanMoveAlongPinLine) {
  auto state = make_state();
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("e4"), WR);
  state.board.set_piece(sq("e8"), BR);
  state.board.set_piece(sq("a8"), BK);

  auto moves = all_legal_moves(state);
  // Rook can move along e-file: e2, e3, e5, e6, e7, e8(capture) = 6
  EXPECT_TRUE(has_move(moves, sq("e4"), sq("e2")));
  EXPECT_TRUE(has_move(moves, sq("e4"), sq("e5")));
  EXPECT_TRUE(has_move(moves, sq("e4"), sq("e8")));  // Capture the pinning rook
  EXPECT_EQ(count_moves_from(moves, sq("e4")), 6);
}

TEST(MoveGenerator, KingInCheckMustResolve) {
  // Black rook on e8 gives check to white king on e1
  auto state = make_state();
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("e8"), BR);
  state.board.set_piece(sq("a8"), BK);
  state.board.set_piece(sq("a2"), WR);  // Can block or stay

  auto moves = all_legal_moves(state);

  // Every legal move must resolve the check
  for (const auto& move : moves) {
    Board copy = state.board;
    MoveGenerator::apply_move_to_board(copy, move);
    Square king_sq = MoveGenerator::find_king(copy, Color::White);
    EXPECT_FALSE(MoveGenerator::is_square_attacked(copy, king_sq, Color::Black))
        << "Move to " << move.to.to_algebraic() << " doesn't resolve check";
  }
}

TEST(MoveGenerator, DoubleCheckOnlyKingCanMove) {
  // King on e1, attacked by rook on e8 and bishop on b4 = double check
  auto state = make_state();
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("e8"), BR);
  state.board.set_piece(sq("b4"), BB);
  state.board.set_piece(sq("a8"), BK);
  state.board.set_piece(sq("d2"), WN);  // Knight can't help in double check

  auto moves = all_legal_moves(state);

  // In double check, only king moves are legal
  for (const auto& move : moves) {
    EXPECT_EQ(move.piece, PieceType::King) << "Non-king move found in double check: from " << move.from.to_algebraic();
  }
  EXPECT_GT(static_cast<int>(moves.size()), 0);
}

// ============================================================
// Starting position
// ============================================================

TEST(MoveGenerator, StartingPositionHas20Moves) {
  auto state = GameState::standard();
  auto moves = all_legal_moves(state);
  EXPECT_EQ(static_cast<int>(moves.size()), 20);
}

TEST(MoveGenerator, StartingPositionBlackHas20Moves) {
  auto state = GameState::standard();
  state.active_color = Color::Black;
  auto moves = all_legal_moves(state);
  EXPECT_EQ(static_cast<int>(moves.size()), 20);
}

// ============================================================
// generate_all_legal_moves() — promoted from test helper
// ============================================================

TEST(MoveGenerator, GenerateAllLegalMovesMatchesHelper) {
  auto state = GameState::standard();
  MoveGenerator gen(state);
  auto api_moves = gen.generate_all_legal_moves();
  auto helper_moves = all_legal_moves(state);
  EXPECT_EQ(api_moves.size(), helper_moves.size());
  EXPECT_EQ(api_moves, helper_moves);
}

TEST(MoveGenerator, GenerateAllLegalMovesEmptyPosition) {
  // Lone kings — each has limited moves
  auto state = make_state();
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("e8"), BK);
  MoveGenerator gen(state);
  auto moves = gen.generate_all_legal_moves();
  EXPECT_EQ(count_moves_from(moves, sq("e1")), 5);  // d1,d2,f1,f2,e2
}

// ============================================================
// En passant edge case: discovered check
// ============================================================

TEST(MoveGenerator, EnPassantBlockedByDiscoveredCheck) {
  // White king on a5, white pawn on d5, black pawn on e5 (just double-pushed), black rook on h5
  // En passant dxe6 would remove both pawns from rank 5, exposing king to rook
  auto state = make_state();
  state.board.set_piece(sq("a5"), WK);
  state.board.set_piece(sq("d5"), WP);
  state.board.set_piece(sq("e5"), BP);
  state.board.set_piece(sq("h5"), BR);
  state.board.set_piece(sq("a8"), BK);
  state.en_passant_target = sq("e6");

  auto moves = all_legal_moves(state);
  // En passant should be illegal because it discovers check
  EXPECT_FALSE(has_move_type(moves, sq("d5"), sq("e6"), MoveType::EnPassant));
}

// ============================================================
// Per-piece generate_moves (pseudo-legal) tests
// ============================================================

TEST(MoveGeneratorPerPiece, GenerateMovesReturnsEmptyForEmptySquare) {
  auto state = make_state();
  state.board.set_piece(sq("a1"), WK);
  state.board.set_piece(sq("h8"), BK);

  MoveGenerator gen(state);
  auto moves = gen.generate_moves(sq("e4"));
  EXPECT_TRUE(moves.empty());
}

TEST(MoveGeneratorPerPiece, GenerateMovesReturnsEmptyForOpponentPiece) {
  auto state = make_state();  // White to move
  state.board.set_piece(sq("a1"), WK);
  state.board.set_piece(sq("h8"), BK);

  MoveGenerator gen(state);
  // h8 has a black king, but it's White's turn
  auto moves = gen.generate_moves(sq("h8"));
  EXPECT_TRUE(moves.empty());
}

TEST(MoveGeneratorPerPiece, GenerateMovesIncludesPseudoLegalMoves) {
  // A pinned piece generates pseudo-legal moves (off the pin line)
  // that is_move_legal would reject
  auto state = make_state();
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("e4"), WR);  // Pinned on e-file
  state.board.set_piece(sq("e8"), BR);
  state.board.set_piece(sq("a8"), BK);

  MoveGenerator gen(state);
  auto moves = gen.generate_moves(sq("e4"));

  // Pseudo-legal includes off-pin moves (e.g. to d4, f4)
  EXPECT_TRUE(has_move(moves, sq("e4"), sq("d4")));
  EXPECT_TRUE(has_move(moves, sq("e4"), sq("f4")));
  // Also includes on-pin moves
  EXPECT_TRUE(has_move(moves, sq("e4"), sq("e5")));
  EXPECT_TRUE(has_move(moves, sq("e4"), sq("e8")));
}

TEST(MoveGeneratorPerPiece, GenerateMovesKnightFromCenter) {
  auto state = make_state();
  state.board.set_piece(sq("e4"), WN);
  state.board.set_piece(sq("a1"), WK);
  state.board.set_piece(sq("h8"), BK);

  MoveGenerator gen(state);
  auto moves = gen.generate_moves(sq("e4"));
  EXPECT_EQ(static_cast<int>(moves.size()), 8);
}

TEST(MoveGeneratorPerPiece, GenerateMovesForPawnWithPromotion) {
  auto state = make_state();
  state.board.set_piece(sq("e7"), WP);
  state.board.set_piece(sq("a1"), WK);
  state.board.set_piece(sq("h8"), BK);

  MoveGenerator gen(state);
  auto moves = gen.generate_moves(sq("e7"));
  // 4 promotion options for single push to e8
  EXPECT_EQ(count_promotions(moves, sq("e7"), sq("e8")), 4);
}

// ============================================================
// is_move_legal tests
// ============================================================

TEST(MoveGeneratorLegality, LegalMoveReturnsTrue) {
  auto state = make_state();
  state.board.set_piece(sq("e2"), WP);
  state.board.set_piece(sq("a1"), WK);
  state.board.set_piece(sq("h8"), BK);

  MoveGenerator gen(state);
  auto move = Move::normal(sq("e2"), sq("e3"), PieceType::Pawn);
  EXPECT_TRUE(gen.is_move_legal(move));
}

TEST(MoveGeneratorLegality, IllegalMoveLeavingKingInCheck) {
  // Rook pinned on e-file: moving it off the file leaves king in check
  auto state = make_state();
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("e4"), WR);
  state.board.set_piece(sq("e8"), BR);
  state.board.set_piece(sq("a8"), BK);

  MoveGenerator gen(state);
  // Moving rook to d4 (off pin line) is illegal
  auto illegal = Move::normal(sq("e4"), sq("d4"), PieceType::Rook);
  EXPECT_FALSE(gen.is_move_legal(illegal));

  // Moving rook to e5 (along pin line) is legal
  auto legal = Move::normal(sq("e4"), sq("e5"), PieceType::Rook);
  EXPECT_TRUE(gen.is_move_legal(legal));
}

TEST(MoveGeneratorLegality, KingCannotMoveIntoAttackedSquare) {
  auto state = make_state();
  state.board.set_piece(sq("e1"), WK);
  state.board.set_piece(sq("d8"), BR);  // Controls d-file
  state.board.set_piece(sq("h8"), BK);

  MoveGenerator gen(state);
  // King moving to d1 is illegal (attacked by rook)
  auto illegal = Move::normal(sq("e1"), sq("d1"), PieceType::King);
  EXPECT_FALSE(gen.is_move_legal(illegal));

  // King moving to f1 is legal
  auto legal = Move::normal(sq("e1"), sq("f1"), PieceType::King);
  EXPECT_TRUE(gen.is_move_legal(legal));
}

}  // namespace
}  // namespace chess
