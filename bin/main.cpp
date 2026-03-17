#include <memory>

#include "chess/game_loop.hpp"
#include "chess/human_player.hpp"
#include "chess/ncurses_display.hpp"

int main() {
  chess::NcursesDisplay display;

  auto human = std::make_unique<chess::HumanPlayer>(display);
  auto computer = std::make_unique<chess::RandomPlayer>();

  chess::Game game(std::move(human), std::move(computer));

  chess::GameStatus status{};
  while (!status.result.has_value()) {
    try {
      status = game.step();
    } catch (const std::runtime_error&) {
      return 0;  // Player quit
    }
  }

  // Show final result
  const char* msg = "Game over.";
  if (status.result->outcome == chess::Outcome::Checkmate) {
    msg = status.result->winner == chess::Color::White ? "Checkmate! White wins!" : "Checkmate! Black wins!";
  } else if (status.result->outcome == chess::Outcome::Stalemate) {
    msg = "Stalemate! Draw.";
  } else {
    msg = "Draw!";
  }

  display.show_message(msg);
  display.render_board(game.state(), chess::Square{}, std::nullopt, {});

  // Wait for user to press any key before exiting
  display.get_input();

  return 0;
}
