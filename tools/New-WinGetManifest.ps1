[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$InstallerPath,
    [Parameter(Mandatory)][uri]$InstallerUrl,
    [string]$OutputRoot = (Join-Path $PSScriptRoot '../build/winget'),
    [switch]$VerifyPublishedAsset
)
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
$setup = Get-Content -LiteralPath (Join-Path $taskRoot 'packaging/kagami.iss') -Raw
function Read-SetupValue([string]$Name) {
    $match = [regex]::Match($setup, "(?m)^$Name=([^\r\n]+)")
    if (!$match.Success) { throw "Missing $Name in kagami.iss" }
    return $match.Groups[1].Value
}
function Quote-Yaml([string]$Value) { return "'" + $Value.Replace("'", "''") + "'" }
$version = Read-SetupValue 'AppVersion'
$name = Read-SetupValue 'AppName'
$publisher = Read-SetupValue 'AppPublisher'
$filename = (Read-SetupValue 'OutputBaseFilename') + '.exe'
if ($version -notmatch '^\d+(\.\d+){1,3}$') { throw 'Unsupported installer version.' }
if (!$InstallerUrl.IsAbsoluteUri -or $InstallerUrl.Scheme -ne 'https' -or
    $InstallerUrl.Host -ne 'github.com' -or $InstallerUrl.Query -or $InstallerUrl.Fragment -or
    $InstallerUrl.AbsolutePath -notmatch '^/celestial-sora/kagami-win/releases/download/[^/]+/[^/]+$' -or
    [uri]::UnescapeDataString(($InstallerUrl.AbsolutePath -split '/')[-1]) -ne $filename) {
    throw "Use an immutable Kagami GitHub release URL ending in $filename. Actions artifacts and latest URLs are not supported."
}
$installer = Get-Item -LiteralPath $InstallerPath
if ($installer.Name -ne $filename) { throw "Expected installer filename: $filename" }
$hash = (Get-FileHash -LiteralPath $installer.FullName -Algorithm SHA256).Hash
if ($VerifyPublishedAsset) {
    $download = Join-Path ([IO.Path]::GetTempPath()) ([guid]::NewGuid().ToString() + '.exe')
    try {
        Invoke-WebRequest -Uri $InstallerUrl.AbsoluteUri -OutFile $download
        if ((Get-FileHash -LiteralPath $download -Algorithm SHA256).Hash -ne $hash) {
            throw 'Published installer differs from the local installer. Do not submit this manifest.'
        }
    } finally {
        if (Test-Path -LiteralPath $download) { Remove-Item -LiteralPath $download }
    }
}
$id = 'celestial-sora.Kagami'
$schema = '1.6.0'
$directory = Join-Path $OutputRoot "manifests/c/celestial-sora/Kagami/$version"
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$common = "PackageIdentifier: $id`nPackageVersion: $(Quote-Yaml $version)"
$versionYaml = @"
# yaml-language-server: `$schema=https://aka.ms/winget-manifest.version.$schema.schema.json
$common
DefaultLocale: en-US
ManifestType: version
ManifestVersion: $schema
"@
$localeYaml = @"
# yaml-language-server: `$schema=https://aka.ms/winget-manifest.defaultLocale.$schema.schema.json
$common
PackageLocale: en-US
Publisher: $(Quote-Yaml $publisher)
PublisherUrl: https://github.com/celestial-sora
PublisherSupportUrl: https://github.com/celestial-sora/kagami-win/issues
PackageName: $(Quote-Yaml $name)
PackageUrl: https://github.com/celestial-sora/kagami-win
License: MIT
LicenseUrl: https://github.com/celestial-sora/kagami-win/blob/main/LICENSE
ShortDescription: Native Kagami virtual camera for Windows 11.
Description: Development preview of the native Kagami camera. Phone reception is not implemented yet.
Moniker: kagami
Tags:
- virtual-camera
- obs
ManifestType: defaultLocale
ManifestVersion: $schema
"@
$installerYaml = @"
# yaml-language-server: `$schema=https://aka.ms/winget-manifest.installer.$schema.schema.json
$common
InstallerType: inno
Scope: machine
MinimumOSVersion: 10.0.22000.0
InstallModes:
- interactive
- silent
- silentWithProgress
UpgradeBehavior: install
AppsAndFeaturesEntries:
- DisplayName: $(Quote-Yaml $name)
  Publisher: $(Quote-Yaml $publisher)
  DisplayVersion: $(Quote-Yaml $version)
  ProductCode: '{5CBA3D97-48A0-4B98-9E87-F058C488C072}_is1'
Installers:
- Architecture: x64
  InstallerUrl: $(Quote-Yaml $InstallerUrl.AbsoluteUri)
  InstallerSha256: $hash
ManifestType: installer
ManifestVersion: $schema
"@
$utf8 = New-Object System.Text.UTF8Encoding($false)
foreach ($item in @(
    @{Suffix='.yaml'; Text=$versionYaml},
    @{Suffix='.locale.en-US.yaml'; Text=$localeYaml},
    @{Suffix='.installer.yaml'; Text=$installerYaml}
)) {
    [IO.File]::WriteAllText((Join-Path $directory ($id + $item.Suffix)), $item.Text + "`n", $utf8)
}
Write-Host "WinGet manifest: $directory"
if (!$VerifyPublishedAsset) { Write-Warning 'Candidate only: public release availability has not been verified. Do not submit yet.' }
Write-Output $directory
