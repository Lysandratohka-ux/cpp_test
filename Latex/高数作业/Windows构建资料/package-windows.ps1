param(
    [Parameter(Mandatory=$true)][string]$QtPrefix,
    [string]$CasDirectory = "",
    [string]$IsccPath = ""
)
$ErrorActionPreference = "Stop"
if ($env:OS -ne "Windows_NT") { throw "A native Windows build environment is required." }
$projectRoot = Split-Path $PSScriptRoot -Parent
Set-Location $projectRoot
# No skip-tests option: an installer must only wrap a tested native deployment.
& (Join-Path $PSScriptRoot "build-windows.ps1") -QtPrefix $QtPrefix -CasDirectory $CasDirectory
$staging = Join-Path $projectRoot "releases/windows"
# Prefer app-local redistributable CRT DLLs from the licensed Visual Studio tools.
if ($env:VCToolsRedistDir) {
    $crtPattern = Join-Path $env:VCToolsRedistDir "x64/Microsoft.VC*.CRT/*.dll"
    Get-ChildItem -Path $crtPattern -ErrorAction SilentlyContinue | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination $staging -Force
    }
}
foreach ($required in @("Infinity.exe", "Qt6Core.dll", "QtWebEngineProcess.exe", "platforms/qwindows.dll", "cas/mathsolver_cas_worker.exe", "msvcp140.dll", "vcruntime140.dll")) {
    if (!(Test-Path (Join-Path $staging $required))) {
        throw "Deployment incomplete: $required. Do not distribute an installer until dependencies are present."
    }
}
if (!$IsccPath) {
    $IsccPath = Join-Path ${env:ProgramFiles(x86)} "Inno Setup 6/ISCC.exe"
}
if (!(Test-Path $IsccPath)) { throw "Install Inno Setup 6 or supply -IsccPath." }
$projectText = Get-Content (Join-Path $projectRoot "CMakeLists.txt") -Raw
$match = [regex]::Match($projectText, 'project\(MathSolver VERSION ([0-9.]+)')
if (!$match.Success) { throw "Cannot determine the CMake project version." }
$version = $match.Groups[1].Value
$outputDir = Join-Path $projectRoot "releases"
& $IsccPath "/DAppVersion=$version" "/DBuildDir=$staging" "/DOutputDir=$outputDir" (Join-Path $PSScriptRoot "Infinity.iss")
if ($LASTEXITCODE) { throw "Installer compilation failed." }
$installer = Join-Path $outputDir "Infinity-$version-Windows-x64-Setup.exe"
if (!(Test-Path $installer)) { throw "Installer output was not created." }
Get-FileHash -Algorithm SHA256 $installer
Write-Host "Created $installer. Test installation, formulas and uninstall on a clean Windows machine before release."
