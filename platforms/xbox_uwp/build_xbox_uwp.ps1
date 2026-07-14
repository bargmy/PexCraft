param(
    [string]$Configuration = "Release",
    [string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$dist = Join-Path $root "dist"
New-Item -ItemType Directory -Force -Path $dist | Out-Null

$buildInfoScript = Join-Path $root "tools\generate_build_info.py"
$python = (Get-Command python.exe -ErrorAction SilentlyContinue).Source
if ($python) {
    & $python $buildInfoScript --root $root
} else {
    $py = (Get-Command py.exe -ErrorAction SilentlyContinue).Source
    if (!$py) { throw "Python 3 is required to generate PexCraft build metadata." }
    & $py -3 $buildInfoScript --root $root
}
if ($LASTEXITCODE -ne 0) { throw "generate_build_info.py failed with $LASTEXITCODE" }

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

# The real engine uses zlib for save files, texturepack zip extraction, and MCPE batch/login payloads.
# Use the hosted runner vcpkg when available; the project consumes x64-uwp headers/libs via MSBuild.
$vcpkg = $env:VCPKG_ROOT
if (!$vcpkg -and (Test-Path "C:\vcpkg\vcpkg.exe")) { $vcpkg = "C:\vcpkg" }
if ($vcpkg -and (Test-Path (Join-Path $vcpkg "vcpkg.exe"))) {
    Write-Host "Installing UWP zlib through vcpkg: $vcpkg"
    & (Join-Path $vcpkg "vcpkg.exe") install zlib:x64-uwp
    if ($LASTEXITCODE -ne 0) { throw "vcpkg zlib:x64-uwp failed with $LASTEXITCODE" }
    $env:VcpkgInstalledDir = (Join-Path $vcpkg "installed") + "\\"
    $env:VcpkgRoot = $vcpkg
} else {
    Write-Warning "vcpkg not found; build will rely on system zlib headers/libs if present."
}

$zlibCandidates = @()
if ($vcpkg) {
    $zlibCandidates += Join-Path $vcpkg "installed\x64-uwp\lib"
    $zlibCandidates += Join-Path $vcpkg "packages\zlib_x64-uwp\lib"
}
$zlibLib = $null
$zlibPreferredNames = @("zlibstatic.lib", "zlib.lib", "z.lib")
foreach ($name in $zlibPreferredNames) {
    foreach ($dir in $zlibCandidates) {
        $candidate = Join-Path $dir $name
        if (Test-Path $candidate) { $zlibLib = $candidate; break }
    }
    if ($zlibLib) { break }
}
if (!$zlibLib) {
    $found = @()
    foreach ($dir in $zlibCandidates) { if (Test-Path $dir) { $found += (Get-ChildItem -Path $dir -Filter "*.lib" -File -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName }) } }
    throw "Could not find UWP zlib .lib after vcpkg install. Searched: $($zlibCandidates -join '; ') Found: $($found -join '; ')"
}
$zlibDir = Split-Path -Parent $zlibLib
Write-Host "Using zlib library: $zlibLib"

$packageSearchRoot = Join-Path $root "build\xbox_uwp"
if (Test-Path $packageSearchRoot) {
    Get-ChildItem -Path $packageSearchRoot -Recurse -Include *.appx,*.msix,*.appxbundle,*.msixbundle -File -ErrorAction SilentlyContinue |
        Remove-Item -Force -ErrorAction SilentlyContinue
}

Write-Host "Using MSBuild: $msbuild"
& $msbuild $project `
    /m `
    /restore `
    /p:Configuration=$Configuration `
    /p:Platform=$Platform `
    /p:AppxBundle=Never `
    /p:GenerateAppxPackageOnBuild=true `
    /p:AppxPackageSigningEnabled=false `
    /p:WindowsTargetPlatformMinVersion=10.0.14393.0 `
    /p:VcpkgTriplet=x64-uwp `
    /p:VcpkgEnabled=true `
    /p:VcpkgApplocalDeps=true `
    /p:ZlibLibrary="$zlibLib" `
    /p:ZlibLibraryDir="$zlibDir"

if ($LASTEXITCODE -ne 0) { throw "MSBuild failed with $LASTEXITCODE" }

$appx = Get-ChildItem -Path $packageSearchRoot -Recurse -Include *.appx,*.msix,*.appxbundle,*.msixbundle -File -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -match '^PexCraft(?:\.UWP)?' } |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1
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

    # App-local vcpkg DLLs are required when the selected .lib is an import library.
    # The old fallback copied only the EXE and assets, so the process could die in
    # the loader before wWinMain with STATUS_DLL_NOT_FOUND.
    $runtimeDirs = @()
    if ($vcpkg) {
        $runtimeDirs += Join-Path $vcpkg "installed\x64-uwp\bin"
        $runtimeDirs += Join-Path $vcpkg "packages\zlib_x64-uwp\bin"
    }
    $runtimeDlls = @()
    foreach ($runtimeDir in $runtimeDirs) {
        if (Test-Path $runtimeDir) {
            $runtimeDlls += Get-ChildItem -Path $runtimeDir -Filter "*.dll" -File -ErrorAction SilentlyContinue
        }
    }
    foreach ($dll in ($runtimeDlls | Sort-Object FullName -Unique)) {
        Copy-Item $dll.FullName (Join-Path $layout $dll.Name) -Force
        Write-Host "Packaged app-local dependency: $($dll.Name)"
    }

    $manifestSrc = Join-Path $PSScriptRoot "Package.appxmanifest"
    $manifestDst = Join-Path $layout "AppxManifest.xml"
    $manifestText = Get-Content -Raw $manifestSrc
    $manifestText = $manifestText.Replace('$targetnametoken$', 'PexCraft.UWP')
    $manifestText = $manifestText.Replace('$targetentrypoint$', 'PexCraftUWP.App')
    if ($manifestText -notmatch 'ProcessorArchitecture="x64"') {
        $manifestText = [regex]::Replace($manifestText, '<Identity\s+([^>]*?)/>', '<Identity $1 ProcessorArchitecture="x64" />', 1)
    }
    $manifestText | Out-File -FilePath $manifestDst -Encoding utf8

    # Generate the resource index expected by a normal Visual Studio UWP package.
    $makepri = Get-ChildItem -Path (Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\bin") -Recurse -Filter "makepri.exe" -File -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match "\\x64\\makepri\.exe$" } |
        Sort-Object FullName -Descending |
        Select-Object -First 1
    if (!$makepri) { throw "makepri.exe not found. Install the Windows SDK UWP packaging tools." }
    $priConfig = Join-Path $layout "priconfig.xml"
    & $makepri.FullName createconfig /cf $priConfig /dq en-US /o
    if ($LASTEXITCODE -ne 0) { throw "MakePri createconfig failed with $LASTEXITCODE" }
    & $makepri.FullName new /pr $layout /cf $priConfig /of (Join-Path $layout "resources.pri") /o
    if ($LASTEXITCODE -ne 0) { throw "MakePri new failed with $LASTEXITCODE" }
    Remove-Item $priConfig -Force -ErrorAction SilentlyContinue

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

$validator = Join-Path $PSScriptRoot "validate_xbox_uwp_package.ps1"
if (Test-Path $validator) {
    & $validator -PackagePath $appx.FullName
    if ($LASTEXITCODE -ne 0) { throw "Xbox UWP package validation failed with $LASTEXITCODE" }
}

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
