#include "chess/human_player.hpp"

#include <gtest/gtest.h>

namespace chess {
namespace {

// --- Helpers ---

Square sq(std::string_view s) { return *Square::from_algebraic(s); }

constexpr CastlingRights kNoCastling{
    .white_kingside = false, .white_queenside = false, .black_kingside = false, .black_queenside = false};

// --- StubDisplay ---

class StubDisplay final : public Display {
 public:
  void render_board(const GameState& /*state*/, Square cursor, std::optional<Square> selected,
                    std::span<const Square> highlights) override final {
    render_calls.push_back(
        {.cursor = cursor, .selected = selected, .highlights = {highlights.begin(), highlights.end()}});
  }

  void show_message(std::string_view msg) override final { messages.emplace_back(msg); }

  InputEvent get_input() override final {
    if (input_idx >= inputs.size()) throw std::runtime_error("StubDisplay: no more inputs");
    return inputs[input_idx++];
  }

  struct RenderCall {
    Square cursor;
    std::optional<Square> selected;
    std::vector<Square> highlights;
  };

  std::vector<InputEvent> inputs;
  size_t input_idx = 0;
  std::vector<RenderCall> render_calls;
  std::vector<std::string> messages;
};

// --- Common test state ---

GameState test_state() {
  GameState state;
  state.active_color = Color::White;
  state.castling = kNoCastling;
  state.en_passant_target = std::nullopt;
  state.halfmove_clock = 0;
  state.fullmove_number = 1;
  state.board.set_piece(sq("e1"), Piece{.color = Color::White, .type = PieceType::King});
  state.board.set_piece(sq("e2"), Piece{.color = Color::White, .type = PieceType::Pawn});
  state.board.set_piece(sq("d1"), Piece{.color = Color::White, .type = PieceType::Rook});
  state.board.set_piece(sq("e8"), Piece{.color = Color::Black, .type = PieceType::King});
  return state;
}

// ============================================================
// Mouse click selection
// ============================================================

TEST(HumanPlayer, MouseClickSelectsPieceAndDestination) {
  auto state = test_state();
  std::vector<Move> legal_moves = {
      Move::normal(sq("e2"), sq("e3"), PieceType::Pawn),
      Move::normal(sq("e2"), sq("e4"), PieceType::Pawn),
  };

  StubDisplay display;
  display.inputs = {
      InputEvent{.type = InputType::MouseClick, .square = sq("e2")},
      InputEvent{.type = InputType::MouseClick, .square = sq("e4")},
  };

  HumanPlayer player(display);
  auto chosen = player.choose_move(state, legal_moves);

  EXPECT_EQ(chosen, Move::normal(sq("e2"), sq("e4"), PieceType::Pawn));
}

TEST(HumanPlayer, ClickingEnemyPieceDoesNotSelect) {
  auto state = test_state();
  std::vector<Move> legal_moves = {
      Move::normal(sq("e2"), sq("e3"), PieceType::Pawn),
  };

  StubDisplay display;
  display.inputs = {
      InputEvent{.type = InputType::MouseClick, .square = sq("e8")},  // black king — ignored
      InputEvent{.type = InputType::MouseClick, .square = sq("e2")},  // select pawn
      InputEvent{.type = InputType::MouseClick, .square = sq("e3")},  // move
  };

  HumanPlayer player(display);
  auto chosen = player.choose_move(state, legal_moves);

  EXPECT_EQ(chosen, Move::normal(sq("e2"), sq("e3"), PieceType::Pawn));
}

TEST(HumanPlayer, ReselectsDifferentPiece) {
  auto state = test_state();
  std::vector<Move> legal_moves = {
      Move::normal(sq("e2"), sq("e3"), PieceType::Pawn),
      Move::normal(sq("d1"), sq("d2"), PieceType::Rook),
  };

  StubDisplay display;
  display.inputs = {
      InputEvent{.type = InputType::MouseClick, .square = sq("e2")},  // select pawn
      InputEvent{.type = InputType::MouseClick, .square = sq("d1")},  // reselect rook
      InputEvent{.type = InputType::MouseClick, .square = sq("d2")},  // move rook
  };

  HumanPlayer player(display);
  auto chosen = player.choose_move(state, legal_moves);

  EXPECT_EQ(chosen, Move::normal(sq("d1"), sq("d2"), PieceType::Rook));
}

// ============================================================
// Keyboard navigation
// ============================================================

TEST(HumanPlayer, ArrowKeysMoveCursor) {
  auto state = test_state();
  std::vector<Move> legal_moves = {
      Move::normal(sq("e2"), sq("e3"), PieceType::Pawn),
  };

  StubDisplay display;
  // Start at a1 (0,0). Navigate: right×4 → e1, up → e2, enter (select), up → e3, enter (move)
  display.inputs = {
      InputEvent{.type = InputType::ArrowRight, .square = {}},
      InputEvent{.type = InputType::ArrowRight, .square = {}},
      InputEvent{.type = InputType::ArrowRight, .square = {}},
      InputEvent{.type = InputType::ArrowRight, .square = {}},
      InputEvent{.type = InputType::ArrowUp, .square = {}},
      InputEvent{.type = InputType::Enter, .square = {}},  // select e2
      InputEvent{.type = InputType::ArrowUp, .square = {}},
      InputEvent{.type = InputType::Enter, .square = {}},  // move to e3
  };

  HumanPlayer player(display);
  auto chosen = player.choose_move(state, legal_moves);

  EXPECT_EQ(chosen, Move::normal(sq("e2"), sq("e3"), PieceType::Pawn));
}

TEST(HumanPlayer, CursorClampsToBoard) {
  auto state = test_state();
  std::vector<Move> legal_moves = {
      Move::normal(sq("e2"), sq("e3"), PieceType::Pawn),
  };

  StubDisplay display;
  // Try to move below row 0 and left of col 0, then navigate to make a move
  display.inputs = {
      InputEvent{.type = InputType::ArrowDown, .square = {}},   // stays at row 0
      InputEvent{.type = InputType::ArrowLeft, .square = {}},   // stays at col 0
      InputEvent{.type = InputType::ArrowRight, .square = {}},  // col 1
      InputEvent{.type = InputType::ArrowRight, .square = {}},  // col 2
      InputEvent{.type = InputType::ArrowRight, .square = {}},  // col 3
      InputEvent{.type = InputType::ArrowRight, .square = {}},  // col 4
      InputEvent{.type = InputType::ArrowUp, .square = {}},     // row 1 = e2
      InputEvent{.type = InputType::Enter, .square = {}},       // select e2
      InputEvent{.type = InputType::ArrowUp, .square = {}},     // row 2 = e3
      InputEvent{.type = InputType::Enter, .square = {}},       // move to e3
  };

  HumanPlayer player(display);
  auto chosen = player.choose_move(state, legal_moves);

  EXPECT_EQ(chosen, Move::normal(sq("e2"), sq("e3"), PieceType::Pawn));
  // First render should show cursor at a1 (clamped)
  EXPECT_EQ(display.render_calls[0].cursor, sq("a1"));
}

// ============================================================
// Highlights and deselection
// ============================================================

TEST(HumanPlayer, HighlightsLegalTargetsOnSelection) {
  auto state = test_state();
  std::vector<Move> legal_moves = {
      Move::normal(sq("e2"), sq("e3"), PieceType::Pawn),
      Move::normal(sq("e2"), sq("e4"), PieceType::Pawn),
  };

  StubDisplay display;
  display.inputs = {
      InputEvent{.type = InputType::MouseClick, .square = sq("e2")},
      InputEvent{.type = InputType::MouseClick, .square = sq("e3")},
  };

  HumanPlayer player(display);
  auto chosen = player.choose_move(state, legal_moves);
  (void)chosen;

  // render_calls[0]: initial (no selection). render_calls[1]: after selecting e2.
  ASSERT_GE(display.render_calls.size(), 2u);
  auto& after_select = display.render_calls[1];
  ASSERT_TRUE(after_select.selected.has_value());
  EXPECT_EQ(*after_select.selected, sq("e2"));
  EXPECT_EQ(after_select.highlights.size(), 2u);
  EXPECT_NE(std::find(after_select.highlights.begin(), after_select.highlights.end(), sq("e3")),
            after_select.highlights.end());
  EXPECT_NE(std::find(after_select.highlights.begin(), after_select.highlights.end(), sq("e4")),
            after_select.highlights.end());
}

TEST(HumanPlayer, DeselectOnClickEmptySquare) {
  auto state = test_state();
  std::vector<Move> legal_moves = {
      Move::normal(sq("e2"), sq("e3"), PieceType::Pawn),
  };

  StubDisplay display;
  display.inputs = {
      InputEvent{.type = InputType::MouseClick, .square = sq("e2")},  // select
      InputEvent{.type = InputType::MouseClick, .square = sq("a5")},  // empty → deselect
      InputEvent{.type = InputType::MouseClick, .square = sq("e2")},  // reselect
      InputEvent{.type = InputType::MouseClick, .square = sq("e3")},  // move
  };

  HumanPlayer player(display);
  auto chosen = player.choose_move(state, legal_moves);

  EXPECT_EQ(chosen, Move::normal(sq("e2"), sq("e3"), PieceType::Pawn));
  // render_calls[2] is after the deselect click
  ASSERT_GE(display.render_calls.size(), 3u);
  EXPECT_FALSE(display.render_calls[2].selected.has_value());
  EXPECT_TRUE(display.render_calls[2].highlights.empty());
}

// ============================================================
// Promotion
// ============================================================

TEST(HumanPlayer, PromotionDefaultsToQueen) {
  GameState state;
  state.active_color = Color::White;
  state.castling = kNoCastling;
  state.en_passant_target = std::nullopt;
  state.halfmove_clock = 0;
  state.fullmove_number = 1;
  state.board.set_piece(sq("e1"), Piece{.color = Color::White, .type = PieceType::King});
  state.board.set_piece(sq("e7"), Piece{.color = Color::White, .type = PieceType::Pawn});
  state.board.set_piece(sq("a8"), Piece{.color = Color::Black, .type = PieceType::King});

  std::vector<Move> legal_moves = {
      Move::promotion(sq("e7"), sq("e8"), PieceType::Queen),
      Move::promotion(sq("e7"), sq("e8"), PieceType::Rook),
      Move::promotion(sq("e7"), sq("e8"), PieceType::Bishop),
      Move::promotion(sq("e7"), sq("e8"), PieceType::Knight),
  };

  StubDisplay display;
  display.inputs = {
      InputEvent{.type = InputType::MouseClick, .square = sq("e7")},
      InputEvent{.type = InputType::MouseClick, .square = sq("e8")},
  };

  HumanPlayer player(display);
  auto chosen = player.choose_move(state, legal_moves);

  EXPECT_EQ(chosen, Move::promotion(sq("e7"), sq("e8"), PieceType::Queen));
}

// ============================================================
// Quit
// ============================================================

TEST(HumanPlayer, QuitThrowsException) {
  auto state = test_state();
  std::vector<Move> legal_moves = {
      Move::normal(sq("e2"), sq("e3"), PieceType::Pawn),
  };

  StubDisplay display;
  display.inputs = {
      InputEvent{.type = InputType::Quit, .square = {}},
  };

  HumanPlayer player(display);
  EXPECT_THROW((void)player.choose_move(state, legal_moves), std::runtime_error);
}

}  // namespace
}  // namespace chess
