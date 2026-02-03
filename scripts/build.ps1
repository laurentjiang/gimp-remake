Param(
    [switch]$Clean,
    [switch]$Coverage,
    [string]$Config = "Debug"
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Resolve-Path (Join-Path $scriptDir "..")
$buildDir = Join-Path $rootDir "build"

# Help vcpkg find VS Build Tools (vswhere needs -products * flag for Build Tools)
if (-not $env:VCPKG_VISUAL_STUDIO_PATH) {
    $vsPath = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -products * -latest -property installationPath 2>$null
    if ($vsPath) {
        $env:VCPKG_VISUAL_STUDIO_PATH = $vsPath
        Write-Host "Using Visual Studio: $vsPath"
    }
}

if ($Clean -and (Test-Path $buildDir)) {
    Write-Host "Cleaning build directory (preserving vcpkg)..."
    Get-ChildItem -Path $buildDir | Where-Object { $_.Name -ne "vcpkg_installed" } | Remove-Item -Recurse -Force
}

$cmakeArgs = @(
    "-S", $rootDir, 
    "-B", $buildDir, 
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=$Config",
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
)

if ($env:VCPKG_ROOT) {
    $toolchain = Join-Path $env:VCPKG_ROOT "scripts/buildsystems/vcpkg.cmake"
    $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$toolchain"
}

if ($Coverage) {
    $cmakeArgs += "-DENABLE_COVERAGE=ON"
    Write-Host "Coverage instrumentation enabled" -ForegroundColor Cyan
}

Write-Host "Configuring project..."
& cmake $cmakeArgs

Write-Host "Building project..."
cmake --build $buildDir --config $Config

Write-Host "Build complete."
