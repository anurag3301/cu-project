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

enum class PlayerKind {
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

const char* kindName(PlayerKind kind) {
    return kind == PlayerKind::Human ? "Human" : "Computer";
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

void drawSideConfig(int y, const char* label, PlayerKind kind, int level) {
    DrawText(label, 100, y, 34, DARKGRAY);

    Rectangle humanButton{270.0f, static_cast<float>(y - 6), 180.0f, 60.0f};
    Rectangle computerButton{470.0f, static_cast<float>(y - 6), 220.0f, 60.0f};

    drawButton(humanButton, "Human", kind == PlayerKind::Human);
    drawButton(computerButton, "Computer", kind == PlayerKind::Computer);

    if (kind == PlayerKind::Computer) {
        for (int i = 1; i <= 5; ++i) {
            Rectangle levelButton{720.0f + (i - 1) * 44.0f, static_cast<float>(y), 38.0f, 42.0f};
            std::string text = std::to_string(i);
            drawButton(levelButton, text.c_str(), i == level);
        }
    }
}

void drawStartMenu(PlayerKind redKind, int redLevel, PlayerKind blackKind, int blackLevel) {
    DrawText("Checkers", 350, 70, 64, BLACK);
    DrawText("Choose each side", 345, 160, 36, DARKGRAY);

    drawSideConfig(260, "Red", redKind, redLevel);
    drawSideConfig(360, "Black", blackKind, blackLevel);

    DrawText("For computer sides: level 1 is random, level 2-5 use deeper MCTS", 120, 470, 28, GRAY);

    Rectangle startButton{345, 650, 270, 90};
    drawButton(startButton, "Start", true);
}

void drawGameOverOverlay(PlayerColor winner) {
    Rectangle panel{230.0f, 280.0f, 500.0f, 300.0f};
    DrawRectangle(0, 0, kWindowSize, kWindowSize, Color{0, 0, 0, 120});
    DrawRectangleRounded(panel, 0.15f, 10, Color{245, 245, 245, 255});
    DrawRectangleRoundedLinesEx(panel, 0.15f, 10, 2.0f, BLACK);

    DrawText("Game Over", 380, 330, 44, BLACK);

    std::string winnerText = std::string("Winner: ") + colorName(winner);
    DrawText(winnerText.c_str(), 345, 390, 34, DARKGRAY);

    Rectangle menuButton{355.0f, 470.0f, 250.0f, 74.0f};
    drawButton(menuButton, "Main Menu", true);
}

}  // namespace

int main() {
    InitWindow(kWindowSize, kWindowSize, "Checkers");
    SetTargetFPS(60);

    checkers::Game game;

    PlayerKind redKind = PlayerKind::Human;
    PlayerKind blackKind = PlayerKind::Computer;
    int redLevel = 2;
    int blackLevel = 2;

    bool gameStarted = false;
    double nextAiMoveTime = 0.0;

    std::unique_ptr<Player> redPlayer;
    std::unique_ptr<Player> blackPlayer;

    auto setupPlayers = [&]() {
        if (redKind == PlayerKind::Human) {
            redPlayer = std::make_unique<checkers::HumanPlayer>();
        } else {
            redPlayer = std::make_unique<checkers::ComputerPlayer>(redLevel);
        }

        if (blackKind == PlayerKind::Human) {
            blackPlayer = std::make_unique<checkers::HumanPlayer>();
        } else {
            blackPlayer = std::make_unique<checkers::ComputerPlayer>(blackLevel);
        }

        game.reset();
        redPlayer->onTurnStarted();
        blackPlayer->onTurnEnded();
        nextAiMoveTime = GetTime();
    };

    auto currentPlayer = [&]() -> Player* {
        if (game.currentTurn() == PlayerColor::Red) {
            return redPlayer.get();
        }
        return blackPlayer.get();
    };

    auto bothComputer = [&]() {
        return redKind == PlayerKind::Computer && blackKind == PlayerKind::Computer;
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
            Rectangle redHuman{270, 254, 180, 60};
            Rectangle redComputer{470, 254, 220, 60};
            Rectangle blackHuman{270, 354, 180, 60};
            Rectangle blackComputer{470, 354, 220, 60};
            Rectangle startButton{345, 650, 270, 90};

            if (buttonClicked(redHuman, mousePos, mousePressed)) {
                redKind = PlayerKind::Human;
            }
            if (buttonClicked(redComputer, mousePos, mousePressed)) {
                redKind = PlayerKind::Computer;
            }
            if (buttonClicked(blackHuman, mousePos, mousePressed)) {
                blackKind = PlayerKind::Human;
            }
            if (buttonClicked(blackComputer, mousePos, mousePressed)) {
                blackKind = PlayerKind::Computer;
            }

            if (redKind == PlayerKind::Computer) {
                for (int i = 1; i <= 5; ++i) {
                    Rectangle levelButton{720.0f + (i - 1) * 44.0f, 260.0f, 38.0f, 42.0f};
                    if (buttonClicked(levelButton, mousePos, mousePressed)) {
                        redLevel = i;
                    }
                }
            }

            if (blackKind == PlayerKind::Computer) {
                for (int i = 1; i <= 5; ++i) {
                    Rectangle levelButton{720.0f + (i - 1) * 44.0f, 360.0f, 38.0f, 42.0f};
                    if (buttonClicked(levelButton, mousePos, mousePressed)) {
                        blackLevel = i;
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

            Rectangle menuButton{355.0f, 470.0f, 250.0f, 74.0f};
            if (game.winner().has_value() && buttonClicked(menuButton, mousePos, mousePressed)) {
                gameStarted = false;
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
                    if (!bothComputer() || GetTime() >= nextAiMoveTime) {
                        const double computeStart = GetTime();
                        auto chosenMove = active->chooseMove(game.board(), game.currentTurn());
                        const double computeElapsed = GetTime() - computeStart;
                        if (bothComputer()) {
                            const double remainingWait = 0.5 - computeElapsed;
                            nextAiMoveTime = GetTime() + (remainingWait > 0.0 ? remainingWait : 0.0);
                        }
                        if (chosenMove.has_value()) {
                            applyMoveAndAdvance(*chosenMove);
                        }
                    }
                }
            }
        }

        BeginDrawing();
        ClearBackground(Color{245, 245, 245, 255});

        if (!gameStarted) {
            drawStartMenu(redKind, redLevel, blackKind, blackLevel);
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

            std::string redSide = std::string("Red: ") + kindName(redKind) +
                (redKind == PlayerKind::Computer ? (" L" + std::to_string(redLevel)) : "");
            std::string blackSide = std::string("Black: ") + kindName(blackKind) +
                (blackKind == PlayerKind::Computer ? (" L" + std::to_string(blackLevel)) : "");
            std::string footer = redSide + "  " + blackSide + "  (R to restart)";

            DrawText(footer.c_str(), 24, 915, 20, GRAY);

            if (game.winner().has_value()) {
                drawGameOverOverlay(*game.winner());
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
