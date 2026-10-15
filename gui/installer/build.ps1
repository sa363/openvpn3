# Builds the OpenVPN3 MSI from an existing core and GUI build.
#
#   powershell -ExecutionPolicy Bypass -File gui\installer\build.ps1
#
# Needs WiX 5 (dotnet tool install --global wix --version 5.0.2).

[CmdletBinding()]
param(
    # core build with omicli, omicliagent and ovpnagent
    [string]$CoreBuild,
    # GUI build, after windeployqt
    [string]$GuiBuild,
    [string]$Config = "Release",
    [string]$OutDir,
    [string]$Wix = "wix"
)

$ErrorActionPreference = "Stop"
$WixVersion = "5.0.2"

function Resolve-Full([string]$Path) {
    return [System.IO.Path]::GetFullPath($Path)
}

# defaults relative to the repository; $PSScriptRoot is not set yet in param() on PowerShell 5.1
$repo = Resolve-Full (Join-Path $PSScriptRoot "..\..")
if (-not $CoreBuild) { $CoreBuild = Join-Path $repo "build\msvc\amd64" }
if (-not $GuiBuild) { $GuiBuild = Join-Path $repo "build\gui" }
if (-not $OutDir) { $OutDir = Join-Path $repo "build\installer" }

$CoreBuild = Resolve-Full $CoreBuild
$GuiBuild = Resolve-Full $GuiBuild
$OutDir = Resolve-Full $OutDir
$stage = Join-Path $OutDir "stage"

# version of the GUI project
$cmake = Get-Content (Join-Path $PSScriptRoot "..\CMakeLists.txt") -Raw
if ($cmake -notmatch 'project\(ovpn3-gui VERSION ([0-9.]+)') {
    throw "Cannot find the project version in gui/CMakeLists.txt"
}
$version = $Matches[1]

Write-Host "Staging OpenVPN3 $version in $stage"
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Force $stage | Out-Null

# GUI with its Qt runtime; the build already copied the workers next to it
$guiDir = Join-Path $GuiBuild $Config
if (-not (Test-Path (Join-Path $guiDir "ovpn3-gui.exe"))) {
    throw "ovpn3-gui.exe not found in $guiDir, build the GUI first"
}
Copy-Item (Join-Path $guiDir "*") $stage -Recurse -Exclude "*.pdb", "*.ilk", "*.exp", "*.lib", "ovpnagent.exe", "agent.log"

# workers and agent from the core build, in case the GUI build has stale copies
$omiDir = Join-Path $CoreBuild "openvpn\omi\$Config"
$agentDir = Join-Path $CoreBuild "openvpn\ovpnagent\win\$Config"
foreach ($f in @("$omiDir\omicliagent.exe", "$omiDir\omicli.exe", "$agentDir\ovpnagent.exe")) {
    if (-not (Test-Path $f)) { throw "$f not found, build the core first" }
    Copy-Item $f $stage -Force
}
foreach ($dir in @($omiDir, $agentDir)) {
    Get-ChildItem $dir -Filter *.dll | Copy-Item -Destination $stage -Force
}

# MSVC runtime, deployed next to the binaries so no separate redistributable is needed
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$crt = Get-ChildItem (Join-Path $vs "VC\Redist\MSVC") -Directory |
    Where-Object { $_.Name -match '^\d' } | Sort-Object Name -Descending |
    ForEach-Object { Join-Path $_.FullName "x64\Microsoft.VC143.CRT" } |
    Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $crt) { throw "MSVC runtime (Microsoft.VC143.CRT) not found under $vs" }
Write-Host "MSVC runtime from $crt"
Get-ChildItem $crt -Filter *.dll | Copy-Item -Destination $stage -Force

# license for the license dialog
$license = Get-Content (Join-Path $repo "LICENSE.md") -Raw
$escaped = $license -replace '\\', '\\' -replace '\{', '\{' -replace '\}', '\}'
$escaped = [regex]::Replace($escaped, '[^\x00-\x7f]', { param($m) '\u' + [int][char]$m.Value + '?' })
$escaped = $escaped -replace "`r?`n", "\par`r`n"
$rtf = "{\rtf1\ansi\deff0{\fonttbl{\f0 Consolas;}}\f0\fs16`r`n$escaped`r`n}"
$licenseRtf = Join-Path $OutDir "license.rtf"
[System.IO.File]::WriteAllText($licenseRtf, $rtf, [System.Text.Encoding]::ASCII)

# WiX extensions
& $Wix extension add -g "WixToolset.UI.wixext/$WixVersion" "WixToolset.Util.wixext/$WixVersion"
if ($LASTEXITCODE) { throw "wix extension add failed" }

$msi = Join-Path $OutDir "OpenVPN3-$version-x64.msi"
& $Wix build (Join-Path $PSScriptRoot "Package.wxs") `
    -arch x64 `
    -culture ru-RU `
    -ext "WixToolset.UI.wixext/$WixVersion" `
    -ext "WixToolset.Util.wixext/$WixVersion" `
    -d "StageDir=$stage" `
    -d "Version=$version" `
    -d "LicenseRtf=$licenseRtf" `
    -o $msi
if ($LASTEXITCODE) { throw "wix build failed" }

Write-Host "Built $msi"
