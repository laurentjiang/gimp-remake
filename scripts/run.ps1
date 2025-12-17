Param(
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Resolve-Path (Join-Path $scriptDir "..")
$buildDir = Join-Path $rootDir "build"
$exePath = Join-Path $buildDir "$Config/gimp-remake.exe"

# Set QT_PLUGIN_PATH dynamically based on vcpkg location in build dir
# This helps it work on different machines assuming vcpkg is used
if ($Config -eq "Debug") {
    $qtPlugins = Join-Path $buildDir "vcpkg_installed/x64-windows/debug/Qt6/plugins"
} elseif ($Config -eq "Release") {
    $qtPlugins = Join-Path $buildDir "vcpkg_installed/x64-windows/release/Qt6/plugins"
} else {
    $qtPlugins = Join-Path $buildDir "vcpkg_installed/x64-windows/Qt6/plugins"
}

if (Test-Path $qtPlugins) {
    # Instead of setting env var, let's copy the platforms folder next to the exe
    # This is the most reliable way for Qt on Windows
    $platformsSource = Join-Path $qtPlugins "platforms"
    $platformsDest = Join-Path (Split-Path $exePath) "platforms"
    
    if ((Test-Path $platformsSource) -and -not (Test-Path $platformsDest)) {
        Write-Host "Deploying Qt platforms plugin to $platformsDest..."
        Copy-Item -Path $platformsSource -Destination $platformsDest -Recurse
    }
}

# Add vcpkg bin directory to PATH so DLLs can be found
$vcpkgRoot = Join-Path $buildDir "vcpkg_installed/x64-windows"
if ($Config -eq "Debug") {
    $binDir = Join-Path $vcpkgRoot "debug/bin"
} else {
    $binDir = Join-Path $vcpkgRoot "bin"
}

if (Test-Path $binDir) {
    $env:PATH = "$binDir;$env:PATH"
    Write-Host "Added $binDir to PATH"
}

if (-not (Test-Path $exePath)) {
    # Fallback to root if not found (e.g. Ninja generator)
    $exePathRoot = Join-Path $buildDir "gimp-remake.exe"
    if (Test-Path $exePathRoot) { 
        $exePath = $exePathRoot 
    } else {
        Write-Error "Executable not found at $exePath or $exePathRoot. Please build the project first."
        exit 1
    }
}

& $exePath
