param(
    [string]$OutputDir = "$PSScriptRoot\..\..\local-release-secrets"
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

function Convert-ToBase64([string]$Path) {
    return [Convert]::ToBase64String([IO.File]::ReadAllBytes($Path))
}

$keytool = (Get-Command keytool -ErrorAction Stop).Source
$openssl = Get-Command openssl -ErrorAction SilentlyContinue

$mainStore = Join-Path $OutputDir "musacad-release.jks"
$managerStore = Join-Path $OutputDir "musacad-manager-release.jks"
$trialPrivate = Join-Path $OutputDir "musacad-trial-private.pem"
$trialPublic = Join-Path $OutputDir "musacad-trial-public.pem"
$envFile = Join-Path $OutputDir "github-secrets-template.txt"

$mainStorePass = Read-NewPassword "MusaCAD ana APK keystore parolası"
$mainKeyPass = Read-NewPassword "MusaCAD ana APK key parolası"
$managerStorePass = Read-NewPassword "License Manager keystore parolası"
$managerKeyPass = Read-NewPassword "License Manager key parolası"

$mainAlias = "musacad-release"
$managerAlias = "musacad-manager-release"

if (Test-Path $mainStore) { throw "Dosya zaten var: $mainStore. Mevcut release keystore'u asla sessizce üzerine yazılmaz." }
if (Test-Path $managerStore) { throw "Dosya zaten var: $managerStore. Mevcut manager keystore'u asla sessizce üzerine yazılmaz." }

& $keytool -genkeypair -v -keystore $mainStore -storepass $mainStorePass -keypass $mainKeyPass -alias $mainAlias -keyalg RSA -keysize 4096 -validity 10000 -dname "CN=MusaCAD Release, OU=MusaCAD, O=MusaCAD, C=TR"
if ($LASTEXITCODE -ne 0) { throw "Ana release keystore oluşturulamadı." }

& $keytool -genkeypair -v -keystore $managerStore -storepass $managerStorePass -keypass $managerKeyPass -alias $managerAlias -keyalg RSA -keysize 4096 -validity 10000 -dname "CN=MusaCAD License Manager, OU=MusaCAD, O=MusaCAD, C=TR"
if ($LASTEXITCODE -ne 0) { throw "License Manager keystore oluşturulamadı." }

if ($openssl) {
    & $openssl.Source genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:3072 -out $trialPrivate
    if ($LASTEXITCODE -ne 0) { throw "Trial private key oluşturulamadı." }
    & $openssl.Source pkey -in $trialPrivate -pubout -out $trialPublic
    if ($LASTEXITCODE -ne 0) { throw "Trial public key oluşturulamadı." }
} else {
    Write-Host "OpenSSL bulunamadı. Trial RSA keypair otomatik oluşturulmadı." -ForegroundColor Yellow
    Write-Host "OpenSSL kurduktan sonra scripti tekrar çalıştırabilir veya trial keypair'i ayrı güvenli ortamda oluşturabilirsiniz." -ForegroundColor Yellow
}

$lines = @()
$lines += "# BU DOSYAYI GITHUB'A COMMIT ETMEYİN."
$lines += "# GitHub > Settings > Secrets and variables > Actions alanına değerleri ayrı ayrı girin."
$lines += ""
$lines += "MUSACAD_RELEASE_KEYSTORE_B64=$(Convert-ToBase64 $mainStore)"
$lines += "MUSACAD_KEYSTORE_PASSWORD=<MAIN_STORE_PASSWORD>"
$lines += "MUSACAD_KEY_ALIAS=$mainAlias"
$lines += "MUSACAD_KEY_PASSWORD=<MAIN_KEY_PASSWORD>"
$lines += ""
$lines += "MUSACAD_MANAGER_KEYSTORE_B64=$(Convert-ToBase64 $managerStore)"
$lines += "MUSACAD_MANAGER_KEYSTORE_PASSWORD=<MANAGER_STORE_PASSWORD>"
$lines += "MUSACAD_MANAGER_KEY_ALIAS=$managerAlias"
$lines += "MUSACAD_MANAGER_KEY_PASSWORD=<MANAGER_KEY_PASSWORD>"
$lines += ""
if (Test-Path $trialPublic) {
    $lines += "MUSACAD_TRIAL_PUBLIC_KEY_PEM=<contents of musacad-trial-public.pem>"
}
$lines += "MUSACAD_TRIAL_API_URL=https://<your-worker>/v1/trial/start"
$lines += "MUSACAD_PLAY_VERIFY_URL=https://<your-worker>/v1/play/verify"
$lines += "MUSACAD_PLAY_YEARLY_PRODUCT_ID=musacad_yearly_renewal"
$lines | Set-Content -Encoding UTF8 $envFile

Write-Host ""
Write-Host "Yerel production materyali oluşturuldu:" -ForegroundColor Green
Write-Host "  $mainStore"
Write-Host "  $managerStore"
if (Test-Path $trialPrivate) { Write-Host "  $trialPrivate  <-- PRIVATE / ÇOK GİZLİ" -ForegroundColor Yellow }
if (Test-Path $trialPublic) { Write-Host "  $trialPublic" }
Write-Host "  $envFile"
Write-Host ""
Write-Host "ÖNEMLİ: Parolalar bu template dosyasına yazılmadı. Onları parola yöneticinizde saklayın." -ForegroundColor Yellow
Write-Host "Private key ve JKS dosyalarını GitHub'a commit etmeyin." -ForegroundColor Yellow
