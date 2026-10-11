[CmdletBinding()]
param([switch]$Installer, [switch]$InstallBuildDependencies, [switch]$AirPlay)
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $taskRoot
function Invoke-Checked {
    param([string]$Program, [string[]]$Arguments)
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}
if ($InstallBuildDependencies) {
    Invoke-Checked winget @('install','--id','Microsoft.VisualStudio.2022.BuildTools','--exact','--source','winget','--silent','--accept-package-agreements','--accept-source-agreements','--override','--quiet --wait --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended')
    if ($AirPlay) { Invoke-Checked winget @('install','--id','MSYS2.MSYS2','--exact','--source','winget','--silent','--accept-package-agreements','--accept-source-agreements') }
    if ($Installer) { Invoke-Checked winget @('install','--id','JRSoftware.InnoSetup','--exact','--source','winget','--silent','--accept-package-agreements','--accept-source-agreements') }
}
if ($AirPlay) {
    $msysBash = 'C:/msys64/usr/bin/bash.exe'
    if (!(Test-Path -LiteralPath $msysBash)) { throw 'Install MSYS2.MSYS2 with winget for the AirPlay build.' }
    $env:MSYSTEM = 'UCRT64'
    $env:CHERE_INVOKING = '1'
    Invoke-Checked $msysBash @('-lc', 'pacman -Syu --noconfirm')
    Invoke-Checked $msysBash @('-lc', 'pacman -S --needed --noconfirm git mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-python mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-libplist mingw-w64-ucrt-x86_64-openssl mingw-w64-ucrt-x86_64-gstreamer mingw-w64-ucrt-x86_64-gst-plugins-base mingw-w64-ucrt-x86_64-gst-plugins-good mingw-w64-ucrt-x86_64-gst-plugins-bad mingw-w64-ucrt-x86_64-gst-libav')
    Invoke-Checked $msysBash @('-lc', 'mkdir -p build && bash tools/build-airplay.sh')
}
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (!(Test-Path -LiteralPath $vswhere)) { throw 'Install C++ Build Tools with -InstallBuildDependencies.' }
$vsPath = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vsPath) { throw 'C++ toolchain not ready. Finish installing Build Tools, then retry.' }
Import-Module (Join-Path $vsPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
New-Item -ItemType Directory -Force '.local/deps','build','dist' | Out-Null
$packages = @(
    @{Name='cppwinrt'; Id='microsoft.windows.cppwinrt'; Version='3.0.260520.1'; Hash='D22E2E26133D63217AE26E91B1685FB024B03A508A78AF645F8347A3126C8435'},
    @{Name='wil'; Id='microsoft.windows.implementationlibrary'; Version='1.0.260126.7'; Hash='7D67BC71DD0EDC342DB44EA0334AC9B6A1776514F81694B0B5B3C7AC9D608E42'}
)
Write-Host '[1/5] Verify pinned native headers'
foreach ($package in $packages) {
    $archive = Join-Path $taskRoot ".local/deps/$($package.Name).zip"
    if (!(Test-Path -LiteralPath $archive)) {
        Invoke-WebRequest "https://api.nuget.org/v3-flatcontainer/$($package.Id)/$($package.Version)/$($package.Id).$($package.Version).nupkg" -OutFile $archive
    }
    if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $package.Hash) { throw "Checksum mismatch for $($package.Name)" }
    Expand-Archive -LiteralPath $archive -DestinationPath ".local/deps/$($package.Name)" -Force
}
Invoke-Checked (Join-Path $taskRoot '.local/deps/cppwinrt/bin/cppwinrt.exe') @('-input','sdk','-output',"$taskRoot/.local/deps/cppwinrt/include")
$common = @('/nologo','/utf-8','/O2','/std:c++17','/EHsc','/MT','/W4','/DUNICODE','/D_UNICODE','/D_WIN32_WINNT=0x0A00',"/I$taskRoot/native", "/I$taskRoot/.local/deps/cppwinrt/include", "/I$taskRoot/.local/deps/wil/include", "/Fo$taskRoot\build\")
Write-Host '[2/5] Build Kagami Media Foundation camera'
$sources = (Get-ChildItem -LiteralPath native/source -Filter '*.cpp').FullName
Invoke-Checked cl ($common + @('/LD') + $sources + @('/link','/DEF:native/source/KagamiCamera.def','/OUT:dist/KagamiCamera.dll','advapi32.lib','shlwapi.lib','propsys.lib'))
Write-Host '[3/5] Build native desktop executable'
Push-Location native
try { Invoke-Checked rc @('/nologo','/fo','../build/app.res','app.rc') } finally { Pop-Location }
Invoke-Checked cl ($common + @('native/CameraHost.cpp','build/app.res','/Fe:dist/Kagami.exe','/link','/SUBSYSTEM:WINDOWS','mfplat.lib','mfuuid.lib','mfsensorgroup.lib','ole32.lib','oleaut32.lib','uuid.lib','advapi32.lib','user32.lib','gdi32.lib','shell32.lib'))
Write-Host '[4/5] Test framing and actual local frame bridge (no camera registration)'
Invoke-Checked cl ($common + @('tests/framing.cpp','/Fe:build/framing-tests.exe'))
Invoke-Checked (Join-Path $taskRoot 'build/framing-tests.exe') @()
Invoke-Checked cl ($common + @('tests/ui_layout.cpp','build/app.res','/Fe:build/ui-layout-tests.exe','/link','user32.lib','gdi32.lib','advapi32.lib'))
Invoke-Checked (Join-Path $taskRoot 'build/ui-layout-tests.exe') @()
Invoke-Checked cl ($common + @('tests/bridge.cpp','/Fe:build/bridge-tests.exe','/link','advapi32.lib'))
Invoke-Checked (Join-Path $taskRoot 'build/bridge-tests.exe') @()
Invoke-Checked cl ($common + @('tests/media_source.cpp','/Fe:build/source-tests.exe','/link','mfplat.lib','mf.lib','mfuuid.lib','ole32.lib','advapi32.lib','propsys.lib'))
Invoke-Checked (Join-Path $taskRoot 'build/source-tests.exe') @()
Write-Host '[5/5] Package development installer'
Copy-Item LICENSE dist/LICENSE.txt
Copy-Item native/source/LICENSE.microsoft dist/LICENSE.microsoft.txt
if ($Installer) {
    $iscc = @((Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6/ISCC.exe'), (Join-Path $env:ProgramFiles 'Inno Setup 6/ISCC.exe'), (Join-Path $env:LOCALAPPDATA 'Programs/Inno Setup 6/ISCC.exe')) | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
    if (!$iscc) { throw 'Install JRSoftware.InnoSetup using winget, or use -InstallBuildDependencies -Installer.' }
    Invoke-Checked $iscc @('packaging/kagami.iss')
}
Get-ChildItem -LiteralPath dist -File | Get-FileHash -Algorithm SHA256 | ForEach-Object { "$($_.Hash.ToLower())  $([IO.Path]::GetFileName($_.Path))" } | Set-Content -Encoding ascii build/SHA256SUMS
Write-Host 'Build passed. Camera registration and phone compatibility remain manual acceptance checks.'
