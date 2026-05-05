#pragma once

#include <optional>

#include "game/Board.hpp"

namespace checkers {

class Player {
public:
    virtual ~Player() = default;

    virtual bool handlesClickInput() const = 0;

    virtual std::optional<Move> onSquareSelected(
        const Board& board,
        PlayerColor color,
        std::optional<Position> square) = 0;

    virtual std::optional<Move> chooseMove(const Board& board, PlayerColor color) = 0;

    virtual std::optional<Position> selectedSquare() const = 0;

    virtual void onTurnStarted() {}
    virtual void onTurnEnded() {}
};

}  // namespace checkers
