#!/bin/zsh
set -euo pipefail
cd -- "${0:A:h}/.."
mathsolver_qt_prefix="${MATHSOLVER_QT_PREFIX:-$PWD/.deps/qt}"
cmake -S . -B build -DCMAKE_PREFIX_PATH="$mathsolver_qt_prefix" -DCMAKE_BUILD_TYPE=Release -DMATHSOLVER_BACKEND=LINKED
cmake --build build -j 4
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$PWD/releases/macos"
echo "Application: $PWD/releases/macos/Infinity.app"
