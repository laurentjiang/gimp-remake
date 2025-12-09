Param(
    [string]$BuildDir = "build"
)

# Locate clang-tidy (try common names)
$clangTidy = Get-Command clang-tidy -ErrorAction SilentlyContinue
if (-not $clangTidy) {
    $clangTidy = Get-Command clang-tidy-17 -ErrorAction SilentlyContinue
}
if (-not $clangTidy) {
    Write-Error "clang-tidy not found. Install via choco (clang-tools) or llvm." 
    exit 1
}

# Ensure build dir with compile_commands.json exists
if (-not (Test-Path $BuildDir)) {
    cmake -B $BuildDir -G Ninja -DCMAKE_BUILD_TYPE=Release
}
if (-not (Test-Path (Join-Path $BuildDir "compile_commands.json"))) {
    cmake -B $BuildDir -G Ninja -DCMAKE_BUILD_TYPE=Release
}

# Run clang-tidy on a starter translation unit; extend list as sources grow
& $clangTidy.Path --quiet src/main.cpp -- -std=c++20

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Host "clang-tidy completed." -ForegroundColor Green
