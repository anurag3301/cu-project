# Checkers (Raylib + CMake)

A desktop checkers game written in C++17 using Raylib for rendering/input and CMake for builds.

<img width="1090" height="1118" alt="image" src="https://github.com/user-attachments/assets/0ed7e7af-3c4f-4757-8566-2ef92dff23b7" />


## What This Project Is

This repository contains a full checkers implementation with:
- Human vs Human play
- Human vs Computer play
- Computer vs Computer play
- Configurable AI difficulty for each side (levels 1-5)

The game starts with a setup menu where Red and Black can each be configured independently as `Human` or `Computer`.

## What It Does

- Draws and runs an 8x8 checkers board UI
- Enforces legal move rules with mandatory captures
- Handles king promotion
- Tracks turn order and game end state
- Shows game-over overlay with winner and a `Main Menu` button
- Supports AI with level-based behavior:
  - `L1`: random legal moves
  - `L2-L4`: mixed policy (random moves + MCTS)
  - `L5`: strongest current setting (MCTS only)

When both sides are computer players, moves are paced to ~0.5s per turn including compute time.

## How It Works

### Core game engine
- `src/game/Board.*` manages board state and legal move generation
- `src/game/Game.*` manages turn flow and winner detection
- `src/game/Types.hpp` defines shared game types (`Move`, `Position`, `PlayerColor`, etc.)

### Player system
- `src/players/Player.hpp` defines a base player interface
- `src/players/HumanPlayer.*` handles click-driven move selection
- `src/players/ComputerPlayer.*` handles AI move selection

### AI strategy (ComputerPlayer)
- Uses Monte Carlo Tree Search (MCTS) for search-based decision making
- Uses multithreading (`std::thread`) to run multiple independent trees and aggregate root statistics
- Difficulty scaling is done through shallower/deeper iteration budgets and controlled randomness at mid-levels

### Frontend/game loop
- `src/main.cpp` contains:
  - setup menu UI
  - board rendering
  - turn loop integration for Human/Computer combinations
  - game-over overlay + navigation back to menu

## Build

### Prerequisites
- CMake 3.16+
- C++17 compiler (GCC/Clang/MSVC with C++17 support)
- Internet access on first configure to fetch Raylib via CMake `FetchContent`

### Commands

```bash
cmake -S . -B build
cmake --build build -j$(nproc)
```

## Run

```bash
./build/checkers
```

## Controls

- In menu: click to configure Red/Black side type and AI levels, then click `Start`
- In-game: click piece then destination to move (for human-controlled side)
- `R`: restart match with current configuration
- On game over: click `Main Menu` to return to setup screen

## Extra utility

- `rewrite_commit_dates.sh` rewrites commit dates in a selected range and can also overwrite commit author/committer name and email.
