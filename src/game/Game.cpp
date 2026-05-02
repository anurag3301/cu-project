#include "Game.hpp"

namespace checkers {

Game::Game() {
    reset();
}

void Game::reset() {
    board_.reset();
    currentTurn_ = PlayerColor::Red;
    winner_ = std::nullopt;
}

const Board& Game::board() const {
    return board_;
}

PlayerColor Game::currentTurn() const {
    return currentTurn_;
}

std::optional<PlayerColor> Game::winner() const {
    return winner_;
}

void Game::advanceTurn() {
    currentTurn_ = currentTurn_ == PlayerColor::Red ? PlayerColor::Black : PlayerColor::Red;
}

bool Game::tryApplyMove(const Move& move) {
    if (winner_.has_value()) {
        return false;
    }

    if (!board_.applyMove(currentTurn_, move)) {
        return false;
    }

    advanceTurn();
    winner_ = board_.winner();
    return true;
}

}  // namespace checkers
