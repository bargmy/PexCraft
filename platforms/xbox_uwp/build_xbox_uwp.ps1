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
if (!$appx) { throw "UWP build succeeded but no appx/msix package was produced. Fake source zips are intentionally disabled." }

$out = Join-Path $dist "pexcraft-xbox-one-uwp-devmode$($appx.Extension)"
Copy-Item $appx.FullName $out -Force

$notes = Join-Path $dist "pexcraft-xbox-one-uwp-devmode-build-notes.txt"
@"
PexCraft Xbox One UWP Dev Mode build
Configuration: $Configuration
Platform: $Platform
Package: $out
Minimum target: Windows.Universal 10.0.14393.0
Writable storage: ApplicationData.Current.LocalFolder\PexCraft
This job now runs MSBuild and requires a real UWP package. It no longer uploads a source-layout zip as a fake build.
"@ | Out-File -Encoding utf8 $notes

Write-Host "XBOX_UWP_PACKAGE=$out"
