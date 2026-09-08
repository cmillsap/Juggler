# Juggler - install and uninstall.
#
# A .scr does not have to live in System32, and putting it there is the reason
# screensavers have a reputation with anti-malware: an unsigned executable
# copying itself into a system directory is the shape of something hostile.
# Windows will run one from anywhere, so this installs per-user under LocalAppData
# and points the registry at it. No elevation, nothing outside the user's own
# profile, and Uninstall puts it all back.
#
#   .\install.ps1                 install and make it the active screensaver
#   .\install.ps1 -Activate:$false   install without selecting it
#   .\install.ps1 -Uninstall      remove it again
#
# Juggler.scr, dxcompiler.dll, dxil.dll and the shaders folder have to stay
# together: the shaders are compiled at startup by DXC, not baked into the binary.

[CmdletBinding()]
param(
    [switch] $Uninstall,
    [bool]   $Activate = $true,
    [string] $Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'

$target = Join-Path $env:LOCALAPPDATA 'Juggler'
$desktop = 'HKCU:\Control Panel\Desktop'

function Get-ActiveSaver {
    (Get-ItemProperty -LiteralPath $desktop -Name 'SCRNSAVE.EXE' -ErrorAction SilentlyContinue).'SCRNSAVE.EXE'
}

if ($Uninstall) {
    # Stand down as the active screensaver first, so nothing is pointing at
    # files that are about to disappear.
    $active = Get-ActiveSaver
    if ($active -and $active.StartsWith($target, [StringComparison]::OrdinalIgnoreCase)) {
        Remove-ItemProperty -LiteralPath $desktop -Name 'SCRNSAVE.EXE' -ErrorAction SilentlyContinue
        Set-ItemProperty -LiteralPath $desktop -Name 'ScreenSaveActive' -Value '0'
        Write-Host 'Deselected Juggler as the screensaver.'
    }

    Get-Process -Name 'Juggler.scr' -ErrorAction SilentlyContinue | ForEach-Object { $_.Kill() }

    if (Test-Path -LiteralPath $target) {
        Remove-Item -LiteralPath $target -Recurse -Force
        Write-Host "Removed $target"
    }

    Write-Host 'Done.'
    return
}

# In a source tree the payload is under build\<Configuration>; in an extracted
# release zip it sits next to this script.
$source = Join-Path $PSScriptRoot "build\$Configuration"
if (-not (Test-Path -LiteralPath (Join-Path $source 'Juggler.scr'))) {
    $source = $PSScriptRoot
}

$required = @('Juggler.scr', 'dxcompiler.dll', 'dxil.dll')
foreach ($file in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $source $file))) {
        throw "$file is missing from $source. Build with: msbuild Juggler.sln /p:Configuration=$Configuration /p:Platform=x64"
    }
}
if (-not (Test-Path -LiteralPath (Join-Path $source 'shaders'))) {
    throw "The shaders folder is missing from $source. Build with: msbuild Juggler.sln /p:Configuration=$Configuration /p:Platform=x64"
}

# Anything currently running is holding the file we are about to overwrite.
Get-Process -Name 'Juggler.scr' -ErrorAction SilentlyContinue | ForEach-Object { $_.Kill() }
Start-Sleep -Milliseconds 300

New-Item -ItemType Directory -Force -Path $target | Out-Null
foreach ($file in $required) {
    Copy-Item -LiteralPath (Join-Path $source $file) -Destination $target -Force
}
# Mirror the shaders rather than merging, so a renamed or deleted shader does
# not linger in the installed copy and get loaded instead of the current one.
$shaderTarget = Join-Path $target 'shaders'
if (Test-Path -LiteralPath $shaderTarget) { Remove-Item -LiteralPath $shaderTarget -Recurse -Force }
Copy-Item -LiteralPath (Join-Path $source 'shaders') -Destination $shaderTarget -Recurse -Force

Write-Host "Installed to $target"

if ($Activate) {
    Set-ItemProperty -LiteralPath $desktop -Name 'SCRNSAVE.EXE' -Value (Join-Path $target 'Juggler.scr')
    Set-ItemProperty -LiteralPath $desktop -Name 'ScreenSaveActive' -Value '1'
    if (-not (Get-ItemProperty -LiteralPath $desktop -Name 'ScreenSaveTimeOut' -ErrorAction SilentlyContinue)) {
        Set-ItemProperty -LiteralPath $desktop -Name 'ScreenSaveTimeOut' -Value '300'
    }
    Write-Host 'Selected Juggler as the screensaver.'
    Write-Host 'Open Screen Saver Settings to change the timeout or preview it.'
}

$signature = Get-AuthenticodeSignature -LiteralPath (Join-Path $target 'Juggler.scr')
if ($signature.Status -ne 'Valid') {
    Write-Warning @'
Juggler.scr is not code-signed, so SmartScreen may warn the first time it runs.
Signing needs a code-signing certificate, which this project does not have;
the binary is unsigned by circumstance rather than by choice.
'@
}
