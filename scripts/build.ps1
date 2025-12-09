Param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release"
)

$src = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Resolve-Path (Join-Path $src "..")
$buildDir = Join-Path $root "build"

if (Test-Path $buildDir) {
    Remove-Item -Recurse -Force $buildDir
}
New-Item -ItemType Directory -Path $buildDir | Out-Null

cmake -S $root -B $buildDir -G "Ninja Multi-Config"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

cmake --build $buildDir --config $Config
