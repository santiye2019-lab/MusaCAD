param(
    [Parameter(Mandatory=$true)][string]$D1DatabaseId,
    [Parameter(Mandatory=$true)][string]$PlayServiceAccountEmail,
    [string]$WorkerName = "musacad-trial-api",
    [string]$D1DatabaseName = "musacad-trials",
    [string]$TrialPrivateKeyPath = "$PSScriptRoot\..\..\local-release-secrets\musacad-trial-private.pem",
    [Parameter(Mandatory=$true)][string]$PlayServiceAccountPrivateKeyPath
)

$ErrorActionPreference = "Stop"
$root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\.."))
$worker = Join-Path $root "server\trial-worker"
$wrangler = Join-Path $worker "wrangler.toml"
$schema = Join-Path $worker "schema.sql"

if (-not (Get-Command node -ErrorAction SilentlyContinue)) { throw "Node.js bulunamadı." }
if (-not (Get-Command npm -ErrorAction SilentlyContinue)) { throw "npm bulunamadı." }
if (-not (Test-Path $TrialPrivateKeyPath)) { throw "Trial private key bulunamadı: $TrialPrivateKeyPath" }
if (-not (Test-Path $PlayServiceAccountPrivateKeyPath)) { throw "Google Play service-account private key bulunamadı: $PlayServiceAccountPrivateKeyPath" }

@"
name = "$WorkerName"
main = "src/index.js"
compatibility_date = "2026-09-01"

[vars]
MUSACAD_PACKAGE_NAME = "com.musa.cad"
MUSACAD_PLAY_YEARLY_PRODUCT_ID = "musacad_yearly_renewal"
MUSACAD_PLAY_SERVICE_ACCOUNT_EMAIL = "$PlayServiceAccountEmail"
MUSACAD_PLAY_ALLOW_LEGACY_UNBOUND = "false"

[[d1_databases]]
binding = "DB"
database_name = "$D1DatabaseName"
database_id = "$D1DatabaseId"
"@ | Set-Content -Encoding UTF8 $wrangler

Push-Location $worker
try {
    & npx --yes wrangler d1 execute $D1DatabaseName --remote --file schema.sql --yes
    if ($LASTEXITCODE -ne 0) { throw "D1 schema uygulanamadı." }

    Get-Content -Raw $TrialPrivateKeyPath | & npx --yes wrangler secret put MUSACAD_TRIAL_PRIVATE_KEY_PEM
    if ($LASTEXITCODE -ne 0) { throw "Trial private key Worker secret olarak kaydedilemedi." }

    Get-Content -Raw $PlayServiceAccountPrivateKeyPath | & npx --yes wrangler secret put MUSACAD_PLAY_SERVICE_ACCOUNT_PRIVATE_KEY_PEM
    if ($LASTEXITCODE -ne 0) { throw "Google Play private key Worker secret olarak kaydedilemedi." }

    & npx --yes wrangler deploy
    if ($LASTEXITCODE -ne 0) { throw "Cloudflare Worker deploy başarısız." }
} finally {
    Pop-Location
}

Write-Host ""
Write-Host "Cloudflare Worker deploy tamamlandı." -ForegroundColor Green
Write-Host "Wrangler çıktısındaki HTTPS Worker adresini kullanın:" -ForegroundColor Yellow
Write-Host "  MUSACAD_TRIAL_API_URL=https://<worker>/v1/trial/start"
Write-Host "  MUSACAD_PLAY_VERIFY_URL=https://<worker>/v1/play/verify"
