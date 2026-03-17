# TODO

## Phase 1: Project Scaffolding

Set up the build system and directory layout so everything compiles from day one.

- [x] CMakeLists.txt — Root CMake config with C++23, `-Wall -Werror -Wextra -Wpedantic`, FetchContent for GoogleTest. INTERFACE library target for `lib/`, test executable for `test/unit/`.
- [x] Directory structure — Create `lib/chess/`, `bin/`, `test/unit/`, `test/integration/`.
- [x] Verify build — Write a trivial GoogleTest case, confirm `cmake --build` and `ctest` pass.

## Phase 2: Core Types (`lib/chess/types.hpp`)

Define the fundamental value types that everything else is built on.

- [x] `Color` enum — `enum class Color { White, Black }`. Helper `opposite(Color)`.
- [x] `PieceType` enum — `enum class PieceType { King, Queen, Rook, Bishop, Knight, Pawn }`.
- [x] `Square` struct — Row (0–7) + col (0–7). `is_valid()`, conversion to/from algebraic notation (`"e4"` ↔ `Square{.row=3, .col=4}`). `operator==` for comparisons.
- [x] `Piece` struct — `{Color, PieceType}`. `display_char()` returning the Unicode chess character (e.g. `♞` for Black Knight).
- [x] Tests — Square validity, algebraic notation round-trip, Piece display characters for all 12 piece/color combinations.

## Phase 3: Board (`lib/chess/board.hpp`)

The board is a flat array of 64 optional pieces. Read/write access by square.

- [x] `Board` class — `std::array<std::optional<Piece>, 64>`. Methods: `piece_at(Square) -> std::optional<Piece>`, `set_piece(Square, Piece)`, `clear_square(Square)`. Private helper `index(Square) -> size_t`.
- [x] `Board::standard()` — Static factory that returns the standard starting position (white on rows 0–1, black on rows 6–7).
- [x] Tests — Empty board has no pieces, standard position has correct pieces at all 64 squares, set/clear round-trip.

## Phase 4: Move Representation (`lib/chess/move.hpp`)

A plain value type describing a single move. No logic — just data + construction helpers.

- [x] `MoveType` enum — `enum class MoveType { Normal, Castle, EnPassant, Promotion }`.
- [x] `Move` struct — Fields: `from`, `to`, `piece`, `captured` (optional), `promoted_to` (optional), `type`. `operator==` for comparisons.
- [x] Static factories — `Move::normal(from, to, piece)`, `Move::capture(from, to, piece, captured)`, `Move::en_passant(from, to)`, `Move::castle(king_from, king_to)`, `Move::promotion(from, to, promote_to, captured)`.
- [x] Tests — Each factory sets the correct fields. Two moves with same data are equal.

## Phase 5: Move Generation (`lib/chess/move_generator.hpp`)

Generate all legal moves for the active color. Two-stage: pseudo-legal generation, then legality filter.

- [x] Sliding helper — `generate_sliding_moves(board, from, directions)` iterates along each direction vector until blocked or capturing.
- [x] Pawn moves — Single push, double push (from starting rank), diagonal captures, en passant, promotion (4 moves per promotion square: Q/R/B/N).
- [x] Knight moves — 8 L-shaped offsets, filter for on-board and not own piece.
- [x] Bishop moves — Sliding along 4 diagonal directions.
- [x] Rook moves — Sliding along 4 orthogonal directions.
- [x] Queen moves — Union of bishop + rook directions.
- [x] King moves — 8 adjacent squares + castling (if rights exist, path clear, king doesn't pass through or land in check).
- [x] `is_square_attacked(board, square, attacker_color)` — Reverse-probe: check knight offsets for enemy knights, diagonals for bishops/queens, ranks/files for rooks/queens, adjacent for king, pawn attack squares for pawns. Used for check detection and castling validation.
- [x] Legality filter — For each pseudo-legal move, apply to a board copy, check if own king is attacked. Discard if yes.
- [x] `generate_legal_moves(state) -> std::vector<Move>` — Public entry point combining all of the above.
- [x] Tests — Each piece type from various positions: open board, blocked paths, captures, edge-of-board. Pawn promotion, en passant, castling (both sides), castling blocked by check/attack/pieces. Pin scenarios (pinned piece can't move off pin line). Starting position move count (20 for White). En passant discovered check edge case. (86 tests total, 44 for move generation.)

## Phase 6: Game State (`lib/chess/game_state.hpp`)

Full game state tracking, move application, and end-condition detection.

- [x] `GameState` class — Fields: `Board`, active `Color`, castling rights (4 bools: K/Q per side), en passant target square (optional), halfmove clock, fullmove number.
- [x] `apply_move(Move)` — Updates the board, toggles active color, updates castling rights (revoke on king/rook move or rook capture), sets/clears en passant square, increments clocks. Handles special moves: castling (move rook too), en passant (remove captured pawn), promotion (replace pawn).
- [x] `is_in_check()` — Is the active color's king attacked?
- [x] `is_checkmate()` — In check and no legal moves.
- [x] `is_stalemate()` — Not in check and no legal moves.
- [x] `is_draw()` — Fifty-move rule (halfmove clock ≥ 100). Insufficient material detection (K vs K, K+B vs K, K+N vs K, K+B vs K+B same color bishops) as a stretch goal.
- [x] Tests — Apply sequences leading to Scholar's Mate (checkmate), smothered mate, back-rank mate. Stalemate position. Fifty-move draw. Castling rights revoked after king/rook moves. En passant square set after double pawn push and cleared next turn.

## Phase 7: Player Interface (`lib/chess/player.hpp`)

Abstract player interface for decoupling game logic from input source.

- [ ] `Player` abstract class — `virtual Move choose_move(const GameState&) = 0`. Virtual destructor.
- [ ] `RandomPlayer` — Picks a uniformly random legal move. Seeds `std::mt19937` from `std::random_device`.
- [ ] Tests — `RandomPlayer` always returns a move that is in the legal move list. Given the same state repeatedly, it produces valid moves each time. With a seeded RNG, output is deterministic.

## Phase 8: CI

- [ ] GitHub Actions workflow — `.github/workflows/ci.yml`: checkout, install deps, cmake configure/build, ctest. Runs on push and PR to main.
