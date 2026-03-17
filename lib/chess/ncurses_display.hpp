#pragma once

#include <algorithm>
#include <clocale>
#include <string>

#include <ncurses.h>

#include "chess/display.hpp"

namespace chess {

class NcursesDisplay final : public Display {
 public:
  NcursesDisplay() {
    std::setlocale(LC_ALL, "");
    initscr();
    start_color();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    mouseinterval(0);
    mousemask(BUTTON1_CLICKED | BUTTON1_PRESSED, nullptr);

    init_pair(kLightSquare, COLOR_BLACK, COLOR_WHITE);
    init_pair(kDarkSquare, COLOR_WHITE, COLOR_GREEN);
    init_pair(kCursor, COLOR_BLACK, COLOR_CYAN);
    init_pair(kSelected, COLOR_BLACK, COLOR_YELLOW);
    init_pair(kHighlight, COLOR_WHITE, COLOR_BLUE);
  }

  ~NcursesDisplay() override { endwin(); }

  NcursesDisplay(const NcursesDisplay&) = delete;
  NcursesDisplay& operator=(const NcursesDisplay&) = delete;

  void render_board(const GameState& state, Square cursor, std::optional<Square> selected,
                    std::span<const Square> highlights) override final {
    erase();

    for (int8_t row = 7; row >= 0; --row) {
      int screen_row = kBoardY + (7 - row);
      mvprintw(screen_row, 0, "%d", row + 1);

      for (int8_t col = 0; col < 8; ++col) {
        Square sq{.row = row, .col = col};
        int screen_col = kBoardX + col * kSquareWidth;

        short color = square_color(sq, cursor, selected, highlights);
        attron(COLOR_PAIR(color));

        auto piece = state.board.piece_at(sq);
        move(screen_row, screen_col);
        if (piece) {
          auto utf8 = piece->display_utf8();
          std::string str(reinterpret_cast<const char*>(utf8.data()), utf8.size());
          printw(" %s ", str.c_str());
        } else {
          printw("   ");
        }

        attroff(COLOR_PAIR(color));
      }
    }

    // File labels
    for (int col = 0; col < 8; ++col) {
      mvprintw(kBoardY + 8, kBoardX + col * kSquareWidth + 1, "%c", 'a' + col);
    }

    // Turn indicator
    mvprintw(kBoardY + 10, 0, "Turn: %s", state.active_color == Color::White ? "White" : "Black");

    // Status message
    if (!status_msg_.empty()) {
      mvprintw(kBoardY + 11, 0, "%s", status_msg_.c_str());
    }

    refresh();
  }

  void show_message(std::string_view msg) override final { status_msg_ = std::string(msg); }

  InputEvent get_input() override final {
    while (true) {
      int ch = getch();
      switch (ch) {
        case KEY_UP:
          return InputEvent{.type = InputType::ArrowUp, .square = {}};
        case KEY_DOWN:
          return InputEvent{.type = InputType::ArrowDown, .square = {}};
        case KEY_LEFT:
          return InputEvent{.type = InputType::ArrowLeft, .square = {}};
        case KEY_RIGHT:
          return InputEvent{.type = InputType::ArrowRight, .square = {}};
        case '\n':
        case KEY_ENTER:
          return InputEvent{.type = InputType::Enter, .square = {}};
        case 'q':
        case 'Q':
          return InputEvent{.type = InputType::Quit, .square = {}};
        case KEY_MOUSE: {
          MEVENT mevent;
          if (getmouse(&mevent) == OK) {
            auto sq = screen_to_square(mevent.y, mevent.x);
            if (sq) return InputEvent{.type = InputType::MouseClick, .square = *sq};
          }
          break;
        }
        default:
          break;
      }
    }
  }

 private:
  static constexpr short kLightSquare = 1;
  static constexpr short kDarkSquare = 2;
  static constexpr short kCursor = 3;
  static constexpr short kSelected = 4;
  static constexpr short kHighlight = 5;

  static constexpr int kBoardX = 3;
  static constexpr int kBoardY = 1;
  static constexpr int kSquareWidth = 3;

  short square_color(Square sq, Square cursor, std::optional<Square> selected,
                     std::span<const Square> highlights) const {
    if (selected && sq == *selected) return kSelected;
    if (sq == cursor) return kCursor;
    if (std::find(highlights.begin(), highlights.end(), sq) != highlights.end()) return kHighlight;
    bool is_light = (sq.row + sq.col) % 2 != 0;
    return is_light ? kLightSquare : kDarkSquare;
  }

  static std::optional<Square> screen_to_square(int screen_row, int screen_col) {
    int row = 7 - (screen_row - kBoardY);
    int col = (screen_col - kBoardX) / kSquareWidth;
    Square sq{.row = static_cast<int8_t>(row), .col = static_cast<int8_t>(col)};
    if (sq.is_valid()) return sq;
    return std::nullopt;
  }

  std::string status_msg_;
};

}  // namespace chess
