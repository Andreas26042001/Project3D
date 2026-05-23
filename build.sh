#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

JOBS="$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)"

echo "→ CMake configuration…"
cmake -S . -B build

echo "→ Building…"
cmake --build build --parallel "$JOBS"

echo ""
echo "Build OK — run the game with: ./run.sh  or  ./bin/SpacePuzzle"
