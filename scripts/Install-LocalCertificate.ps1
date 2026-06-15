param(
    [string]$CertificatePath = (Join-Path $PSScriptRoot "Mach3SurfacingGenerator-Local.cer")
)

$ErrorActionPreference = "Stop"
$certificate = [System.IO.Path]::GetFullPath($CertificatePath)

if (-not (Test-Path -LiteralPath $certificate)) {
    throw "Certificat introuvable: $certificate"
}

& certutil.exe -user -f -addstore Root $certificate | Out-Null
if ($LASTEXITCODE -ne 0) {
    throw "Impossible d'ajouter le certificat au magasin racine."
}

& certutil.exe -user -f -addstore TrustedPublisher $certificate | Out-Null
if ($LASTEXITCODE -ne 0) {
    throw "Impossible d'ajouter le certificat aux editeurs approuves."
}

Write-Host "Certificat installe pour l'utilisateur courant."

