#include "ComputerPlayer.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <memory>
#include <optional>
#include <random>
#include <thread>
#include <vector>

namespace checkers {
namespace {

struct Node {
    Board board;
    PlayerColor toMove;
    std::optional<Move> moveFromParent;
    Node* parent;
    int visits = 0;
    double wins = 0.0;
    std::vector<Move> untried;
    std::vector<std::unique_ptr<Node>> children;

    Node(const Board& state, PlayerColor turn, std::optional<Move> move, Node* parentNode)
        : board(state), toMove(turn), moveFromParent(move), parent(parentNode), untried(state.legalMoves(turn)) {}
};

struct RootStat {
    Move move;
    int visits = 0;
    double wins = 0.0;
};

int levelIterations(int level) {
    const std::array<int, 5> budgets = {0, 1400, 3800, 8000, 16000};
    const int idx = std::clamp(level, 1, 5) - 1;
    return budgets[idx];
}

int evaluateDraw(const Board& board, PlayerColor rootColor) {
    const int rootPieces = board.pieceCount(rootColor);
    const int oppPieces = board.pieceCount(oppositeColor(rootColor));
    if (rootPieces > oppPieces) {
        return 1;
    }
    if (rootPieces < oppPieces) {
        return -1;
    }
    return 0;
}

Node* selectNode(Node* node, std::mt19937& rng) {
    constexpr double kExplore = 1.41421356;

    while (node->untried.empty() && !node->children.empty()) {
        Node* best = nullptr;
        double bestScore = -1e18;

        for (const auto& childPtr : node->children) {
            Node* child = childPtr.get();
            double score = 0.0;

            if (child->visits == 0) {
                score = 1e9;
            } else {
                const double exploit = child->wins / static_cast<double>(child->visits);
                const double explore = kExplore * std::sqrt(
                    std::log(static_cast<double>(node->visits) + 1.0) / static_cast<double>(child->visits));
                score = exploit + explore;
            }

            if (score > bestScore ||
                (score == bestScore && (rng() % 2 == 0))) {
                best = child;
                bestScore = score;
            }
        }

        if (best == nullptr) {
            break;
        }
        node = best;
    }

    return node;
}

Node* expandNode(Node* node, std::mt19937& rng) {
    if (node->untried.empty()) {
        return node;
    }

    std::uniform_int_distribution<size_t> pick(0, node->untried.size() - 1);
    const size_t index = pick(rng);
    const Move move = node->untried[index];

    node->untried[index] = node->untried.back();
    node->untried.pop_back();

    Board nextBoard = node->board;
    nextBoard.applyMove(node->toMove, move);

    auto child = std::make_unique<Node>(nextBoard, oppositeColor(node->toMove), move, node);
    Node* raw = child.get();
    node->children.push_back(std::move(child));
    return raw;
}

double rolloutResult(Board board, PlayerColor turn, PlayerColor rootColor, std::mt19937& rng) {
    constexpr int kMaxPlies = 120;

    for (int ply = 0; ply < kMaxPlies; ++ply) {
        auto w = board.winner();
        if (w.has_value()) {
            return *w == rootColor ? 1.0 : 0.0;
        }

        const auto moves = board.legalMoves(turn);
        if (moves.empty()) {
            return turn == rootColor ? 0.0 : 1.0;
        }

        std::uniform_int_distribution<size_t> pick(0, moves.size() - 1);
        const Move move = moves[pick(rng)];
        board.applyMove(turn, move);
        turn = oppositeColor(turn);
    }

    const int score = evaluateDraw(board, rootColor);
    if (score > 0) {
        return 1.0;
    }
    if (score < 0) {
        return 0.0;
    }
    return 0.5;
}

void backpropagate(Node* node, double reward) {
    while (node != nullptr) {
        node->visits++;
        node->wins += reward;
        node = node->parent;
    }
}

std::vector<RootStat> runWorkerTree(const Board& board, PlayerColor color, int iterations, unsigned int seed) {
    std::mt19937 rng(seed);
    Node root(board, color, std::nullopt, nullptr);

    for (int i = 0; i < iterations; ++i) {
        Node* selected = selectNode(&root, rng);
        if (!selected->untried.empty()) {
            selected = expandNode(selected, rng);
        }

        const double reward = rolloutResult(selected->board, selected->toMove, color, rng);
        backpropagate(selected, reward);
    }

    std::vector<RootStat> stats;
    stats.reserve(root.children.size());
    for (const auto& child : root.children) {
        if (child->moveFromParent.has_value()) {
            stats.push_back(RootStat{*child->moveFromParent, child->visits, child->wins});
        }
    }
    return stats;
}

bool sameMove(const Move& a, const Move& b) {
    return a.from == b.from && a.to == b.to;
}

std::optional<Move> chooseWithMcts(const Board& board, PlayerColor color, int level) {
    const auto legal = board.legalMoves(color);
    if (legal.empty()) {
        return std::nullopt;
    }

    const int totalIterations = levelIterations(level);
    if (totalIterations <= 0) {
        return legal.front();
    }

    const unsigned int hw = std::thread::hardware_concurrency();
    const unsigned int preferredThreads = hw == 0 ? 8u : (hw * 2u);
    const unsigned int threadCount = std::max(4u, std::min(32u, preferredThreads));
    const int baseIterations = totalIterations / static_cast<int>(threadCount);
    const int remainder = totalIterations % static_cast<int>(threadCount);

    std::vector<std::vector<RootStat>> threadStats(threadCount);
    std::vector<std::thread> workers;
    workers.reserve(threadCount);

    std::random_device rd;
    for (unsigned int t = 0; t < threadCount; ++t) {
        const int iterations = baseIterations + (static_cast<int>(t) < remainder ? 1 : 0);
        if (iterations <= 0) {
            continue;
        }
        const unsigned int seed = rd() ^ (static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count())) ^ (t * 911U);
        workers.emplace_back([&, t, seed, iterations]() {
            threadStats[t] = runWorkerTree(board, color, iterations, seed);
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    std::vector<RootStat> aggregate;
    aggregate.reserve(legal.size());

    for (const auto& move : legal) {
        aggregate.push_back(RootStat{move, 0, 0.0});
    }

    for (const auto& stats : threadStats) {
        for (const auto& stat : stats) {
            for (auto& total : aggregate) {
                if (sameMove(total.move, stat.move)) {
                    total.visits += stat.visits;
                    total.wins += stat.wins;
                    break;
                }
            }
        }
    }

    const RootStat* best = nullptr;
    for (const auto& stat : aggregate) {
        if (stat.visits == 0) {
            continue;
        }

        if (best == nullptr || stat.visits > best->visits) {
            best = &stat;
            continue;
        }

        if (best != nullptr && stat.visits == best->visits) {
            const double scoreA = stat.wins / static_cast<double>(stat.visits);
            const double scoreB = best->wins / static_cast<double>(best->visits);
            if (scoreA > scoreB) {
                best = &stat;
            }
        }
    }

    if (best == nullptr) {
        return legal.front();
    }
    return best->move;
}

}  // namespace

ComputerPlayer::ComputerPlayer(int level) : level_(std::clamp(level, 1, 5)) {}

bool ComputerPlayer::handlesClickInput() const {
    return false;
}

std::optional<Move> ComputerPlayer::onSquareSelected(
    const Board&,
    PlayerColor,
    std::optional<Position>) {
    return std::nullopt;
}

std::optional<Move> ComputerPlayer::chooseMove(const Board& board, PlayerColor color) {
    const auto legal = board.legalMoves(color);
    if (legal.empty()) {
        return std::nullopt;
    }

    if (level_ <= 1) {
        static thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<size_t> pick(0, legal.size() - 1);
        return legal[pick(rng)];
    }

    return chooseWithMcts(board, color, level_);
}

std::optional<Position> ComputerPlayer::selectedSquare() const {
    return std::nullopt;
}

}  // namespace checkers
