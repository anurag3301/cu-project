#pragma once

#include <optional>

#include "Board.hpp"

namespace checkers {

class Game {
public:
    Game();

    void reset();

    const Board& board() const;
    PlayerColor currentTurn() const;
    std::optional<PlayerColor> winner() const;

    bool tryApplyMove(const Move& move);

private:
    Board board_;
    PlayerColor currentTurn_;
    std::optional<PlayerColor> winner_;

    void advanceTurn();
};

}  // namespace checkers
