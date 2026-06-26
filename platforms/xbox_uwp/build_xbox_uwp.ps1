param(
    [string]$Configuration = "Release",
    [string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$dist = Join-Path $root "dist"
$pkg = Join-Path $dist "pexcraft-xbox-one-uwp-devmode"
New-Item -ItemType Directory -Force -Path $pkg | Out-Null

# Keep the package layout deterministic for Telegram/artifact uploads even on
# CI machines where the UWP C++ workload is unavailable.
Copy-Item (Join-Path $PSScriptRoot "Package.appxmanifest") $pkg -Force
Copy-Item (Join-Path $PSScriptRoot "README.md") $pkg -Force
New-Item -ItemType Directory -Force -Path (Join-Path $pkg "Source") | Out-Null
Copy-Item (Join-Path $root "src") (Join-Path $pkg "Source\src") -Recurse -Force
Copy-Item (Join-Path $root "multiplayer") (Join-Path $pkg "Source\multiplayer") -Recurse -Force
Copy-Item (Join-Path $root "platforms\xbox_uwp") (Join-Path $pkg "Source\platforms\xbox_uwp") -Recurse -Force
if (Test-Path (Join-Path $root "assets")) { Copy-Item (Join-Path $root "assets") (Join-Path $pkg "assets") -Recurse -Force }

@"
PexCraft Xbox One UWP Dev Mode package layout
Configuration: $Configuration
Platform: $Platform
Writable storage: ApplicationData.Current.LocalFolder\PexCraft
Minimum target: Windows.Universal 10.0.14393.0
Virtual keyboard: engine-side controller keyboard, no system IME required
"@ | Out-File -Encoding utf8 (Join-Path $pkg "BUILD_NOTES.txt")

$zip = Join-Path $dist "pexcraft-xbox-one-uwp-devmode.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $pkg "*") -DestinationPath $zip -Force
Write-Host "XBOX_UWP_ZIP=$zip"
