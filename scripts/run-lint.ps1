$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Resolve-Path (Join-Path $scriptDir "..")
$buildDir = Join-Path $rootDir "build"

# Locate clang-tidy (try common names)
$clangTidy = Get-Command clang-tidy -ErrorAction SilentlyContinue
if (-not $clangTidy) {
    $clangTidy = Get-Command clang-tidy-18 -ErrorAction SilentlyContinue
}
if (-not $clangTidy) {
    Write-Error "clang-tidy not found. Install LLVM 18+ via: choco install llvm" 
    exit 1
}

# Detect VCPKG_ROOT from GitHub Actions environment if not set
if (-not $env:VCPKG_ROOT -and $env:VCPKG_INSTALLATION_ROOT) {
    $env:VCPKG_ROOT = $env:VCPKG_INSTALLATION_ROOT
    Write-Host "Detected GitHub Actions VCPKG_ROOT: $env:VCPKG_ROOT"
}

# Requires compile_commands.json from a Ninja build
# Run build.ps1 first if it doesn't exist
if (-not (Test-Path (Join-Path $buildDir "compile_commands.json"))) {
    Write-Error "compile_commands.json not found. Run .\scripts\build.ps1 first."
    exit 1
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
    & $clangTidy.Path -p $buildDir --quiet $sources.FullName 2>&1 | ForEach-Object {
        $line = $_.ToString()
        if ($line -notmatch "warnings generated\.$") {
            Write-Output $line
        }
    }
} else {
    Write-Host "No source files found."
}

Write-Host "clang-tidy completed." -ForegroundColor Green
