Param(
    [switch]$Clean,
    [string]$Config = "Debug"
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Resolve-Path (Join-Path $scriptDir "..")
$buildDir = Join-Path $rootDir "build"

if ($Clean -and (Test-Path $buildDir)) {
    Write-Host "Cleaning build directory (preserving vcpkg)..."
    Get-ChildItem -Path $buildDir | Where-Object { $_.Name -ne "vcpkg_installed" } | Remove-Item -Recurse -Force
}

$cmakeArgs = @("-S", $rootDir, "-B", $buildDir)

if ($env:VCPKG_ROOT) {
    $toolchain = Join-Path $env:VCPKG_ROOT "scripts/buildsystems/vcpkg.cmake"
    $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$toolchain"
}

Write-Host "Configuring project..."
& cmake $cmakeArgs

Write-Host "Building project..."
cmake --build $buildDir --config $Config

Write-Host "Build complete."
