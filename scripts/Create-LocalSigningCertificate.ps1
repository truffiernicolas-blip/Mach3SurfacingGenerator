param(
    [string]$OutputDirectory = (Join-Path $PSScriptRoot "..\certificates")
)

$ErrorActionPreference = "Stop"
$subject = "CN=Mach3 Surfacing Generator Local Code Signing"

$certificate = Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert |
    Where-Object {
        $_.Subject -eq $subject -and
        $_.HasPrivateKey -and
        $_.NotAfter -gt (Get-Date)
    } |
    Sort-Object NotAfter -Descending |
    Select-Object -First 1

if (-not $certificate) {
    $certificate = New-SelfSignedCertificate `
        -Type CodeSigningCert `
        -Subject $subject `
        -CertStoreLocation Cert:\CurrentUser\My `
        -HashAlgorithm SHA256 `
        -KeyAlgorithm RSA `
        -KeyLength 3072 `
        -KeyExportPolicy Exportable `
        -NotAfter (Get-Date).AddYears(5)
}

$outputPath = [System.IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $outputPath -Force | Out-Null
$publicCertificatePath = Join-Path $outputPath "Mach3SurfacingGenerator-Local.cer"
Export-Certificate -Cert $certificate -FilePath $publicCertificatePath -Force | Out-Null

& certutil.exe -user -f -addstore Root $publicCertificatePath | Out-Null
if ($LASTEXITCODE -ne 0) {
    throw "Impossible d'ajouter le certificat au magasin racine."
}

& certutil.exe -user -f -addstore TrustedPublisher $publicCertificatePath | Out-Null
if ($LASTEXITCODE -ne 0) {
    throw "Impossible d'ajouter le certificat aux éditeurs approuvés."
}

Write-Host "Certificat local créé et approuvé pour l'utilisateur courant."
Write-Host "Thumbprint: $($certificate.Thumbprint)"
Write-Host "Certificat public: $publicCertificatePath"
