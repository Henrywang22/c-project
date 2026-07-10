param(
    [string]$Version = "1.0.4"
)

$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildDir = Join-Path $root "build-release"
$releaseRoot = Join-Path $root "release"
$stageDir = Join-Path $releaseRoot ("FishingVoyage_v" + $Version)
$zipPath = Join-Path $releaseRoot ("FishingVoyage_v" + $Version + "_win64.zip")
$runtimeBin = Split-Path -Parent ((Get-Command g++.exe -ErrorAction Stop).Source)
$dependencyCollector = Join-Path $PSScriptRoot "collect_runtime_dependencies.cmake"

cmake -S $root -B $buildDir -G Ninja -DCMAKE_BUILD_TYPE=Release -DFISHINGVOYAGE_ENABLE_TEST_MODE=ON
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }

cmake --build $buildDir --config Release
if ($LASTEXITCODE -ne 0) { throw "Release build failed." }

New-Item -ItemType Directory -Path $releaseRoot -Force | Out-Null

if (Test-Path -LiteralPath $stageDir) {
    $resolvedStage = (Resolve-Path -LiteralPath $stageDir).Path
    if (-not $resolvedStage.StartsWith($releaseRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove a staging directory outside the release root."
    }
    Remove-Item -LiteralPath $resolvedStage -Recurse -Force
}
New-Item -ItemType Directory -Path $stageDir -Force | Out-Null

Copy-Item -LiteralPath (Join-Path $buildDir "FishingVoyage.exe") -Destination $stageDir
Copy-Item -LiteralPath (Join-Path $buildDir "FishingVoyage.rcc") -Destination $stageDir
Copy-Item -LiteralPath (Join-Path $root "RELEASE_README.txt") -Destination (Join-Path $stageDir "README.txt")

windeployqt --release --compiler-runtime --no-translations (Join-Path $stageDir "FishingVoyage.exe")
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed." }

# windeployqt does not consistently include the MinGW/UCRT runtime and Qt's
# non-Qt dependency DLLs. Collect them recursively so the archive runs on a
# computer without the development environment.
cmake "-DSTAGE_DIR=$stageDir" "-DRUNTIME_BIN=$runtimeBin" -P $dependencyCollector
if ($LASTEXITCODE -ne 0) { throw "Runtime dependency collection failed." }

cmake "-DSTAGE_DIR=$stageDir" "-DRUNTIME_BIN=$runtimeBin" -DVERIFY_ONLY=ON -P $dependencyCollector
if ($LASTEXITCODE -ne 0) { throw "Runtime dependency verification failed." }

$forbidden = @("save.dat", "save.tmp", "Log.dat", "highscore.dat")
foreach ($name in $forbidden) {
    if (Test-Path -LiteralPath (Join-Path $stageDir $name)) {
        throw "Release staging contains forbidden user data: $name"
    }
}

if (Test-Path -LiteralPath $zipPath) {
    $resolvedZip = (Resolve-Path -LiteralPath $zipPath).Path
    if (-not $resolvedZip.StartsWith($releaseRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove an archive outside the release root."
    }
    Remove-Item -LiteralPath $resolvedZip -Force
}

Compress-Archive -Path (Join-Path $stageDir "*") -DestinationPath $zipPath -CompressionLevel Optimal

Write-Output ("Release directory: " + $stageDir)
Write-Output ("Release archive: " + $zipPath)
