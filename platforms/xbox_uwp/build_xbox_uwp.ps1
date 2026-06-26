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


function Find-UwpWinmdUsingDirs {
    $roots = @()
    if ($env:ProgramFiles) { $roots += (Join-Path $env:ProgramFiles "Microsoft Visual Studio") }
    if (${env:ProgramFiles(x86)}) {
        $roots += (Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio")
        $roots += (Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10")
    }

    $dirs = New-Object System.Collections.Generic.List[string]
    foreach ($rootPath in $roots) {
        if (!(Test-Path $rootPath)) { continue }
        try {
            Get-ChildItem -Path $rootPath -Filter platform.winmd -Recurse -ErrorAction SilentlyContinue |
                Select-Object -First 8 | ForEach-Object { $dirs.Add($_.DirectoryName) }
        } catch {}
    }

    if (${env:ProgramFiles(x86)}) {
        $kits = Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10"
        $union = Join-Path $kits "UnionMetadata"
        if (Test-Path $union) {
            $dirs.Add($union)
            Get-ChildItem -Path $union -Directory -ErrorAction SilentlyContinue |
                Sort-Object Name -Descending | Select-Object -First 3 | ForEach-Object { $dirs.Add($_.FullName) }
        }
        $refs = Join-Path $kits "References"
        foreach ($contract in @("Windows.Foundation.UniversalApiContract", "Windows.Foundation.FoundationContract")) {
            $contractRoot = Join-Path $refs $contract
            if (Test-Path $contractRoot) {
                Get-ChildItem -Path $contractRoot -Directory -ErrorAction SilentlyContinue |
                    Sort-Object Name -Descending | Select-Object -First 3 | ForEach-Object { $dirs.Add($_.FullName) }
            }
        }
    }

    $unique = @()
    foreach ($d in $dirs) {
        if ($d -and (Test-Path $d) -and ($unique -notcontains $d)) { $unique += $d }
    }
    return $unique
}

$uwpUsingDirs = Find-UwpWinmdUsingDirs
if ($uwpUsingDirs.Count -eq 0) {
    Write-Warning "No explicit UWP WinMD paths were found. MSBuild property paths will still be used."
} else {
    Write-Host "UWP WinMD /AI paths:"
    $uwpUsingDirs | ForEach-Object { Write-Host "  $_" }
    $env:LIBPATH = (($uwpUsingDirs + @($env:LIBPATH)) -join ";")
}
# Do not pass semicolon-delimited WinMD paths through /p: on the MSBuild
# command line. MSBuild treats semicolons in /p: as property separators,
# so a value like C:\foo;C:\bar becomes a broken command-line switch.
# Instead, generate a tiny props file and import it from the vcxproj.
function Escape-XmlAttr([string]$value) {
    if ($null -eq $value) { return "" }
    return $value.Replace("&", "&amp;").Replace("<", "&lt;").Replace(">", "&gt;").Replace('"', "&quot;").Replace("'", "&apos;")
}

$uwpExtraUsingDirs = ($uwpUsingDirs -join ";")
$generatedProps = Join-Path $PSScriptRoot "GeneratedWinmd.props"
$escapedDirs = Escape-XmlAttr $uwpExtraUsingDirs
@"
<?xml version="1.0" encoding="utf-8"?>
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup>
    <UwpExtraUsingDirs>$escapedDirs</UwpExtraUsingDirs>
  </PropertyGroup>
</Project>
"@ | Out-File -Encoding utf8 $generatedProps
Write-Host "Generated WinMD props: $generatedProps"

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
