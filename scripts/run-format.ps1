# Formats all tracked C/C++ sources in-place using clang-format.

# Locate clang-format (try common names)
$clangFormat = Get-Command clang-format -ErrorAction SilentlyContinue
if (-not $clangFormat) {
    $clangFormat = Get-Command clang-format-18 -ErrorAction SilentlyContinue
}
if (-not $clangFormat) {
    Write-Error "clang-format not found. Install LLVM 18+ via: choco install llvm" 
    exit 1
}

# Gather files
$files = git ls-files "*.c" "*.cc" "*.cpp" "*.cxx" "*.h" "*.hpp"
if (-not $files) {
    Write-Host "No C/C++ files to format." -ForegroundColor Yellow
    exit 0
}

$files.Split("`n") | Where-Object { $_ -ne "" } | ForEach-Object {
    & $clangFormat.Path -i $_
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

Write-Host "clang-format completed." -ForegroundColor Green
