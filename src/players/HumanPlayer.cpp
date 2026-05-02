#include "HumanPlayer.hpp"

namespace checkers {

std::optional<Move> HumanPlayer::onSquareSelected(
    const Board& board,
    PlayerColor color,
    std::optional<Position> square) {
    if (!square.has_value()) {
        return std::nullopt;
    }

    const Position clicked = *square;

    if (selectedSquare_.has_value() && clicked == *selectedSquare_) {
        selectedSquare_ = std::nullopt;
        return std::nullopt;
    }

    if (selectedSquare_.has_value()) {
        const auto selectedMoves = board.legalMovesForPiece(color, *selectedSquare_);
        for (const auto& move : selectedMoves) {
            if (move.to == clicked) {
                selectedSquare_ = std::nullopt;
                return move;
            }
        }
    }

    const auto clickedPiece = board.pieceAt(clicked);
    if (clickedPiece.has_value() && clickedPiece->color == color) {
        const auto clickedMoves = board.legalMovesForPiece(color, clicked);
        if (!clickedMoves.empty()) {
            selectedSquare_ = clicked;
            return std::nullopt;
        }
    }

    selectedSquare_ = std::nullopt;
    return std::nullopt;
}

std::optional<Position> HumanPlayer::selectedSquare() const {
    return selectedSquare_;
}

void HumanPlayer::onTurnEnded() {
    selectedSquare_ = std::nullopt;
}

}  // namespace checkers
