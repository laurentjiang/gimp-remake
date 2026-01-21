Param(
    [string]$Config = "Debug"
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Resolve-Path (Join-Path $scriptDir "..")
$buildDir = Join-Path $rootDir "build"

# With Ninja, exe is in build root, not build/Config
$exePath = Join-Path $buildDir "gimp-remake.exe"
if (-not (Test-Path $exePath)) {
    # Fallback for multi-config generators (Visual Studio)
    $exePath = Join-Path $buildDir "$Config/gimp-remake.exe"
    if (-not (Test-Path $exePath)) {
        Write-Error "Executable not found. Please run .\scripts\build.ps1 first."
        exit 1
    }
}

$exeDir = Split-Path $exePath

# Set QT_PLUGIN_PATH dynamically based on vcpkg location in build dir
# Local dev uses x64-windows triplet (debug+release)
$vcpkgRoot = Join-Path $buildDir "vcpkg_installed/x64-windows"
if ($Config -eq "Debug") {
    $qtPlugins = Join-Path $vcpkgRoot "debug/Qt6/plugins"
    $binDir = Join-Path $vcpkgRoot "debug/bin"
} else {
    $qtPlugins = Join-Path $vcpkgRoot "Qt6/plugins"
    $binDir = Join-Path $vcpkgRoot "bin"
}

# Copy platforms folder next to exe - most reliable for Qt on Windows
if (Test-Path $qtPlugins) {
    $platformsSource = Join-Path $qtPlugins "platforms"
    $platformsDest = Join-Path $exeDir "platforms"
    
    # Always refresh to ensure correct debug/release version
    if (Test-Path $platformsDest) {
        Remove-Item -Path $platformsDest -Recurse -Force
    }
    if (Test-Path $platformsSource) {
        Write-Host "Deploying Qt platforms plugin..."
        Copy-Item -Path $platformsSource -Destination $platformsDest -Recurse
    }
}

# Add vcpkg bin directory to PATH so DLLs can be found
if (Test-Path $binDir) {
    $env:PATH = "$binDir;$env:PATH"
    Write-Host "Added $binDir to PATH"
}

# Set QT_PLUGIN_PATH as backup
$env:QT_PLUGIN_PATH = $exeDir

Write-Host "Starting $exePath..."
& $exePath
