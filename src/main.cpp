#include "raylib.h"

int main() {
    const int screenWidth = 960;
    const int screenHeight = 960;

    InitWindow(screenWidth, screenHeight, "Checkers");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Checkers scaffold", 20, 20, 30, BLACK);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
