param(
    [string]$QtDirectory = "C:\Qt\6.11.0\mingw_64",
    [string]$MinGwDirectory = "C:\Qt\Tools\mingw1310_64",
    [string]$NinjaDirectory = "C:\Qt\Tools\Ninja",
    [switch]$SkipSigning
)

$ErrorActionPreference = "Stop"

$root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$cmakeText = Get-Content -LiteralPath (Join-Path $root "CMakeLists.txt") -Raw
if ($cmakeText -notmatch "project\(Mach3SurfacingGenerator VERSION ([0-9.]+)") {
    throw "Version du projet introuvable."
}
$version = $Matches[1]

$buildDirectory = Join-Path $root "build-release-windows"
$distDirectory = Join-Path $root "dist"
$releaseName = "Mach3SurfacingGenerator-$version-windows-x64"
$stageDirectory = Join-Path $distDirectory $releaseName
$archivePath = Join-Path $distDirectory "$releaseName.zip"

$pathEntries = @()
if (-not [string]::IsNullOrWhiteSpace($MinGwDirectory) -and
    (Test-Path -LiteralPath (Join-Path $MinGwDirectory "bin"))) {
    $pathEntries += (Join-Path $MinGwDirectory "bin")
}
if (-not [string]::IsNullOrWhiteSpace($NinjaDirectory) -and
    (Test-Path -LiteralPath $NinjaDirectory)) {
    $pathEntries += $NinjaDirectory
}
if ($pathEntries.Count -gt 0) {
    $env:PATH = ($pathEntries -join ";") + ";$env:PATH"
}

cmake -S $root -B $buildDirectory -G Ninja `
    "-DCMAKE_PREFIX_PATH=$QtDirectory" `
    -DCMAKE_BUILD_TYPE=Release `
    -DBUILD_TESTING=ON
if ($LASTEXITCODE -ne 0) { throw "Configuration CMake echouee." }

cmake --build $buildDirectory --parallel
if ($LASTEXITCODE -ne 0) { throw "Compilation Windows echouee." }

$env:PATH = "$QtDirectory\bin;$env:PATH"
ctest --test-dir $buildDirectory --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "Tests Windows echoues." }

New-Item -ItemType Directory -Path $distDirectory -Force | Out-Null
if (Test-Path -LiteralPath $stageDirectory) {
    $resolvedStage = [System.IO.Path]::GetFullPath($stageDirectory)
    $resolvedDist = [System.IO.Path]::GetFullPath($distDirectory) +
        [System.IO.Path]::DirectorySeparatorChar
    if (-not $resolvedStage.StartsWith(
            $resolvedDist,
            [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Dossier de release inattendu: $resolvedStage"
    }
    Remove-Item -LiteralPath $resolvedStage -Recurse -Force
}
New-Item -ItemType Directory -Path $stageDirectory -Force | Out-Null

Copy-Item -LiteralPath (Join-Path $buildDirectory "Mach3SurfacingGenerator.exe") `
    -Destination $stageDirectory
Copy-Item -LiteralPath (Join-Path $root "LICENSE") -Destination $stageDirectory
Copy-Item -LiteralPath (Join-Path $root "README.md") -Destination $stageDirectory
Copy-Item -LiteralPath (Join-Path $root "CHANGELOG.md") -Destination $stageDirectory
Copy-Item -LiteralPath (Join-Path $root "presets") `
    -Destination $stageDirectory -Recurse

& (Join-Path $QtDirectory "bin\windeployqt.exe") `
    --release --compiler-runtime --no-translations `
    (Join-Path $stageDirectory "Mach3SurfacingGenerator.exe")
if ($LASTEXITCODE -ne 0) { throw "Deploiement Qt echoue." }

if (-not $SkipSigning) {
    & (Join-Path $PSScriptRoot "Sign-Release.ps1") `
        -BuildDirectory $stageDirectory
    if ($LASTEXITCODE -ne 0) { throw "Signature Windows echouee." }

    $publicCertificate = Join-Path $root `
        "certificates\Mach3SurfacingGenerator-Local.cer"
    if (Test-Path -LiteralPath $publicCertificate) {
        Copy-Item -LiteralPath $publicCertificate -Destination $stageDirectory
        Copy-Item -LiteralPath `
            (Join-Path $PSScriptRoot "Install-LocalCertificate.ps1") `
            -Destination $stageDirectory
    }
}

if (Test-Path -LiteralPath $archivePath) {
    Remove-Item -LiteralPath $archivePath -Force
}
Compress-Archive -Path (Join-Path $stageDirectory "*") `
    -DestinationPath $archivePath -CompressionLevel Optimal

Write-Host "Release Windows: $archivePath"
