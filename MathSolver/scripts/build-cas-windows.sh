#!/usr/bin/env bash
# Run in MSYS2 UCRT64. Installs/builds no Python runtime.
set -euo pipefail
cd -- "$(dirname -- "$0")/.."
if [[ ${MSYSTEM:-} != UCRT64 ]]; then
  echo 'Use the MSYS2 UCRT64 terminal.' >&2
  exit 1
fi
pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-libqalculate
cmake -S . -B build-cas-windows -G Ninja -DMATHSOLVER_GUI=OFF -DMATHSOLVER_BACKEND=LINKED -DCMAKE_PREFIX_PATH=/ucrt64 -DCMAKE_BUILD_TYPE=Release
cmake --build build-cas-windows -j 4
ctest --test-dir build-cas-windows --output-on-failure
stage="$PWD/build-cas-windows/stage"
mkdir -p "$stage" "$stage/definitions" "$stage/licenses"
cp build-cas-windows/mathsolver_cas_worker.exe "$stage/"
# Recursively collect only the DLLs actually imported by our native CAS helper.
queue=("$stage/mathsolver_cas_worker.exe")
for ((i=0;i<${#queue[@]};++i)); do
  while IFS= read -r dll; do
    if [[ -f /ucrt64/bin/$dll && ! -f $stage/$dll ]]; then
      cp "/ucrt64/bin/$dll" "$stage/"
      queue+=("$stage/$dll")
    fi
  done < <(objdump -p "${queue[i]}" | sed -n 's/^[[:space:]]*DLL Name: //p' | tr -d '\r')
done
cp /ucrt64/share/qalculate/*.xml "$stage/definitions/"
cp -R /ucrt64/share/licenses/libqalculate "$stage/licenses/"
echo "CAS helper staged in $stage. Next run scripts/build-windows.ps1 in a VS Developer PowerShell."
cp -R /ucrt64/share/licenses/. "$stage/licenses/"
pacman -Q > "$stage/installed-build-packages.txt"
