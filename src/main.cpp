#include <array>
#include <optional>
#include <string>
#include <vector>

#include "raylib.h"

#include "game/Game.hpp"
#include "players/HumanPlayer.hpp"

namespace {

constexpr int kWindowSize = 960;
constexpr int kBoardPixels = 800;
constexpr int kSquareSize = kBoardPixels / 8;
constexpr int kBoardOffsetX = (kWindowSize - kBoardPixels) / 2;
constexpr int kBoardOffsetY = 110;

using checkers::Board;
using checkers::Move;
using checkers::Player;
using checkers::PlayerColor;
using checkers::Position;

Color playerColorToDrawColor(PlayerColor color) {
    return color == PlayerColor::Red ? Color{190, 30, 45, 255} : Color{25, 25, 25, 255};
}

const char* colorName(PlayerColor color) {
    return color == PlayerColor::Red ? "Red" : "Black";
}

std::optional<Position> mouseToSquare(Vector2 mousePos) {
    if (mousePos.x < kBoardOffsetX || mousePos.y < kBoardOffsetY) {
        return std::nullopt;
    }

    const int col = static_cast<int>((mousePos.x - kBoardOffsetX) / kSquareSize);
    const int row = static_cast<int>((mousePos.y - kBoardOffsetY) / kSquareSize);

    if (row < 0 || row >= 8 || col < 0 || col >= 8) {
        return std::nullopt;
    }

    return Position{row, col};
}

void drawBoard(const Board& board, std::optional<Position> selected, const std::vector<Move>& selectedMoves) {
    const Color lightSquare{238, 213, 183, 255};
    const Color darkSquare{120, 76, 48, 255};

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            Rectangle square{
                static_cast<float>(kBoardOffsetX + col * kSquareSize),
                static_cast<float>(kBoardOffsetY + row * kSquareSize),
                static_cast<float>(kSquareSize),
                static_cast<float>(kSquareSize)};

            DrawRectangleRec(square, ((row + col) % 2 == 0) ? lightSquare : darkSquare);

            Position pos{row, col};
            if (selected.has_value() && pos == *selected) {
                DrawRectangleLinesEx(square, 3.0f, GOLD);
            }

            for (const auto& move : selectedMoves) {
                if (move.to == pos) {
                    DrawCircle(
                        kBoardOffsetX + col * kSquareSize + (kSquareSize / 2),
                        kBoardOffsetY + row * kSquareSize + (kSquareSize / 2),
                        10.0f,
                        GREEN);
                }
            }

            const auto piece = board.pieceAt(pos);
            if (!piece.has_value()) {
                continue;
            }

            const int centerX = kBoardOffsetX + col * kSquareSize + (kSquareSize / 2);
            const int centerY = kBoardOffsetY + row * kSquareSize + (kSquareSize / 2);

            DrawCircle(centerX, centerY, 38.0f, playerColorToDrawColor(piece->color));
            DrawCircleLines(centerX, centerY, 38.0f, RAYWHITE);

            if (piece->king) {
                DrawText("K", centerX - 9, centerY - 14, 28, GOLD);
            }
        }
    }
}

}  // namespace

int main() {
    InitWindow(kWindowSize, kWindowSize, "Checkers");
    SetTargetFPS(60);

    checkers::Game game;
    checkers::HumanPlayer redPlayer;
    checkers::HumanPlayer blackPlayer;

    std::array<Player*, 2> players = {&redPlayer, &blackPlayer};

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_R)) {
            game.reset();
            redPlayer.onTurnEnded();
            blackPlayer.onTurnEnded();
        }

        if (!game.winner().has_value() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            const auto clickedSquare = mouseToSquare(GetMousePosition());
            const PlayerColor turn = game.currentTurn();
            Player* currentPlayer = players[turn == PlayerColor::Red ? 0 : 1];

            auto chosenMove = currentPlayer->onSquareSelected(game.board(), turn, clickedSquare);
            if (chosenMove.has_value()) {
                if (game.tryApplyMove(*chosenMove)) {
                    currentPlayer->onTurnEnded();
                }
            }
        }

        const PlayerColor turn = game.currentTurn();
        Player* currentPlayer = players[turn == PlayerColor::Red ? 0 : 1];
        const auto selected = currentPlayer->selectedSquare();
        const auto selectedMoves = selected.has_value()
            ? game.board().legalMovesForPiece(turn, *selected)
            : std::vector<Move>{};

        BeginDrawing();
        ClearBackground(Color{245, 245, 245, 255});

        drawBoard(game.board(), selected, selectedMoves);

        DrawText("Checkers", 24, 22, 42, BLACK);

        std::string status = game.winner().has_value()
            ? (std::string("Winner: ") + colorName(*game.winner()))
            : (std::string("Turn: ") + colorName(game.currentTurn()));
        DrawText(status.c_str(), 24, 68, 28, DARKGRAY);

        DrawText("Click a piece, then click destination. Press R to reset.", 24, 915, 20, GRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
