param(
 [Parameter(Mandatory=$true)][string]$QtPrefix,
 [string]$CasDirectory = "",
 [switch]$SkipTests
)
$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")
if (!$CasDirectory) { $CasDirectory = Join-Path $PWD "build-cas-windows/stage" }
if (!(Test-Path (Join-Path $CasDirectory "mathsolver_cas_worker.exe"))) { throw "First run scripts/build-cas-windows.sh in MSYS2 UCRT64." }
if (!(Test-Path (Join-Path $QtPrefix "lib/cmake/Qt6WebEngineWidgets"))) { throw "Install Qt 6 MSVC 2022 x64 with Qt WebEngine and Concurrent." }
# Run in a Visual Studio 2022 Developer PowerShell, never a MinGW Qt shell.
& cmake -S . -B build-windows -G "Visual Studio 17 2022" -A x64 "-DCMAKE_PREFIX_PATH=$QtPrefix" -DMATHSOLVER_BACKEND=PROCESS "-DMATHSOLVER_CAS_DIRECTORY=$CasDirectory"
if ($LASTEXITCODE) { throw "CMake configure failed" }
& cmake --build build-windows --config Release --parallel 4
if ($LASTEXITCODE) { throw "Build failed" }
if (!$SkipTests) {
 & ctest --test-dir build-windows -C Release --output-on-failure
 if ($LASTEXITCODE) { throw "Tests failed" }
}
& cmake --install build-windows --config Release --prefix releases/windows
if ($LASTEXITCODE) { throw "Qt deployment failed" }
Compress-Archive -Path releases/windows/* -DestinationPath releases/Infinity-Windows-x64.zip -Force
Write-Host "Created releases/Infinity-Windows-x64.zip"
