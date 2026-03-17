#include "chess/game_loop.hpp"

#include <gtest/gtest.h>

#include <queue>
#include <stdexcept>

namespace chess {
namespace {

// --- StubPlayer: pops moves from a pre-populated queue ---

class StubPlayer final : public Player {
 public:
  explicit StubPlayer(std::vector<Move> moves) {
    for (auto& m : moves) moves_.push(m);
  }

  [[nodiscard]] Move choose_move(const GameState& /*state*/, std::span<const Move> /*legal_moves*/) override final {
    if (moves_.empty()) {
      throw std::runtime_error("StubPlayer: no more moves in queue");
    }
    Move m = moves_.front();
    moves_.pop();
    return m;
  }

 private:
  std::queue<Move> moves_;
};

// --- Helpers ---

Square sq(std::string_view s) { return *Square::from_algebraic(s); }

// ============================================================
// Scholar's Mate (4-move checkmate)
// ============================================================
// 1. e4   e5
// 2. Bc4  Nc6
// 3. Qh5  Nf6??
// 4. Qxf7#
//
// 7 half-moves total. After White's 4th move the game is over.

TEST(GameLoop, ScholarsMate) {
  std::vector<Move> white_moves = {
      Move::normal(sq("e2"), sq("e4"), PieceType::Pawn),                     // 1. e4
      Move::normal(sq("f1"), sq("c4"), PieceType::Bishop),                   // 2. Bc4
      Move::normal(sq("d1"), sq("h5"), PieceType::Queen),                    // 3. Qh5
      Move::capture(sq("h5"), sq("f7"), PieceType::Queen, PieceType::Pawn),  // 4. Qxf7#
  };
  std::vector<Move> black_moves = {
      Move::normal(sq("e7"), sq("e5"), PieceType::Pawn),    // 1... e5
      Move::normal(sq("b8"), sq("c6"), PieceType::Knight),  // 2... Nc6
      Move::normal(sq("g8"), sq("f6"), PieceType::Knight),  // 3... Nf6??
  };

  auto white = std::make_unique<StubPlayer>(std::move(white_moves));
  auto black = std::make_unique<StubPlayer>(std::move(black_moves));
  Game game(std::move(white), std::move(black));

  GameStatus status{};

  // Play 6 half-moves (3 full moves), game should still be ongoing
  for (int i = 0; i < 6; ++i) {
    status = game.step();
    EXPECT_FALSE(status.result.has_value()) << "Game ended prematurely at half-move " << (i + 1);
  }

  // 7th half-move: Qxf7# — checkmate
  status = game.step();
  ASSERT_TRUE(status.result.has_value());
  EXPECT_EQ(status.result->outcome, Outcome::Checkmate);
  ASSERT_TRUE(status.result->winner.has_value());
  EXPECT_EQ(*status.result->winner, Color::White);

  // History should have all 7 moves
  EXPECT_EQ(game.history().size(), 7u);
}

// ============================================================
// Stalemate position
// ============================================================

TEST(GameLoop, StalematePosition) {
  // Set up a position where Black is stalemated after White's move.
  // White: Ka6, Qb6. Black: Ka8.
  // White plays Qb6 — but let's set up a position right before stalemate.
  //
  // Position: White Kb6, Qa6+, Black Ka8. But that's check, not stalemate.
  // Classic stalemate: White Kb6, Qc6, Black Ka8.
  // Black to move: king on a8, blocked by Kb6 (covers a7,b7,b8), Qc6 (covers a8? no, c6 covers a6,b7,c7,c8,d6,etc)
  // Actually let's use: White Kf6, Qe6, Black Kf8. Stalemate if Black to move.
  // Kf8 squares: e8(Qe6 controls), e7(Kf6 controls), f7(Kf6 controls), g7(Kf6 controls), g8(Qe6 controls via diagonal).
  // Hmm that needs checking. Let me use a well-known stalemate:
  // White: Ka1, Qb3. Black: Ka8 (isn't stalemated, can go to b8).
  // Let me just construct it directly in the GameState.

  GameState state;
  state.active_color = Color::White;
  state.castling = CastlingRights{
      .white_kingside = false, .white_queenside = false, .black_kingside = false, .black_queenside = false};
  state.en_passant_target = std::nullopt;
  state.halfmove_clock = 0;
  state.fullmove_number = 1;

  // White: Kb6, Qc5. Black: Ka8.
  // After some White move that produces stalemate.
  // Simpler: start from a position where it's one move away from stalemate.
  // Position before: White Kb6, Qa6. Black Ka8. It's White's turn.
  // White plays Qa7 (not possible, a7 would be check/mate). Hmm.
  //
  // Let's take the simplest approach: start from a stalemate position directly
  // and verify step() returns stalemate immediately.

  // Stalemate: Black to move, king on a8. White Qa6, Kb6.
  // Black king can go to: a7(Qa6), b8(Kb6/Qa6 covers b7, Kb6 covers c7, but b8 is not covered).
  // Wait, Kb6 covers a5,a6(occupied),a7,b5,b7,c5,c6,c7. Qa6 covers a1-a8 file, b7,c8,d3,e2,f1,b6(occupied),b5...
  // a8: not attacked. b8: covered by Qa6? Qa6 on a6, b8 is not on any line from a6 unless diagonal. a6 to b7 is
  // diagonal. a6 to b8 is NOT (that would be knight move). So b8 might be free. Hmm, this isn't clearly stalemate. Let
  // me use a known one.
  //
  // Known stalemate: White Kc6, Qd6. Black Ka7. Black to move.
  // Ka7 options: a8(Qd6 covers a-file? No, Qd6 is on d6. Rook-like: d1-d8 and a6-h6. Bishop-like: c5,b4,a3 and
  // e5,f4,g3,h2 and c7,b8 and e7,f8.) So Qd6 covers: rank 6, file d, diag c5-a3, diag e7-f8, diag c7-b8, diag e5-h2.
  // Ka7: a8 — is it attacked? Qd6 doesn't cover a8 directly. Kc6 covers b6,b7,c7,d7,d6(occ),c5,b5.
  // a8 is not covered by either. So Ka7->a8 is legal. Not stalemate.
  //
  // OK let me just use the classic: White Kf7, Qg6. Black Kh8. Black to move.
  // Kh8 options: g8(Kf7 + Qg6), h7(Qg6 covers h7? g6 to h7 is diagonal yes). g7(Kf7 covers).
  // That's actually checkmate if Qg6 covers h7. Let me check: Qg6 diagonal to h7 yes. So h7 covered.
  // g8 covered by Kf7. g7 covered by Kf7 and Qg6. So all squares covered. Is h8 in check? Qg6 to h8: no (would need to
  // be on same file/rank/diag). g6-h7 is one diagonal step, g6-h8 is a knight move. Kf7 doesn't reach h8. So Kh8 is NOT
  // in check. No legal moves. STALEMATE. But wait, can I verify g8 is really covered? Qg6: covers g7,g8 (along g-file).
  // Yes. And Kf7 covers g8 too. All exits blocked. So the position is: Black Kh8, White Kf7 Qg6, Black to move =
  // stalemate.

  state.active_color = Color::Black;
  state.board.set_piece(sq("h8"), Piece{.color = Color::Black, .type = PieceType::King});
  state.board.set_piece(sq("f7"), Piece{.color = Color::White, .type = PieceType::King});
  state.board.set_piece(sq("g6"), Piece{.color = Color::White, .type = PieceType::Queen});

  // Black has no moves, so step() should detect stalemate with no player interaction
  auto black_stub = std::make_unique<StubPlayer>(std::vector<Move>{});
  auto white_stub = std::make_unique<StubPlayer>(std::vector<Move>{});
  Game game(std::move(white_stub), std::move(black_stub), state);

  auto status = game.step();
  ASSERT_TRUE(status.result.has_value());
  EXPECT_EQ(status.result->outcome, Outcome::Stalemate);
  EXPECT_FALSE(status.result->winner.has_value());

  // No moves should be in history (game ended before any player acted)
  EXPECT_EQ(game.history().size(), 0u);
}

// ============================================================
// 50-move draw
// ============================================================

TEST(GameLoop, FiftyMoveDrawDetected) {
  GameState state;
  state.active_color = Color::White;
  state.castling = CastlingRights{
      .white_kingside = false, .white_queenside = false, .black_kingside = false, .black_queenside = false};
  state.en_passant_target = std::nullopt;
  state.halfmove_clock = 100;  // Already at 50-move threshold
  state.fullmove_number = 51;
  state.board.set_piece(sq("e1"), Piece{.color = Color::White, .type = PieceType::King});
  state.board.set_piece(sq("e8"), Piece{.color = Color::Black, .type = PieceType::King});

  auto white_stub = std::make_unique<StubPlayer>(std::vector<Move>{});
  auto black_stub = std::make_unique<StubPlayer>(std::vector<Move>{});
  Game game(std::move(white_stub), std::move(black_stub), state);

  auto status = game.step();
  ASSERT_TRUE(status.result.has_value());
  EXPECT_EQ(status.result->outcome, Outcome::Draw);
  EXPECT_FALSE(status.result->winner.has_value());

  // No move was played
  EXPECT_EQ(game.history().size(), 0u);
}

// ============================================================
// History accumulates
// ============================================================

TEST(GameLoop, HistoryAccumulatesMoves) {
  // Play 4 half-moves from starting position using real legal moves
  std::vector<Move> white_moves = {
      Move::normal(sq("e2"), sq("e4"), PieceType::Pawn),
      Move::normal(sq("d2"), sq("d4"), PieceType::Pawn),
  };
  std::vector<Move> black_moves = {
      Move::normal(sq("e7"), sq("e5"), PieceType::Pawn),
      Move::normal(sq("d7"), sq("d5"), PieceType::Pawn),
  };

  auto white = std::make_unique<StubPlayer>(std::move(white_moves));
  auto black = std::make_unique<StubPlayer>(std::move(black_moves));
  Game game(std::move(white), std::move(black));

  game.step();
  EXPECT_EQ(game.history().size(), 1u);

  game.step();
  EXPECT_EQ(game.history().size(), 2u);

  game.step();
  EXPECT_EQ(game.history().size(), 3u);

  game.step();
  EXPECT_EQ(game.history().size(), 4u);
}

// ============================================================
// Active color alternates
// ============================================================

TEST(GameLoop, ActiveColorAlternates) {
  std::vector<Move> white_moves = {
      Move::normal(sq("e2"), sq("e4"), PieceType::Pawn),
  };
  std::vector<Move> black_moves = {
      Move::normal(sq("e7"), sq("e5"), PieceType::Pawn),
  };

  auto white = std::make_unique<StubPlayer>(std::move(white_moves));
  auto black = std::make_unique<StubPlayer>(std::move(black_moves));
  Game game(std::move(white), std::move(black));

  EXPECT_EQ(game.state().active_color, Color::White);

  game.step();
  EXPECT_EQ(game.state().active_color, Color::Black);

  game.step();
  EXPECT_EQ(game.state().active_color, Color::White);
}

// ============================================================
// Ongoing game returns no result
// ============================================================

TEST(GameLoop, OngoingReturnsNoResult) {
  std::vector<Move> white_moves = {
      Move::normal(sq("e2"), sq("e4"), PieceType::Pawn),
  };
  std::vector<Move> black_moves = {};

  auto white = std::make_unique<StubPlayer>(std::move(white_moves));
  auto black = std::make_unique<StubPlayer>(std::move(black_moves));
  Game game(std::move(white), std::move(black));

  auto status = game.step();
  // After 1.e4, the game is still ongoing
  EXPECT_FALSE(status.result.has_value());
}

// ============================================================
// Smothered Mate
// ============================================================
// Classic Philidor smothered mate pattern:
// White: Kg1, Qa2, Nf5. Black: Kg8, Rf8, pawns g7, h7.
// No f7 pawn, so the a2-g8 diagonal is clear for the queen sacrifice.
//
// 1. Nh6+  Kh8   (knight check on g8, king to corner)
// 2. Qg8+  Rxg8  (queen sacrifice via a2-g8 diagonal, rook must recapture)
// 3. Nf7#         (smothered mate — king on h8, blocked by Rg8, g7, h7)

TEST(GameLoop, SmotheredMate) {
  GameState state;
  state.active_color = Color::White;
  state.castling = CastlingRights{
      .white_kingside = false, .white_queenside = false, .black_kingside = false, .black_queenside = false};
  state.en_passant_target = std::nullopt;
  state.halfmove_clock = 0;
  state.fullmove_number = 1;

  // White pieces
  state.board.set_piece(sq("g1"), Piece{.color = Color::White, .type = PieceType::King});
  state.board.set_piece(sq("a2"), Piece{.color = Color::White, .type = PieceType::Queen});
  state.board.set_piece(sq("f5"), Piece{.color = Color::White, .type = PieceType::Knight});

  // Black pieces (no f7 pawn — keeps a2-g8 diagonal open)
  state.board.set_piece(sq("g8"), Piece{.color = Color::Black, .type = PieceType::King});
  state.board.set_piece(sq("f8"), Piece{.color = Color::Black, .type = PieceType::Rook});
  state.board.set_piece(sq("g7"), Piece{.color = Color::Black, .type = PieceType::Pawn});
  state.board.set_piece(sq("h7"), Piece{.color = Color::Black, .type = PieceType::Pawn});

  std::vector<Move> white_moves = {
      Move::normal(sq("f5"), sq("h6"), PieceType::Knight),  // 1. Nh6+
      Move::normal(sq("a2"), sq("g8"), PieceType::Queen),   // 2. Qg8+ (sac)
      Move::normal(sq("h6"), sq("f7"), PieceType::Knight),  // 3. Nf7#
  };
  std::vector<Move> black_moves = {
      Move::normal(sq("g8"), sq("h8"), PieceType::King),                     // 1... Kh8
      Move::capture(sq("f8"), sq("g8"), PieceType::Rook, PieceType::Queen),  // 2... Rxg8
  };

  auto white = std::make_unique<StubPlayer>(std::move(white_moves));
  auto black = std::make_unique<StubPlayer>(std::move(black_moves));
  Game game(std::move(white), std::move(black), state);

  GameStatus status{};

  // Play 4 half-moves (each player moves twice), game ongoing
  for (int i = 0; i < 4; ++i) {
    status = game.step();
    EXPECT_FALSE(status.result.has_value()) << "Game ended prematurely at half-move " << (i + 1);
  }

  // 5th half-move: Nf7# — smothered mate
  status = game.step();
  ASSERT_TRUE(status.result.has_value());
  EXPECT_EQ(status.result->outcome, Outcome::Checkmate);
  ASSERT_TRUE(status.result->winner.has_value());
  EXPECT_EQ(*status.result->winner, Color::White);

  EXPECT_EQ(game.history().size(), 5u);
}

}  // namespace
}  // namespace chess
