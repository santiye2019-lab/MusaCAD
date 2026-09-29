param(
    [string]$OutputDir = "$PSScriptRoot\..\..\local-release-secrets",
    [string]$GitHubRepo = "santiye2019-lab/MusaCAD",
    [switch]$SetGitHubSecrets
)

$ErrorActionPreference = "Stop"
$OutputDir = [System.IO.Path]::GetFullPath($OutputDir)
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

function Read-NewPassword([string]$Label) {
    while ($true) {
        $a = Read-Host "$Label (min 12 karakter)" -AsSecureString
        $b = Read-Host "$Label tekrar" -AsSecureString
        $pa = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($a)
        $pb = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($b)
        try {
            $sa = [Runtime.InteropServices.Marshal]::PtrToStringBSTR($pa)
            $sb = [Runtime.InteropServices.Marshal]::PtrToStringBSTR($pb)
            if ($sa.Length -lt 12) { Write-Host "Parola en az 12 karakter olmalı." -ForegroundColor Yellow; continue }
            if ($sa -ne $sb) { Write-Host "Parolalar eşleşmiyor." -ForegroundColor Yellow; continue }
            return $sa
        } finally {
            [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($pa)
            [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($pb)
        }
    }
}

function Set-GhSecret([string]$Name,[string]$Value) {
    $Value | & gh secret set $Name --repo $GitHubRepo
    if ($LASTEXITCODE -ne 0) { throw "GitHub secret ayarlanamadı: $Name" }
}

$keytool = (Get-Command keytool -ErrorAction Stop).Source
$openssl = Get-Command openssl -ErrorAction SilentlyContinue

$mainStore = Join-Path $OutputDir "musacad-release.jks"
$managerStore = Join-Path $OutputDir "musacad-manager-release.jks"
$trialPrivate = Join-Path $OutputDir "musacad-trial-private.pem"
$trialPublic = Join-Path $OutputDir "musacad-trial-public.pem"
$checklist = Join-Path $OutputDir "github-secret-names.txt"

if (Test-Path $mainStore) { throw "Dosya zaten var: $mainStore. Release keystore sessizce üzerine yazılmaz." }
if (Test-Path $managerStore) { throw "Dosya zaten var: $managerStore. Manager keystore sessizce üzerine yazılmaz." }

$mainStorePass = Read-NewPassword "MusaCAD ana APK keystore parolası"
$mainKeyPass = Read-NewPassword "MusaCAD ana APK key parolası"
$managerStorePass = Read-NewPassword "License Manager keystore parolası"
$managerKeyPass = Read-NewPassword "License Manager key parolası"

$mainAlias = "musacad-release"
$managerAlias = "musacad-manager-release"

& $keytool -genkeypair -v -storetype JKS -keystore $mainStore -storepass $mainStorePass -keypass $mainKeyPass -alias $mainAlias -keyalg RSA -keysize 4096 -validity 10000 -dname "CN=MusaCAD Release, OU=MusaCAD, O=MusaCAD, C=TR"
if ($LASTEXITCODE -ne 0) { throw "Ana release keystore oluşturulamadı." }

& $keytool -genkeypair -v -storetype JKS -keystore $managerStore -storepass $managerStorePass -keypass $managerKeyPass -alias $managerAlias -keyalg RSA -keysize 4096 -validity 10000 -dname "CN=MusaCAD License Manager, OU=MusaCAD, O=MusaCAD, C=TR"
if ($LASTEXITCODE -ne 0) { throw "License Manager keystore oluşturulamadı." }

if ($openssl) {
    & $openssl.Source genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:3072 -out $trialPrivate
    if ($LASTEXITCODE -ne 0) { throw "Trial private key oluşturulamadı." }
    & $openssl.Source pkey -in $trialPrivate -pubout -out $trialPublic
    if ($LASTEXITCODE -ne 0) { throw "Trial public key oluşturulamadı." }
} else {
    Write-Host "OpenSSL bulunamadı; trial keypair atlandı." -ForegroundColor Yellow
}

@(
  "MUSACAD_RELEASE_KEYSTORE_B64",
  "MUSACAD_KEYSTORE_PASSWORD",
  "MUSACAD_KEY_ALIAS",
  "MUSACAD_KEY_PASSWORD",
  "MUSACAD_TRIAL_API_URL",
  "MUSACAD_TRIAL_PUBLIC_KEY_PEM",
  "MUSACAD_AI_API_URL",
  "MUSACAD_AI_SESSION_URL",
  "MUSACAD_PLAY_VERIFY_URL",
  "MUSACAD_MANAGER_KEYSTORE_B64",
  "MUSACAD_MANAGER_KEYSTORE_PASSWORD",
  "MUSACAD_MANAGER_KEY_ALIAS",
  "MUSACAD_MANAGER_KEY_PASSWORD",
  "Repository variable: MUSACAD_PLAY_YEARLY_PRODUCT_ID=musacad_yearly_renewal"
) | Set-Content -Encoding UTF8 $checklist

if ($SetGitHubSecrets) {
    if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
        throw "GitHub CLI (gh) bulunamadı. -SetGitHubSecrets kullanmak için gh kurup 'gh auth login' çalıştırın."
    }

    $mainB64 = [Convert]::ToBase64String([IO.File]::ReadAllBytes($mainStore))
    $managerB64 = [Convert]::ToBase64String([IO.File]::ReadAllBytes($managerStore))
    try {
        Set-GhSecret "MUSACAD_RELEASE_KEYSTORE_B64" $mainB64
        Set-GhSecret "MUSACAD_KEYSTORE_PASSWORD" $mainStorePass
        Set-GhSecret "MUSACAD_KEY_ALIAS" $mainAlias
        Set-GhSecret "MUSACAD_KEY_PASSWORD" $mainKeyPass

        Set-GhSecret "MUSACAD_MANAGER_KEYSTORE_B64" $managerB64
        Set-GhSecret "MUSACAD_MANAGER_KEYSTORE_PASSWORD" $managerStorePass
        Set-GhSecret "MUSACAD_MANAGER_KEY_ALIAS" $managerAlias
        Set-GhSecret "MUSACAD_MANAGER_KEY_PASSWORD" $managerKeyPass

        if (Test-Path $trialPublic) {
            Set-GhSecret "MUSACAD_TRIAL_PUBLIC_KEY_PEM" ([IO.File]::ReadAllText($trialPublic))
        }
        "musacad_yearly_renewal" | & gh variable set MUSACAD_PLAY_YEARLY_PRODUCT_ID --repo $GitHubRepo
        if ($LASTEXITCODE -ne 0) { throw "GitHub repository variable ayarlanamadı." }
        Write-Host "Signing secret'ları GitHub'a doğrudan aktarıldı. Base64 keystore değeri diske yazılmadı." -ForegroundColor Green
    } finally {
        $mainB64 = $null
        $managerB64 = $null
    }
}

Write-Host ""
Write-Host "Production materyali yerel olarak oluşturuldu:" -ForegroundColor Green
Write-Host "  $mainStore"
Write-Host "  $managerStore"
if (Test-Path $trialPrivate) { Write-Host "  $trialPrivate  <-- PRIVATE / ÇOK GİZLİ" -ForegroundColor Yellow }
if (Test-Path $trialPublic) { Write-Host "  $trialPublic" }
Write-Host "  $checklist"
Write-Host ""
Write-Host "Parolalar ve Base64 keystore değerleri hiçbir template dosyasına yazılmadı." -ForegroundColor Green
Write-Host "JKS/private key dosyalarını iki ayrı şifreli offline konumda yedekleyin." -ForegroundColor Yellow
