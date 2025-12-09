Param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release"
)

$src = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Resolve-Path (Join-Path $src "..")
$buildDir = Join-Path $root "build"

$toolchain = $env:VCPKG_ROOT
if (-not $toolchain) {
    Write-Error "VCPKG_ROOT is not set; please set it to your vcpkg directory."
    exit 1
}
$toolchainFile = Join-Path $toolchain "scripts/buildsystems/vcpkg.cmake"

if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

cmake -S $root -B $buildDir -G Ninja -DCMAKE_TOOLCHAIN_FILE="$toolchainFile" "-DCMAKE_BUILD_TYPE=$Config"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

cmake --build $buildDir --config $Config
