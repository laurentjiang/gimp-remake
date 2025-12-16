Param(
    [string]$BuildDir = "build-lint"
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Resolve-Path (Join-Path $scriptDir "..")
$lintBuildDir = Join-Path $rootDir $BuildDir

# Locate clang-tidy (try common names)
$clangTidy = Get-Command clang-tidy -ErrorAction SilentlyContinue
if (-not $clangTidy) {
    $clangTidy = Get-Command clang-tidy-17 -ErrorAction SilentlyContinue
}
if (-not $clangTidy) {
    Write-Error "clang-tidy not found. Install via choco (clang-tools) or llvm." 
    exit 1
}

# Detect VCPKG_ROOT from GitHub Actions environment if not set
if (-not $env:VCPKG_ROOT -and $env:VCPKG_INSTALLATION_ROOT) {
    $env:VCPKG_ROOT = $env:VCPKG_INSTALLATION_ROOT
    Write-Host "Detected GitHub Actions VCPKG_ROOT: $env:VCPKG_ROOT"
}

# Configure for linting (needs compile_commands.json, usually requires Ninja on Windows)
# We use a separate build directory to avoid conflicts with the main Visual Studio build
if (-not (Test-Path (Join-Path $lintBuildDir "compile_commands.json"))) {
    Write-Host "Configuring lint build in $lintBuildDir..."
    
    $cmakeArgs = @("-S", $rootDir, "-B", $lintBuildDir, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Debug", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
    
    if ($env:VCPKG_ROOT) {
        $toolchain = Join-Path $env:VCPKG_ROOT "scripts/buildsystems/vcpkg.cmake"
        $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$toolchain"
    }
    
    & cmake $cmakeArgs
}

# Find source files (excluding tests, build dirs, and vcpkg)
$sources = Get-ChildItem -Path $rootDir -Recurse -Include *.cpp | 
           Where-Object { 
               $_.FullName -notmatch "\\build" -and 
               $_.FullName -notmatch "\\vcpkg" -and 
               $_.FullName -notmatch "\\tests" 
           }

if ($sources) {
    Write-Host "Running clang-tidy on $($sources.Count) files..."
    # -p points to the build directory containing compile_commands.json
    # We filter out "warnings generated" messages which count suppressed warnings from system headers
    & $clangTidy.Path -p $lintBuildDir --quiet $sources.FullName 2>&1 | ForEach-Object {
        $line = $_.ToString()
        if ($line -notmatch "warnings generated\.$") {
            Write-Output $line
        }
    }
} else {
    Write-Host "No source files found."
}

Write-Host "clang-tidy completed." -ForegroundColor Green
