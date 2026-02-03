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
$vsPath = $env:VCPKG_VISUAL_STUDIO_PATH
if (-not $vsPath) {
    $vsPath = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -products * -latest -property installationPath 2>$null
    if ($vsPath) {
        $env:VCPKG_VISUAL_STUDIO_PATH = $vsPath
    }
}

# Setup MSVC environment if cl.exe not in PATH
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    if ($vsPath) {
        $vcvarsall = Join-Path $vsPath "VC\Auxiliary\Build\vcvarsall.bat"
        if (Test-Path $vcvarsall) {
            Write-Host "Setting up MSVC environment..."
            # Run vcvarsall and capture environment variables
            $envVars = cmd /c "`"$vcvarsall`" x64 >nul 2>&1 && set" | ForEach-Object {
                if ($_ -match "^(.+?)=(.*)$") {
                    [PSCustomObject]@{ Name = $matches[1]; Value = $matches[2] }
                }
            }
            foreach ($var in $envVars) {
                Set-Item -Path "env:$($var.Name)" -Value $var.Value
            }
        }
    }
}

Write-Host "Using Visual Studio: $vsPath"

if ($Clean -and (Test-Path $buildDir)) {
    Write-Host "Cleaning build directory (preserving vcpkg)..."
    Get-ChildItem -Path $buildDir | Where-Object { $_.Name -ne "vcpkg_installed" } | Remove-Item -Recurse -Force
}

$cmakeArgs = @(
    "-S", $rootDir, 
    "-B", $buildDir, 
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=$Config",
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    "-DCMAKE_CXX_COMPILER=cl.exe",
    "-DCMAKE_C_COMPILER=cl.exe"
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
