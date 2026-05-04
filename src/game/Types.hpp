#pragma once

#include <optional>

namespace checkers {

enum class PlayerColor {
    Red,
    Black
};

inline PlayerColor oppositeColor(PlayerColor color) {
    return color == PlayerColor::Red ? PlayerColor::Black : PlayerColor::Red;
}

struct Position {
    int row;
    int col;

    bool operator==(const Position& other) const {
        return row == other.row && col == other.col;
    }
};

struct Piece {
    PlayerColor color;
    bool king;
};

struct Move {
    Position from;
    Position to;
    bool isCapture;
    std::optional<Position> captured;
};

}  // namespace checkers
