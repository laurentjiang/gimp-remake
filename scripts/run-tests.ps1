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

# Coverage output directory
$coverageDir = Join-Path $buildDir "coverage"

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

# Coverage report using OpenCppCoverage (works with MSVC)
if (-not $NoCoverage) {
    Write-Host ""
    Write-Host "Coverage Analysis" -ForegroundColor Cyan
    Write-Host "-----------------"
    
    # Check for OpenCppCoverage
    $openCppCov = Get-Command "OpenCppCoverage" -ErrorAction SilentlyContinue
    if (-not $openCppCov) {
        # Try common install location
        $defaultPath = "C:\Program Files\OpenCppCoverage\OpenCppCoverage.exe"
        if (Test-Path $defaultPath) {
            $openCppCov = @{ Source = $defaultPath }
        }
    }
    
    if (-not $openCppCov) {
        Write-Host "OpenCppCoverage not found." -ForegroundColor Yellow
        Write-Host "Install it with: choco install opencppcoverage" -ForegroundColor Yellow
        Write-Host "Or download from: https://github.com/OpenCppCoverage/OpenCppCoverage/releases" -ForegroundColor Yellow
        exit 0
    }
    
    $openCppCovExe = if ($openCppCov.Source) { $openCppCov.Source } else { "OpenCppCoverage" }
    
    # Create coverage output directory
    New-Item -ItemType Directory -Path $coverageDir -Force | Out-Null
    
    $srcDir = Join-Path $rootDir "src"
    $includeDir = Join-Path $rootDir "include"
    $coverageXml = Join-Path $coverageDir "coverage.xml"
    $coverageHtml = Join-Path $coverageDir "html"
    
    Write-Host "Running tests with coverage..." -ForegroundColor Cyan
    
    # Build coverage arguments
    $covArgs = @(
        "--sources", $srcDir,
        "--sources", $includeDir,
        "--excluded_sources", (Join-Path $rootDir "tests"),
        "--excluded_sources", (Join-Path $buildDir "vcpkg_installed"),
        "--excluded_sources", (Join-Path $buildDir "_deps"),
        "--export_type", "cobertura:$coverageXml",
        "--export_type", "html:$coverageHtml",
        "--", $testExe
    )
    
    if ($Filter) {
        $covArgs += $Filter
    }
    
    & $openCppCovExe $covArgs
    
    # Parse and display coverage summary
    if (Test-Path $coverageXml) {
        [xml]$cov = Get-Content $coverageXml
        $lineRate = [math]::Round([double]$cov.coverage.'line-rate' * 100, 1)
        $branchRate = [math]::Round([double]$cov.coverage.'branch-rate' * 100, 1)
        
        Write-Host ""
        Write-Host "Coverage Summary:" -ForegroundColor Cyan
        Write-Host "  Line Coverage:   $lineRate%" -ForegroundColor $(if ($lineRate -ge 70) { "Green" } elseif ($lineRate -ge 50) { "Yellow" } else { "Red" })
        Write-Host "  Branch Coverage: $branchRate%" -ForegroundColor $(if ($branchRate -ge 70) { "Green" } elseif ($branchRate -ge 50) { "Yellow" } else { "Red" })
        Write-Host ""
        Write-Host "Detailed HTML report: $coverageHtml\index.html" -ForegroundColor Gray
    }
}

exit 0
