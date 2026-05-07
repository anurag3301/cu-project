#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "raylib.h"

#include "game/Game.hpp"
#include "players/ComputerPlayer.hpp"
#include "players/HumanPlayer.hpp"

namespace {

constexpr int kWindowSize = 960;
constexpr int kBoardPixels = 800;
constexpr int kSquareSize = kBoardPixels / 8;
constexpr int kBoardOffsetX = (kWindowSize - kBoardPixels) / 2;
constexpr int kBoardOffsetY = 110;

enum class OpponentMode {
    Human,
    Computer
};

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

bool buttonClicked(const Rectangle& rect, Vector2 point, bool pressed) {
    return pressed && CheckCollisionPointRec(point, rect);
}

void drawButton(const Rectangle& rect, const char* text, bool selected) {
    const Color bg = selected ? Color{48, 130, 95, 255} : Color{85, 85, 85, 255};
    DrawRectangleRounded(rect, 0.2f, 8, bg);
    DrawRectangleRoundedLinesEx(rect, 0.2f, 8, 2.0f, BLACK);
    const int fontSize = 28;
    const int textWidth = MeasureText(text, fontSize);
    DrawText(text, static_cast<int>(rect.x + (rect.width - textWidth) / 2), static_cast<int>(rect.y + (rect.height - fontSize) / 2), fontSize, RAYWHITE);
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

void drawStartMenu(OpponentMode mode, int level) {
    DrawText("Checkers", 350, 70, 64, BLACK);
    DrawText("Select mode", 390, 180, 34, DARKGRAY);

    Rectangle humanVsHuman{260, 250, 200, 70};
    Rectangle humanVsComputer{500, 250, 200, 70};

    drawButton(humanVsHuman, "Human", mode == OpponentMode::Human);
    drawButton(humanVsComputer, "Computer", mode == OpponentMode::Computer);

    if (mode == OpponentMode::Computer) {
        DrawText("Computer level", 360, 360, 34, DARKGRAY);
        for (int i = 1; i <= 5; ++i) {
            Rectangle levelButton{180.0f + (i - 1) * 120.0f, 420.0f, 90.0f, 64.0f};
            std::string text = "L" + std::to_string(i);
            drawButton(levelButton, text.c_str(), i == level);
        }
        DrawText("L1 = Random, L2-L5 = increasing MCTS depth", 250, 510, 26, GRAY);
    }

    Rectangle startButton{345, 650, 270, 90};
    drawButton(startButton, "Start", true);
}

}  // namespace

int main() {
    InitWindow(kWindowSize, kWindowSize, "Checkers");
    SetTargetFPS(60);

    checkers::Game game;

    OpponentMode mode = OpponentMode::Human;
    int level = 2;
    bool gameStarted = false;

    std::unique_ptr<Player> redPlayer;
    std::unique_ptr<Player> blackPlayer;

    auto setupPlayers = [&]() {
        redPlayer = std::make_unique<checkers::HumanPlayer>();
        if (mode == OpponentMode::Human) {
            blackPlayer = std::make_unique<checkers::HumanPlayer>();
        } else {
            blackPlayer = std::make_unique<checkers::ComputerPlayer>(level);
        }

        game.reset();
        redPlayer->onTurnStarted();
        blackPlayer->onTurnEnded();
    };

    auto currentPlayer = [&]() -> Player* {
        if (game.currentTurn() == PlayerColor::Red) {
            return redPlayer.get();
        }
        return blackPlayer.get();
    };

    auto applyMoveAndAdvance = [&](const Move& move) {
        Player* active = currentPlayer();
        if (game.tryApplyMove(move)) {
            active->onTurnEnded();
            currentPlayer()->onTurnStarted();
        }
    };

    while (!WindowShouldClose()) {
        const Vector2 mousePos = GetMousePosition();
        const bool mousePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

        if (!gameStarted) {
            Rectangle humanVsHuman{260, 250, 200, 70};
            Rectangle humanVsComputer{500, 250, 200, 70};
            Rectangle startButton{345, 650, 270, 90};

            if (buttonClicked(humanVsHuman, mousePos, mousePressed)) {
                mode = OpponentMode::Human;
            }
            if (buttonClicked(humanVsComputer, mousePos, mousePressed)) {
                mode = OpponentMode::Computer;
            }

            if (mode == OpponentMode::Computer) {
                for (int i = 1; i <= 5; ++i) {
                    Rectangle levelButton{180.0f + (i - 1) * 120.0f, 420.0f, 90.0f, 64.0f};
                    if (buttonClicked(levelButton, mousePos, mousePressed)) {
                        level = i;
                    }
                }
            }

            if (buttonClicked(startButton, mousePos, mousePressed)) {
                setupPlayers();
                gameStarted = true;
            }
        } else {
            if (IsKeyPressed(KEY_R)) {
                setupPlayers();
            }

            if (!game.winner().has_value()) {
                Player* active = currentPlayer();

                if (active->handlesClickInput()) {
                    if (mousePressed) {
                        const auto clickedSquare = mouseToSquare(mousePos);
                        auto chosenMove = active->onSquareSelected(game.board(), game.currentTurn(), clickedSquare);
                        if (chosenMove.has_value()) {
                            applyMoveAndAdvance(*chosenMove);
                        }
                    }
                } else {
                    auto chosenMove = active->chooseMove(game.board(), game.currentTurn());
                    if (chosenMove.has_value()) {
                        applyMoveAndAdvance(*chosenMove);
                    }
                }
            }
        }

        BeginDrawing();
        ClearBackground(Color{245, 245, 245, 255});

        if (!gameStarted) {
            drawStartMenu(mode, level);
        } else {
            Player* active = currentPlayer();
            const auto selected = active->selectedSquare();
            const auto selectedMoves = selected.has_value()
                ? game.board().legalMovesForPiece(game.currentTurn(), *selected)
                : std::vector<Move>{};

            drawBoard(game.board(), selected, selectedMoves);

            DrawText("Checkers", 24, 22, 42, BLACK);

            std::string status = game.winner().has_value()
                ? (std::string("Winner: ") + colorName(*game.winner()))
                : (std::string("Turn: ") + colorName(game.currentTurn()));
            DrawText(status.c_str(), 24, 68, 28, DARKGRAY);

            std::string footer = mode == OpponentMode::Computer
                ? ("Red: Human  Black: Computer L" + std::to_string(level) + "  (R to restart)")
                : "Red: Human  Black: Human  (R to restart)";
            DrawText(footer.c_str(), 24, 915, 20, GRAY);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
