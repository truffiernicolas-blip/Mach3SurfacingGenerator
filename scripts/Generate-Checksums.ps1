param(
    [string]$DistDirectory = (Join-Path $PSScriptRoot "..\dist")
)

$ErrorActionPreference = "Stop"
$distPath = [System.IO.Path]::GetFullPath($DistDirectory)
$checksumPath = Join-Path $distPath "SHA256SUMS.txt"

$artifacts = Get-ChildItem -LiteralPath $distPath -File |
    Where-Object {
        $_.Name -ne "SHA256SUMS.txt" -and
        $_.Extension -in ".zip", ".deb", ".gz"
    } |
    Sort-Object Name

if (-not $artifacts) {
    throw "Aucun artefact de release dans $distPath."
}

$lines = foreach ($artifact in $artifacts) {
    $hash = (Get-FileHash -LiteralPath $artifact.FullName -Algorithm SHA256).Hash
    "$($hash.ToLowerInvariant())  $($artifact.Name)"
}

Set-Content -LiteralPath $checksumPath -Value $lines -Encoding ascii
Write-Host "Checksums: $checksumPath"

