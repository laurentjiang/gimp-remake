$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Resolve-Path (Join-Path $scriptDir "..")
$buildDir = Join-Path $rootDir "build"

# Dynamic parallelism: use ~75% of logical processors, min 2, max 16
# This balances speed vs memory usage (clang-tidy is memory-intensive)
$cpuCount = [Environment]::ProcessorCount
$parallelJobs = [Math]::Max(2, [Math]::Min(16, [Math]::Floor($cpuCount * 0.75)))

# Allow override via environment variable (useful for CI tuning)
if ($env:LINT_PARALLEL_JOBS) {
    $parallelJobs = [int]$env:LINT_PARALLEL_JOBS
}

# Locate clang-tidy (try common names)
$clangTidy = Get-Command clang-tidy -ErrorAction SilentlyContinue
if (-not $clangTidy) {
    $clangTidy = Get-Command clang-tidy-18 -ErrorAction SilentlyContinue
}
if (-not $clangTidy) {
    Write-Error "clang-tidy not found. Install LLVM 18+ via: choco install llvm" 
    exit 1
}

Write-Host "Using clang-tidy: $($clangTidy.Path)"
$version = & $clangTidy.Path --version 2>&1 | Select-Object -First 2
Write-Host $version
Write-Host "Parallel jobs: $parallelJobs (CPUs: $cpuCount)"

# Detect VCPKG_ROOT from GitHub Actions environment if not set
if (-not $env:VCPKG_ROOT -and $env:VCPKG_INSTALLATION_ROOT) {
    $env:VCPKG_ROOT = $env:VCPKG_INSTALLATION_ROOT
    Write-Host "Detected GitHub Actions VCPKG_ROOT: $env:VCPKG_ROOT"
}

# Requires compile_commands.json from a Ninja build
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
    
    $clangTidyPath = $clangTidy.Path
    $totalFiles = $sources.Count
    
    # Run clang-tidy in parallel using ForEach-Object -Parallel (PowerShell 7+)
    $results = $sources | ForEach-Object -Parallel {
        $file = $_
        $output = & $using:clangTidyPath -p $using:buildDir --quiet $file.FullName 2>&1
        
        # Filter warning count noise but keep everything else
        $filtered = $output | Where-Object { $_ -notmatch "^\d+ warnings? generated\.$" }
        $outputStr = ($filtered | Out-String).Trim()
        
        $hasIssues = $outputStr -match "warning:|error:"
        
        [PSCustomObject]@{
            File = $file.Name
            FullName = $file.FullName
            Output = $outputStr
            HasIssues = $hasIssues
        }
    } -ThrottleLimit $parallelJobs
    
    # Separate successes and failures
    $failures = $results | Where-Object { $_.HasIssues }
    $successes = $results | Where-Object { -not $_.HasIssues }
    
    # Display summary
    Write-Host "`nResults: $($successes.Count) OK, $($failures.Count) with issues" -ForegroundColor $(if ($failures.Count -eq 0) { "Green" } else { "Yellow" })
    
    # Display all failures at the end
    if ($failures.Count -gt 0) {
        Write-Host "`n" + ("=" * 60) -ForegroundColor Red
        Write-Host "CLANG-TIDY ISSUES FOUND" -ForegroundColor Red
        Write-Host ("=" * 60) -ForegroundColor Red
        
        foreach ($result in $failures) {
            Write-Host "`n=== $($result.FullName) ===" -ForegroundColor Yellow
            Write-Host $result.Output
        }
        
        Write-Host "`nclang-tidy found issues in $($failures.Count) file(s)." -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host "No source files found."
}

Write-Host "`nclang-tidy completed successfully." -ForegroundColor Green
