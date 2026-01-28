Param(
    [switch]$NoCoverage,
    [switch]$Verbose,
    [string]$Filter = ""
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Resolve-Path (Join-Path $scriptDir "..")
$buildDir = Join-Path $rootDir "build"
$testExe = Join-Path $buildDir "unit_tests.exe"

# Suppress OpenCV plugin loader INFO messages
$env:OPENCV_LOG_LEVEL = "WARNING"

# Set up coverage output directory
$profDataDir = Join-Path $buildDir "profdata"
if (-not $NoCoverage) {
    New-Item -ItemType Directory -Path $profDataDir -Force | Out-Null
    $env:LLVM_PROFILE_FILE = Join-Path $profDataDir "default.profraw"
}

# Check if test executable exists
if (-not (Test-Path $testExe)) {
    Write-Host "Test executable not found. Building..." -ForegroundColor Yellow
    & "$scriptDir\build.ps1"
    
    if (-not (Test-Path $testExe)) {
        Write-Error "Failed to build tests."
        exit 1
    }
}

# Build test arguments
$testArgs = @()

if ($Verbose) {
    $testArgs += "--success"
}

if ($Filter) {
    $testArgs += $Filter
}

# Run tests
Write-Host "Running tests..." -ForegroundColor Cyan
& $testExe $testArgs
$testExitCode = $LASTEXITCODE

if ($testExitCode -ne 0) {
    Write-Host "Tests failed with exit code $testExitCode" -ForegroundColor Red
    exit $testExitCode
}

Write-Host "All tests passed!" -ForegroundColor Green

# Coverage report (requires build with coverage flags)
if (-not $NoCoverage) {
    Write-Host ""
    Write-Host "Coverage Analysis" -ForegroundColor Cyan
    Write-Host "-----------------"
    
    $profRaw = Join-Path $profDataDir "default.profraw"
    $profData = Join-Path $profDataDir "coverage.profdata"
    
    # Check for llvm-profdata and llvm-cov
    $llvmProfdata = Get-Command "llvm-profdata" -ErrorAction SilentlyContinue
    $llvmCov = Get-Command "llvm-cov" -ErrorAction SilentlyContinue
    
    if (-not $llvmProfdata -or -not $llvmCov) {
        Write-Host "Coverage tools not found (llvm-profdata, llvm-cov)." -ForegroundColor Yellow
        Write-Host "To enable coverage, ensure LLVM is installed and in PATH." -ForegroundColor Yellow
        exit 0
    }
    
    # Check for profile data
    if (-not (Test-Path $profRaw)) {
        Write-Host "No coverage data found." -ForegroundColor Yellow
        Write-Host "Rebuild with coverage enabled:" -ForegroundColor Yellow
        Write-Host "  .\scripts\build.ps1 -Clean" -ForegroundColor Yellow
        Write-Host "  cmake -B build -DENABLE_COVERAGE=ON" -ForegroundColor Yellow
        Write-Host "  cmake --build build" -ForegroundColor Yellow
        Write-Host "Then run tests again." -ForegroundColor Yellow
        exit 0
    }
    
    # Merge profile data
    & llvm-profdata merge -sparse $profRaw -o $profData
    
    # Generate coverage report for src/ files only (exclude tests, third-party)
    $srcDir = Join-Path $rootDir "src"
    $includeDir = Join-Path $rootDir "include"
    
    Write-Host ""
    Write-Host "Coverage Summary (src/ and include/ only):" -ForegroundColor Cyan
    & llvm-cov report $testExe "-instr-profile=$profData" "-ignore-filename-regex=tests[/\\]|build[/\\]|vcpkg_installed[/\\]|_deps[/\\]" -show-region-summary=false
    
    Write-Host ""
    Write-Host "To see detailed line coverage:" -ForegroundColor Gray
    Write-Host "  llvm-cov show $testExe -instr-profile=$profData -format=html -o coverage/" -ForegroundColor Gray
}

exit 0
