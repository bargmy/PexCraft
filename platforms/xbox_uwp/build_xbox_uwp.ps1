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

# The real engine uses zlib for save files, texturepack zip extraction, and MCPE batch/login payloads.
# Use the hosted runner vcpkg when available; the project consumes x64-uwp headers/libs via explicit MSBuild properties.
$vcpkg = $env:VCPKG_ROOT
if (!$vcpkg -and (Test-Path "C:\vcpkg\vcpkg.exe")) { $vcpkg = "C:\vcpkg" }
$triplet = "x64-uwp"
$zlibLibDir = $null
$zlibLib = $null
$zlibIncludeDir = $null

function Find-ZlibLibrary([string]$VcpkgRoot, [string]$Triplet) {
    $candidateFiles = @(
        (Join-Path $VcpkgRoot "installed\$Triplet\lib\zlib.lib"),
        (Join-Path $VcpkgRoot "installed\$Triplet\lib\zlibstatic.lib"),
        (Join-Path $VcpkgRoot "packages\zlib_$Triplet\lib\zlib.lib"),
        (Join-Path $VcpkgRoot "packages\zlib_$Triplet\lib\zlibstatic.lib"),
        (Join-Path $VcpkgRoot "packages\zlib_$Triplet\lib\z.lib")
    )
    foreach ($candidate in $candidateFiles) {
        if (Test-Path $candidate) { return (Resolve-Path $candidate).Path }
    }

    $candidateDirs = @(
        (Join-Path $VcpkgRoot "installed\$Triplet\lib"),
        (Join-Path $VcpkgRoot "packages\zlib_$Triplet\lib")
    ) | Where-Object { Test-Path $_ }

    $libs = foreach ($dir in $candidateDirs) {
        Get-ChildItem -Path $dir -File -Filter "*.lib" -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -notmatch "\\debug\\" }
    }

    $preferred = $libs | Where-Object { $_.Name -match "^(zlib|zlibstatic|z)\.lib$" } | Select-Object -First 1
    if ($preferred) { return $preferred.FullName }

    $any = $libs | Select-Object -First 1
    if ($any) { return $any.FullName }

    return $null
}

function Find-ZlibIncludeDir([string]$VcpkgRoot, [string]$Triplet) {
    $candidateDirs = @(
        (Join-Path $VcpkgRoot "installed\$Triplet\include"),
        (Join-Path $VcpkgRoot "packages\zlib_$Triplet\include")
    )
    foreach ($dir in $candidateDirs) {
        if (Test-Path (Join-Path $dir "zlib.h")) { return (Resolve-Path $dir).Path }
    }
    return $null
}

if ($vcpkg -and (Test-Path (Join-Path $vcpkg "vcpkg.exe"))) {
    Write-Host "Installing UWP zlib through vcpkg: $vcpkg"
    & (Join-Path $vcpkg "vcpkg.exe") install "zlib:$triplet"
    if ($LASTEXITCODE -ne 0) { throw "vcpkg zlib:$triplet failed with $LASTEXITCODE" }

    $zlibLib = Find-ZlibLibrary -VcpkgRoot $vcpkg -Triplet $triplet
    $zlibIncludeDir = Find-ZlibIncludeDir -VcpkgRoot $vcpkg -Triplet $triplet

    if (!$zlibLib) {
        $debugRoots = @(
            (Join-Path $vcpkg "installed\$triplet"),
            (Join-Path $vcpkg "packages\zlib_$triplet")
        ) | Where-Object { Test-Path $_ }
        $found = foreach ($rootPath in $debugRoots) {
            Get-ChildItem -Path $rootPath -Recurse -File -Filter "*.lib" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName
        }
        $list = ($found | Select-Object -First 50) -join "`n"
        throw "vcpkg installed zlib:$triplet, but no release zlib .lib could be found under installed or packages. Found .lib files:`n$list"
    }

    if (!$zlibIncludeDir) {
        throw "vcpkg installed zlib:$triplet, but zlib.h was not found under installed or packages."
    }

    $zlibLibDir = Split-Path -Parent $zlibLib

    # Make the dependency visible both to MSBuild properties and to link.exe's LIB search path.
    # Some GitHub-hosted vcpkg runs leave the usable UWP .lib in packages\zlib_x64-uwp\lib
    # instead of installed\x64-uwp\lib, so always pass the exact file path discovered above.
    $env:VCPKG_ROOT = $vcpkg
    $env:VcpkgRoot = $vcpkg
    $env:VcpkgInstalledDir = (Join-Path $vcpkg "installed") + "\"
    $env:LIB = "$zlibLibDir;$env:LIB"
    $env:INCLUDE = "$zlibIncludeDir;$env:INCLUDE"
    Write-Host "Using zlib include dir: $zlibIncludeDir"
    Write-Host "Using zlib library: $zlibLib"
} else {
    Write-Warning "vcpkg not found; build will rely on system zlib headers/libs if present."
}


