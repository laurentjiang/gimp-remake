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

Write-Host "Using clang-tidy: $($clangTidy.Path)"
$version = & $clangTidy.Path --version 2>&1 | Select-Object -First 2
Write-Host $version

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
    $failures = @()
    $index = 0
    
    foreach ($file in $sources) {
        $index++
        Write-Host "[$index/$($sources.Count)] $($file.Name)" -NoNewline
        
        $output = & $clangTidyPath -p $buildDir --quiet $file.FullName 2>&1
        
        # Filter warning count noise but keep everything else
        $filtered = $output | Where-Object { $_ -notmatch "^\d+ warnings? generated\.$" }
        $outputStr = ($filtered | Out-String).Trim()
        
        if ($outputStr -match "warning:|error:") {
            Write-Host " - ISSUES FOUND" -ForegroundColor Red
            $failures += [PSCustomObject]@{
                File = $file.Name
                FullName = $file.FullName
                Output = $outputStr
            }
        } else {
            Write-Host " - OK" -ForegroundColor Green
        }
    }
    
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
