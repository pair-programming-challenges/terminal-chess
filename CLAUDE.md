# Terminal Chess

## Project
A terminal-based chess game built as a coding challenge (https://codingchallenges.fyi/challenges/challenge-chess).
Human vs computer with a chess engine using game tree search and pruning.

### Challenge Steps
1. Display the chess board with pieces in starting positions (ncurses terminal UI).
2. Validate possible moves for each piece type based on position and board state (TDD).
3. Alternating turns: human moves (validated) vs computer (initially random valid moves).
4. Win condition checking (checkmate, stalemate, draw).
5. Chess engine: game tree with minmax, alpha-beta pruning, or Monte Carlo tree search. Selectable difficulty level (e.g. search depth, evaluation sophistication).

### Bonus steps
6. Log play to a file in figurine algebraic notation.
7. Replay games interactively from log files.
8. Highlight valid moves for selected piece.

## UI
- ncurses for terminal rendering (board, input, status messages).
- Unicode chess piece characters for display:
  - White: `♔ ♕ ♖ ♗ ♘ ♙`
  - Black: `♚ ♛ ♜ ♝ ♞ ♟`
- Wrap ncurses in an abstract interface for testability (inject a stub in tests).
- Humans can play by using either the mouse or keyboard (arrow keys and ENTER).

## Design for Future Network Play
- Core game logic must be fully decoupled from I/O (rendering, input, transport).
- Abstract a `Player` interface so a player can be local (human/AI) or remote (network) without changing game logic.
- No networking in the initial implementation, but the architecture must not preclude adding it later.

## Project Structure
- `lib/` - Header-only library (CMake `INTERFACE`). Core chess logic lives here.
- `bin/` - Executable entry points only.
- `test/unit/` - Unit tests. `test/integration/` - Integration tests.

# C++ Conventions

## Standard
- C++23. Use `std::span`, `std::optional`, `std::string_view`, designated initializers, `enum class`, `constexpr`.
- Prefer `constexpr` and compile-time computation where possible.

## Style
- Classes/Structs: `PascalCase` (`Board`, `MoveGenerator`, `ChessEngine`)
- Functions/methods: `snake_case` (`generate_moves`, `is_checkmate`, `apply_move`)
- Variables: `snake_case` (`piece_type`, `target_square`)
- Private members: `trailing_underscore_` (`board_`, `turn_`, `history_`)
- Constants: `kPascalCase` for local constexpr (`kBoardSize`, `kMaxMoves`), system macros stay `UPPER_CASE`
- Enums: `enum class` with `PascalCase` values (`PieceType::Knight`, `Color::White`)
- Namespaces: `lowercase::nested` (`chess::engine`)
- Public functions of classes and structs should be declared before private ones.
- Member variables of classes and structs should be at the end.

## Formatting
- `.clang-format` based on Google style, `ColumnLimit: 120`.
- `#pragma once` for include guards.
- Include order: system `<>` headers, blank line, project `""` headers.
- Headers: `.hpp`, sources: `.cpp`.
- Git hook on commit for enforcing format.

## Architecture
- Header-only library in `lib/` (CMake `INTERFACE` library). Only executables in `bin/`.
- Dependency injection via abstract base classes with `std::unique_ptr`.
- RAII for resource management. Acquire in constructor, release in destructor.
- Delete copy operations on RAII types: `ClassName(const ClassName&) = delete;`
- Use `override final` on concrete implementations of virtual methods.
- Static factory methods on structs for semantic construction (`Move::castle(Color::White, Side::KingSide)`).

## Error Handling
- `std::optional<T>` for recoverable failures (parsing user input, invalid moves).
- `std::runtime_error` for unrecoverable failures (corrupted board state).
- Graceful fallbacks where appropriate (re-prompt on invalid input).

## Memory & Performance
- `std::unique_ptr` for exclusive ownership. No naked `new`/`delete`.
- `std::move` for ownership transfer. Pass large objects by `const&`.
- `std::string_view` and `std::span` for non-owning views in function parameters.
- `std::array` for fixed-size buffers (board representation). `std::vector` with `reserve` for dynamic (move lists).
- Stack-allocate short-lived RAII objects.

## Designated Initializers
- When using designated initializers, initialize ALL fields explicitly to avoid `-Wmissing-designated-field-initializers`.
- Prefer static factory methods over raw designated initializers for repeated patterns.

## Testing (Strict TDD)
- **Red-Green-Refactor**: Write a failing test first, write minimal code to pass, then refactor. No production code without a failing test.
- GoogleTest. `TEST(Suite, Case)` or `TEST_F(Fixture, Case)` naming.
- Arrange/Act/Assert pattern.
- Unit tests in `test/unit/`, integration tests in `test/integration/`.
- Manual stub classes implementing abstract interfaces (no Google Mock).
- Use fixtures (`::testing::Test`) for shared setup and helper methods.
- Factory helpers in tests to reduce boilerplate (`make_board`, `make_move`).
- Integration tests in `test/integration/` to simulate full games: sequence of turns through to checkmate, stalemate, or draw.
- Github Actions defined in `.github/workflows`.

## Build
- CMake 3.16+. `CMAKE_CXX_STANDARD 23`, `CMAKE_CXX_STANDARD_REQUIRED ON`.
- Compiler flags: `-Wall -Werror -Wextra -Wpedantic`.
- `FetchContent` for external dependencies (googletest, cxxopts, ncurses via `find_package`).
- `gtest_discover_tests()` for automatic test registration.