Write-Host "Using MSBuild: $msbuild"
$msbuildArgs = @(
    $project,
    "/m",
    "/restore",
    "/p:Configuration=$Configuration",
    "/p:Platform=$Platform",
    "/p:AppxBundle=Never",
    "/p:GenerateAppxPackageOnBuild=true",
    "/p:AppxPackageSigningEnabled=false",
    "/p:WindowsTargetPlatformMinVersion=10.0.14393.0",
    "/p:VcpkgTriplet=x64-uwp",
    "/p:VcpkgEnabled=true"
)

if ($vcpkg) {
    $msbuildArgs += "/p:VcpkgRoot=$vcpkg"
    $msbuildArgs += "/p:VcpkgInstalledDir=$((Join-Path $vcpkg 'installed') + '\')"
}
if ($zlibLibDir) { $msbuildArgs += "/p:ZlibLibraryDir=$zlibLibDir" }
if ($zlibIncludeDir) { $msbuildArgs += "/p:ZlibIncludeDir=$zlibIncludeDir" }
if ($zlibLib) { $msbuildArgs += "/p:ZlibLibrary=$zlibLib" }

& $msbuild @msbuildArgs

if ($LASTEXITCODE -ne 0) { throw "MSBuild failed with $LASTEXITCODE" }


# Always create a controlled appx layout ourselves after MSBuild.
# The MSBuild-generated package can leave activation-critical placeholders or
# omit app-local VC runtime files on CI/loose dev-mode installs.  The manual
# layout below has a literal manifest, x64 architecture, and copied runtime DLLs.
$appx = $null
$packagingMode = "MakeAppx controlled layout"
$certOut = $null

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
if ($manifestText -notmatch 'ProcessorArchitecture=') {
    $manifestText = $manifestText -replace '<Identity ([^>]+?) />', '<Identity $1 ProcessorArchitecture="x64" />'
}
$manifestText | Out-File -FilePath $manifestDst -Encoding utf8

function Copy-UwpVcRuntimeDlls([string]$LayoutDir) {
    $wanted = @(
        'vcruntime140_app.dll',
        'vcruntime140_1_app.dll',
        'msvcp140_app.dll',
        'concrt140_app.dll'
    )

    $roots = @()
    if ($env:VCToolsRedistDir) { $roots += $env:VCToolsRedistDir }
    if ($env:VCINSTALLDIR) { $roots += (Join-Path $env:VCINSTALLDIR 'Redist') }
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $vsInstall = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($vsInstall) {
            $roots += (Join-Path $vsInstall 'VC\Redist')
            $roots += (Join-Path $vsInstall 'VC\Tools\MSVC')
        }
    }
    $roots += (Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio')
    $roots = $roots | Where-Object { $_ -and (Test-Path $_) } | Select-Object -Unique

    $copied = New-Object System.Collections.Generic.List[string]
    foreach ($dll in $wanted) {
        $match = $null
        foreach ($rootPath in $roots) {
            $candidates = Get-ChildItem -Path $rootPath -Recurse -File -Filter $dll -ErrorAction SilentlyContinue |
                Where-Object { $_.FullName -match '\\x64\\' -and ($_.FullName -match 'onecore|uwp|appx|Microsoft\.VC') } |
                Sort-Object FullName -Descending
            $match = $candidates | Select-Object -First 1
            if ($match) { break }
        }
        if ($match) {
            Copy-Item $match.FullName (Join-Path $LayoutDir $dll) -Force
            $copied.Add($match.FullName) | Out-Null
            Write-Host "Copied UWP VC runtime: $($match.FullName)"
        } else {
            Write-Warning "UWP VC runtime DLL not found on builder: $dll"
        }
    }
    return $copied
}

$runtimeCopied = Copy-UwpVcRuntimeDlls -LayoutDir $layout
$runtimeCopied | Out-File -FilePath (Join-Path $layout 'copied-vc-runtime-files.txt') -Encoding utf8

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
            Write-Host "Signed controlled appx with temporary CI test certificate: $cerPath"
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
