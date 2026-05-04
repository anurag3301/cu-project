#pragma once

#include <optional>
#include <vector>

#include "Types.hpp"

namespace checkers {

class Board {
public:
    Board();

    void reset();
    bool isInside(Position pos) const;
    bool isPlayableSquare(Position pos) const;
    std::optional<Piece> pieceAt(Position pos) const;

    std::vector<Move> legalMoves(PlayerColor color) const;
    std::vector<Move> legalMovesForPiece(PlayerColor color, Position from) const;
    bool applyMove(PlayerColor color, const Move& move);

    bool hasAnyPieces(PlayerColor color) const;
    int pieceCount(PlayerColor color) const;
    bool hasAnyMoves(PlayerColor color) const;
    std::optional<PlayerColor> winner() const;

private:
    std::optional<Piece> grid_[8][8];

    std::vector<Move> pseudoMovesForPiece(Position from) const;
    std::vector<Position> stepDirections(const Piece& piece) const;
};

}  // namespace checkers
