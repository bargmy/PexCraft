param(
    [Parameter(Mandatory = $true)]
    [string]$PackagePath
)

$ErrorActionPreference = "Stop"
$PackagePath = (Resolve-Path $PackagePath).Path
$failed = $false

function Fail([string]$Message) {
    $script:failed = $true
    Write-Error $Message
}

function Find-SdkTool([string]$Name) {
    $root = Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\bin"
    if (!(Test-Path $root)) { return $null }
    return Get-ChildItem -Path $root -Recurse -Filter $Name -File -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match "\\x64\\$([regex]::Escape($Name))$" } |
        Sort-Object FullName -Descending |
        Select-Object -First 1
}

$makeappx = Find-SdkTool "makeappx.exe"
if (!$makeappx) { throw "makeappx.exe not found" }

$temp = Join-Path ([System.IO.Path]::GetTempPath()) ("pexcraft-appx-check-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force -Path $temp | Out-Null
try {
    & $makeappx.FullName unpack /p $PackagePath /d $temp /o | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "MakeAppx unpack failed with $LASTEXITCODE" }

    $manifestPath = Join-Path $temp "AppxManifest.xml"
    if (!(Test-Path $manifestPath)) { Fail "AppxManifest.xml is missing" }
    else {
        [xml]$manifest = Get-Content -Raw $manifestPath
        $identity = $manifest.Package.Identity
        $app = $manifest.Package.Applications.Application
        if ($identity.ProcessorArchitecture -ne "x64") {
            Fail "Manifest ProcessorArchitecture must be x64; found '$($identity.ProcessorArchitecture)'"
        }
        if (!$app.Executable -or $app.Executable -match '\$target') {
            Fail "Manifest Executable is unresolved or empty: '$($app.Executable)'"
        }
        if (!$app.EntryPoint -or $app.EntryPoint -match '\$target') {
            Fail "Manifest EntryPoint is unresolved or empty: '$($app.EntryPoint)'"
        }
        $exePath = Join-Path $temp $app.Executable
        if (!(Test-Path $exePath)) { Fail "Manifest executable is absent from package: $($app.Executable)" }
        else {
            Write-Host "Executable: $($app.Executable)"
            $dumpbin = (Get-Command dumpbin.exe -ErrorAction SilentlyContinue).Source
            if (!$dumpbin) {
                $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
                if (Test-Path $vswhere) {
                    $install = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
                    if ($install) {
                        $dumpbin = Get-ChildItem -Path (Join-Path $install "VC\Tools\MSVC") -Recurse -Filter dumpbin.exe -File -ErrorAction SilentlyContinue |
                            Where-Object { $_.FullName -match '\\Hostx64\\x64\\dumpbin\.exe$' } |
                            Sort-Object FullName -Descending | Select-Object -First 1 -ExpandProperty FullName
                    }
                }
            }
            if ($dumpbin) {
                $deps = & $dumpbin /nologo /dependents $exePath 2>&1
                $deps | Out-Host
                $zlibImports = $deps | Select-String -Pattern '^\s*(zlib[^\s]*\.dll)\s*$' -AllMatches
                foreach ($hit in $zlibImports) {
                    foreach ($match in $hit.Matches) {
                        $dllName = $match.Groups[1].Value
                        if (!(Test-Path (Join-Path $temp $dllName))) {
                            Fail "Executable imports $dllName, but that DLL is not app-local in the package"
                        }
                    }
                }
            } else {
                Write-Warning "dumpbin.exe not found; native DLL imports were not checked"
            }
        }
    }

    if (!(Test-Path (Join-Path $temp "resources.pri"))) {
        Write-Warning "resources.pri is missing. Literal manifest strings may still work, but this is not a normal VS-generated UWP layout."
    } else {
        Write-Host "resources.pri: present"
    }

    $localDlls = Get-ChildItem -Path $temp -Filter *.dll -File -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Name
    if ($localDlls) { Write-Host ("App-local DLLs: " + ($localDlls -join ", ")) }
    else { Write-Host "App-local DLLs: none" }

    if ($failed) { exit 1 }
    Write-Host "Xbox UWP package validation passed: $PackagePath"
    exit 0
} finally {
    Remove-Item $temp -Recurse -Force -ErrorAction SilentlyContinue
}
