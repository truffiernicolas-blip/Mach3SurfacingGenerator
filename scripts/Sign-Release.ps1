param(
    [string]$BuildDirectory = (Join-Path $PSScriptRoot "..\build"),
    [string]$CertificateThumbprint = $env:MACH3_SIGNING_CERT_THUMBPRINT,
    [string]$TimestampUrl = $env:MACH3_TIMESTAMP_URL
)

$ErrorActionPreference = "Stop"

$buildPath = [System.IO.Path]::GetFullPath($BuildDirectory)
$signTool = Get-ChildItem "C:\Program Files (x86)\Windows Kits\10\bin" `
    -Recurse -Filter signtool.exe -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match "\\x64\\signtool\.exe$" } |
    Sort-Object FullName -Descending |
    Select-Object -First 1

if (-not $signTool) {
    throw "SignTool x64 est introuvable. Installez le Windows SDK."
}

if ([string]::IsNullOrWhiteSpace($CertificateThumbprint)) {
    $certificate = Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert |
        Where-Object {
            $_.Subject -eq "CN=Mach3 Surfacing Generator Local Code Signing" -and
            $_.HasPrivateKey -and
            $_.NotAfter -gt (Get-Date)
        } |
        Sort-Object NotAfter -Descending |
        Select-Object -First 1

    if (-not $certificate) {
        throw "Aucun certificat Mach3 local valide n'a été trouvé."
    }
    $CertificateThumbprint = $certificate.Thumbprint
}

$targets = Get-ChildItem -LiteralPath $buildPath -Recurse -File |
    Where-Object {
        $_.Extension -in ".exe", ".dll" -and
        $_.Name -ne "GCodeGeneratorTests.exe" -and
        $_.FullName -notmatch "\\CMakeFiles\\" -and
        $_.FullName -notmatch "_autogen\\"
    } |
    Where-Object {
        (Get-AuthenticodeSignature -LiteralPath $_.FullName).Status -eq "NotSigned"
    }

if (-not $targets) {
    Write-Host "Tous les binaires distribués sont déjà signés."
    exit 0
}

foreach ($target in $targets) {
    $arguments = @(
        "sign",
        "/sha1", $CertificateThumbprint,
        "/fd", "SHA256"
    )

    if (-not [string]::IsNullOrWhiteSpace($TimestampUrl)) {
        $arguments += @("/tr", $TimestampUrl, "/td", "SHA256")
    }

    $arguments += $target.FullName
    & $signTool.FullName @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Échec de la signature de $($target.FullName)."
    }
}

foreach ($target in $targets) {
    & $signTool.FullName verify /pa /v $target.FullName | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "Échec de la vérification de $($target.FullName)."
    }
}

Write-Host "$($targets.Count) binaires signés et vérifiés."
