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

  [[nodiscard]] constexpr char32_t display_char() const {
    // White: ♔ ♕ ♖ ♗ ♘ ♙  (U+2654..U+2659)
    // Black: ♚ ♛ ♜ ♝ ♞ ♟  (U+265A..U+265F)
    constexpr char32_t kWhiteKing = U'\u2654';
    constexpr char32_t kBlackKing = U'\u265A';
    auto base = (color == Color::White) ? kWhiteKing : kBlackKing;
    // King=0, Queen=1, Rook=2, Bishop=3, Knight=4, Pawn=5 — matches Unicode order
    return base + static_cast<char32_t>(type);
  }

  // UTF-8 encoded display string
  [[nodiscard]] std::string display_utf8() const {
    char32_t cp = display_char();
    std::string result;
    // All chess symbols are in U+2654..U+265F (3-byte UTF-8)
    result += static_cast<char>(0xE0 | ((cp >> 12) & 0x0F));
    result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    result += static_cast<char>(0x80 | (cp & 0x3F));
    return result;
  }

  Color color = Color::White;
  PieceType type = PieceType::Pawn;
};

}  // namespace chess
