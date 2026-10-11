[CmdletBinding()]
param([string]$Version = '0.1.0.0')
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $taskRoot
$stage = Join-Path $taskRoot 'build/smartview-package'
# Reusing a stage can retain removed DLLs or an obsolete manifest. Delete only
# this resolved workspace-owned staging directory before creating the package.
$stage = [IO.Path]::GetFullPath($stage)
$buildRoot = [IO.Path]::GetFullPath((Join-Path $taskRoot 'build')) + [IO.Path]::DirectorySeparatorChar
if (!$stage.StartsWith($buildRoot,[StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe Smart View staging directory.' }
if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
New-Item -ItemType Directory -Force "$stage/Assets", 'dist/smartview' | Out-Null
Copy-Item dist/Kagami.exe,dist/KagamiSmartView.exe,dist/LICENSE*.txt $stage -Force
Copy-Item dist/airplay $stage -Recurse -Force
$manifest = [xml](Get-Content packaging/AppxManifest.xml -Raw)
$manifest.Package.Identity.Version = $Version
$manifest.Save("$stage/AppxManifest.xml")
Add-Type -AssemblyName System.Drawing
$art = [Drawing.Image]::FromFile("$taskRoot/packaging/kagami-icon.png")
try {
    foreach ($asset in @(@{Name='StoreLogo';Size=50},@{Name='Square150x150Logo';Size=150},@{Name='Square44x44Logo';Size=44})) {
        $bitmap = [Drawing.Bitmap]::new($asset.Size,$asset.Size,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $graphics = [Drawing.Graphics]::FromImage($bitmap)
        try {
            $graphics.Clear([Drawing.Color]::Transparent)
            $graphics.CompositingMode = [Drawing.Drawing2D.CompositingMode]::SourceCopy
            $graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $factor = $asset.Size / [Math]::Max($art.Width,$art.Height)
            $width = [int]($art.Width*$factor); $height = [int]($art.Height*$factor)
            $graphics.DrawImage($art,[int](($asset.Size-$width)/2),[int](($asset.Size-$height)/2),$width,$height)
            $bitmap.Save("$stage/Assets/$($asset.Name).png",[Drawing.Imaging.ImageFormat]::Png)
        } finally { $graphics.Dispose(); $bitmap.Dispose() }
    }
} finally { $art.Dispose() }
$sdkBin = Join-Path $env:WindowsSdkDir "bin/$($env:WindowsSDKVersion.TrimEnd('\'))/x64"
& "$sdkBin/makeappx.exe" pack /d $stage /p "$taskRoot/dist/smartview/Kagami-SmartView.msix" /o
if ($LASTEXITCODE) { throw 'MSIX manifest validation or packing failed.' }
# Only a public development leaf certificate ships. Never export a private key.
$cert = New-SelfSignedCertificate -Type Custom -Subject 'CN=Kagami Development Preview' -KeyUsage DigitalSignature -FriendlyName 'Kagami development artifact signing' -CertStoreLocation Cert:\CurrentUser\My -KeyAlgorithm RSA -KeyLength 3072 -HashAlgorithm SHA256 -NotAfter (Get-Date).AddMonths(3) -TextExtension @('2.5.29.37={text}1.3.6.1.5.5.7.3.3','2.5.29.19={text}')
try {
    Export-Certificate -Cert $cert -FilePath dist/smartview/Kagami-Development.cer | Out-Null
    & "$sdkBin/signtool.exe" sign /fd SHA256 /s My /sha1 $cert.Thumbprint dist/smartview/Kagami-SmartView.msix
    if ($LASTEXITCODE) { throw 'Development MSIX signing failed.' }
} finally { Remove-Item -LiteralPath "Cert:\CurrentUser\My\$($cert.Thumbprint)" -DeleteKey }
Copy-Item packaging/Install-SmartView.ps1 dist/smartview -Force
