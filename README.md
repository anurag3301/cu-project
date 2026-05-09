# Checkers (Raylib + CMake)

Checkers implementation using Raylib and CMake.

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

```bash
./build/checkers
```

## Gameplay

- Start screen lets you choose `Human` or `Computer` mode.
- In `Computer` mode, choose level `L1` to `L5` and click `Start`.
- `L1` plays random legal moves.
- `L2` to `L5` use threaded MCTS with increasing iteration budgets.
- Press `R` during a match to restart with current settings.
