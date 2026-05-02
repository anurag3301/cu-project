#pragma once

#include <optional>

#include "Player.hpp"

namespace checkers {

class HumanPlayer : public Player {
public:
    std::optional<Move> onSquareSelected(
        const Board& board,
        PlayerColor color,
        std::optional<Position> square) override;

    std::optional<Position> selectedSquare() const override;

    void onTurnEnded() override;

private:
    std::optional<Position> selectedSquare_;
};

}  // namespace checkers
