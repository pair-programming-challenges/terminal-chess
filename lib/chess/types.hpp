#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace chess {

enum class Color : uint8_t { White, Black };

[[nodiscard]] constexpr Color opposite(Color c) { return c == Color::White ? Color::Black : Color::White; }

enum class PieceType : uint8_t { King, Queen, Rook, Bishop, Knight, Pawn };

struct Square {
  [[nodiscard]] constexpr bool is_valid() const { return row >= 0 && row <= 7 && col >= 0 && col <= 7; }

  [[nodiscard]] constexpr bool operator==(const Square&) const = default;

  // "e4" -> Square{.row=3, .col=4}
  [[nodiscard]] static constexpr std::optional<Square> from_algebraic(std::string_view s) {
    if (s.size() != 2) return std::nullopt;
    auto file = s[0];
    auto rank = s[1];
    if (file < 'a' || file > 'h' || rank < '1' || rank > '8') return std::nullopt;
    return Square{.row = static_cast<int8_t>(rank - '1'), .col = static_cast<int8_t>(file - 'a')};
  }

  // Square{.row=3, .col=4} -> "e4"
  [[nodiscard]] std::string to_algebraic() const {
    std::string result(2, '\0');
    result[0] = static_cast<char>('a' + col);
    result[1] = static_cast<char>('1' + row);
    return result;
  }

  int8_t row = 0;  // 0 = rank 1 (White's back rank), 7 = rank 8
  int8_t col = 0;  // 0 = file a, 7 = file h
};

struct Piece {
  [[nodiscard]] constexpr bool operator==(const Piece&) const = default;

  [[nodiscard]] constexpr std::u8string_view display_utf8() const {
    static constexpr std::u8string_view kWhite[] = {u8"♔", u8"♕", u8"♖", u8"♗", u8"♘", u8"♙"};
    static constexpr std::u8string_view kBlack[] = {u8"♚", u8"♛", u8"♜", u8"♝", u8"♞", u8"♟"};
    return (color == Color::White) ? kWhite[static_cast<int>(type)] : kBlack[static_cast<int>(type)];
  }

  [[nodiscard]] constexpr wchar_t display_wchar() const {
    // Unicode code points for chess pieces
    static constexpr wchar_t kWhite[] = {L'\u2654', L'\u2655', L'\u2656', L'\u2657', L'\u2658', L'\u2659'};
    static constexpr wchar_t kBlack[] = {L'\u265A', L'\u265B', L'\u265C', L'\u265D', L'\u265E', L'\u265F'};
    return (color == Color::White) ? kWhite[static_cast<int>(type)] : kBlack[static_cast<int>(type)];
  }

  Color color = Color::White;
  PieceType type = PieceType::Pawn;
};

}  // namespace chess
