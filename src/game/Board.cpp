#include "Board.hpp"

#include <algorithm>

namespace checkers {
namespace {

PlayerColor opposite(PlayerColor color) {
    return color == PlayerColor::Red ? PlayerColor::Black : PlayerColor::Red;
}

bool containsMove(const std::vector<Move>& moves, const Move& candidate) {
    return std::any_of(moves.begin(), moves.end(), [&](const Move& move) {
        return move.from == candidate.from && move.to == candidate.to;
    });
}

}  // namespace

Board::Board() {
    reset();
}

void Board::reset() {
    for (auto& row : grid_) {
        for (auto& cell : row) {
            cell = std::nullopt;
        }
    }

    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 8; ++col) {
            Position pos{row, col};
            if (isPlayableSquare(pos)) {
                grid_[row][col] = Piece{PlayerColor::Black, false};
            }
        }
    }

    for (int row = 5; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            Position pos{row, col};
            if (isPlayableSquare(pos)) {
                grid_[row][col] = Piece{PlayerColor::Red, false};
            }
        }
    }
}

bool Board::isInside(Position pos) const {
    return pos.row >= 0 && pos.row < 8 && pos.col >= 0 && pos.col < 8;
}

bool Board::isPlayableSquare(Position pos) const {
    return ((pos.row + pos.col) % 2) == 1;
}

std::optional<Piece> Board::pieceAt(Position pos) const {
    if (!isInside(pos)) {
        return std::nullopt;
    }
    return grid_[pos.row][pos.col];
}

std::vector<Position> Board::stepDirections(const Piece& piece) const {
    if (piece.king) {
        return {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    }
    if (piece.color == PlayerColor::Red) {
        return {{-1, -1}, {-1, 1}};
    }
    return {{1, -1}, {1, 1}};
}

std::vector<Move> Board::pseudoMovesForPiece(Position from) const {
    std::vector<Move> moves;
    auto piece = pieceAt(from);
    if (!piece.has_value()) {
        return moves;
    }

    const auto directions = stepDirections(*piece);
    for (const auto& direction : directions) {
        Position step{from.row + direction.row, from.col + direction.col};
        Position jump{from.row + (2 * direction.row), from.col + (2 * direction.col)};

        if (isInside(step) && isPlayableSquare(step)) {
            auto stepPiece = pieceAt(step);
            if (!stepPiece.has_value()) {
                moves.push_back(Move{from, step, false, std::nullopt});
            } else if (stepPiece->color == opposite(piece->color) && isInside(jump) && isPlayableSquare(jump) && !pieceAt(jump).has_value()) {
                moves.push_back(Move{from, jump, true, step});
            }
        }
    }

    return moves;
}

std::vector<Move> Board::legalMoves(PlayerColor color) const {
    std::vector<Move> allMoves;
    bool hasCapture = false;

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            Position pos{row, col};
            auto piece = pieceAt(pos);
            if (!piece.has_value() || piece->color != color) {
                continue;
            }
            auto pieceMoves = pseudoMovesForPiece(pos);
            for (const auto& move : pieceMoves) {
                hasCapture = hasCapture || move.isCapture;
                allMoves.push_back(move);
            }
        }
    }

    if (!hasCapture) {
        return allMoves;
    }

    std::vector<Move> captureMoves;
    for (const auto& move : allMoves) {
        if (move.isCapture) {
            captureMoves.push_back(move);
        }
    }
    return captureMoves;
}

std::vector<Move> Board::legalMovesForPiece(PlayerColor color, Position from) const {
    std::vector<Move> moves;
    const auto allMoves = legalMoves(color);
    for (const auto& move : allMoves) {
        if (move.from == from) {
            moves.push_back(move);
        }
    }
    return moves;
}

bool Board::applyMove(PlayerColor color, const Move& move) {
    const auto validMoves = legalMoves(color);
    if (!containsMove(validMoves, move)) {
        return false;
    }

    auto piece = pieceAt(move.from);
    if (!piece.has_value() || piece->color != color) {
        return false;
    }

    grid_[move.from.row][move.from.col] = std::nullopt;
    if (move.isCapture && move.captured.has_value()) {
        grid_[move.captured->row][move.captured->col] = std::nullopt;
    }

    Piece moved = *piece;
    if (moved.color == PlayerColor::Red && move.to.row == 0) {
        moved.king = true;
    }
    if (moved.color == PlayerColor::Black && move.to.row == 7) {
        moved.king = true;
    }
    grid_[move.to.row][move.to.col] = moved;

    return true;
}

bool Board::hasAnyPieces(PlayerColor color) const {
    for (const auto& row : grid_) {
        for (const auto& cell : row) {
            if (cell.has_value() && cell->color == color) {
                return true;
            }
        }
    }
    return false;
}

bool Board::hasAnyMoves(PlayerColor color) const {
    return !legalMoves(color).empty();
}

std::optional<PlayerColor> Board::winner() const {
    const bool redPieces = hasAnyPieces(PlayerColor::Red);
    const bool blackPieces = hasAnyPieces(PlayerColor::Black);
    const bool redMoves = hasAnyMoves(PlayerColor::Red);
    const bool blackMoves = hasAnyMoves(PlayerColor::Black);

    if ((!redPieces || !redMoves) && blackPieces && blackMoves) {
        return PlayerColor::Black;
    }
    if ((!blackPieces || !blackMoves) && redPieces && redMoves) {
        return PlayerColor::Red;
    }
    return std::nullopt;
}

}  // namespace checkers
