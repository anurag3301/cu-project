#pragma once

#include <optional>

#include "Player.hpp"

namespace checkers {

class ComputerPlayer : public Player {
public:
    explicit ComputerPlayer(int level);

    bool handlesClickInput() const override;

    std::optional<Move> onSquareSelected(
        const Board& board,
        PlayerColor color,
        std::optional<Position> square) override;

    std::optional<Move> chooseMove(const Board& board, PlayerColor color) override;

    std::optional<Position> selectedSquare() const override;

private:
    int level_;
};

}  // namespace checkers
