#pragma once

#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "chess/game_state.hpp"
#include "chess/types.hpp"

namespace chess {

enum class InputType : uint8_t {
  ArrowUp,
  ArrowDown,
  ArrowLeft,
  ArrowRight,
  Enter,
  MouseClick,
  Quit,
};

struct InputEvent {
  InputType type = InputType::Quit;
  Square square{};  // Meaningful for MouseClick
};

class Display {
 public:
  virtual ~Display() = default;

  virtual void render_board(const GameState& state, Square cursor, std::optional<Square> selected,
                            std::span<const Square> highlights) = 0;

  virtual void show_message(std::string_view msg) = 0;

  virtual InputEvent get_input() = 0;

 protected:
  Display() = default;
  Display(const Display&) = default;
  Display& operator=(const Display&) = default;
  Display(Display&&) = default;
  Display& operator=(Display&&) = default;
};

}  // namespace chess
