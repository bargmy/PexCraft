param(
    [string]$Configuration = "Release",
    [string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$dist = Join-Path $root "dist"
New-Item -ItemType Directory -Force -Path $dist | Out-Null

$project = Join-Path $PSScriptRoot "PexCraft.UWP.vcxproj"
if (!(Test-Path $project)) { throw "missing UWP project: $project" }

$msbuild = (Get-Command msbuild.exe -ErrorAction SilentlyContinue).Source
if (!$msbuild) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $install = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
        if ($install) { $msbuild = Join-Path $install "MSBuild\Current\Bin\MSBuild.exe" }
    }
}
if (!$msbuild -or !(Test-Path $msbuild)) { throw "MSBuild not found" }

# C++/WinRT shell does not use /ZW, so no platform.winmd /AI path hack is needed.

Write-Host "Using MSBuild: $msbuild"
& $msbuild $project `
    /m `
    /restore `
    /p:Configuration=$Configuration `
    /p:Platform=$Platform `
    /p:AppxBundle=Never `
    /p:GenerateAppxPackageOnBuild=true `
    /p:AppxPackageSigningEnabled=false `
    /p:WindowsTargetPlatformMinVersion=10.0.14393.0

if ($LASTEXITCODE -ne 0) { throw "MSBuild failed with $LASTEXITCODE" }

$appx = Get-ChildItem -Path (Join-Path $root "build") -Recurse -Include *.appx,*.msix,*.appxbundle,*.msixbundle -File | Sort-Object LastWriteTime -Descending | Select-Object -First 1
$packagingMode = "MSBuild"
$certOut = $null

if (!$appx) {
    Write-Host "MSBuild produced the UWP executable but no appx/msix. Falling back to MakeAppx packaging."

    $exe = Get-ChildItem -Path (Join-Path $root "build\xbox_uwp") -Recurse -Filter "PexCraft.UWP.exe" -File | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (!$exe) { throw "UWP build succeeded but PexCraft.UWP.exe was not found; cannot package." }

    $makeappx = Get-ChildItem -Path (Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\bin") -Recurse -Filter "makeappx.exe" -File -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match "\\x64\\makeappx\.exe$" } |
        Sort-Object FullName -Descending |
        Select-Object -First 1
    if (!$makeappx) { throw "makeappx.exe not found. Install the Windows 10 SDK app packaging tools on the runner." }

    $layout = Join-Path $root "build\xbox_uwp\appx_layout"
    if (Test-Path $layout) { Remove-Item $layout -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $layout | Out-Null

    Copy-Item $exe.FullName (Join-Path $layout "PexCraft.UWP.exe") -Force
    Copy-Item (Join-Path $PSScriptRoot "Assets") (Join-Path $layout "Assets") -Recurse -Force

    $manifestSrc = Join-Path $PSScriptRoot "Package.appxmanifest"
    $manifestDst = Join-Path $layout "AppxManifest.xml"
    $manifestText = Get-Content -Raw $manifestSrc
    $manifestText = $manifestText.Replace('$targetnametoken$', 'PexCraft.UWP')
    $manifestText = $manifestText.Replace('$targetentrypoint$', 'PexCraftUwp.App')
    $manifestText | Out-File -FilePath $manifestDst -Encoding utf8

    $manualAppx = Join-Path $root "build\xbox_uwp\PexCraft.UWP.appx"
    if (Test-Path $manualAppx) { Remove-Item $manualAppx -Force }
    & $makeappx.FullName pack /d $layout /p $manualAppx /o
    if ($LASTEXITCODE -ne 0) { throw "MakeAppx failed with $LASTEXITCODE" }

    $signtool = Get-ChildItem -Path (Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\bin") -Recurse -Filter "signtool.exe" -File -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match "\\x64\\signtool\.exe$" } |
        Sort-Object FullName -Descending |
        Select-Object -First 1
    if ($signtool) {
        try {
            $cert = New-SelfSignedCertificate `
                -Type Custom `
                -Subject "CN=PexCraft" `
                -KeyUsage DigitalSignature `
                -FriendlyName "PexCraft Xbox UWP Dev Mode Test Certificate" `
                -CertStoreLocation "Cert:\CurrentUser\My" `
                -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3", "2.5.29.19={text}")
            $cerPath = Join-Path $dist "pexcraft-xbox-one-uwp-devmode-testcert.cer"
            Export-Certificate -Cert $cert -FilePath $cerPath | Out-Null
            & $signtool.FullName sign /fd SHA256 /sha1 $cert.Thumbprint $manualAppx
            if ($LASTEXITCODE -eq 0) {
                $certOut = $cerPath
                Write-Host "Signed fallback appx with temporary CI test certificate: $cerPath"
            } else {
                Write-Warning "SignTool failed with $LASTEXITCODE; appx was created but may need signing before install."
            }
        } catch {
            Write-Warning "Could not generate/sign with a test certificate: $_"
        }
    } else {
        Write-Warning "signtool.exe not found; appx was created but may need signing before install."
    }

    $appx = Get-Item $manualAppx
    $packagingMode = "MakeAppx fallback"
}

if (!$appx) { throw "UWP build succeeded but no appx/msix package was produced. Fake source zips are intentionally disabled." }

$out = Join-Path $dist "pexcraft-xbox-one-uwp-devmode$($appx.Extension)"
Copy-Item $appx.FullName $out -Force

$notes = Join-Path $dist "pexcraft-xbox-one-uwp-devmode-build-notes.txt"
@"
PexCraft Xbox One UWP Dev Mode build
Configuration: $Configuration
Platform: $Platform
Package: $out
Packaging mode: $packagingMode
Minimum target: Windows.Universal 10.0.14393.0
Writable storage: ApplicationData.Current.LocalFolder\PexCraft
Test certificate: $certOut
This job runs MSBuild and requires a real UWP package. It no longer uploads a source-layout zip as a fake build.
"@ | Out-File -Encoding utf8 $notes

Write-Host "XBOX_UWP_PACKAGE=$out"
