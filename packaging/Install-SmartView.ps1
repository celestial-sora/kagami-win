[CmdletBinding()]
param([switch]$TrustDevelopmentCertificate, [switch]$Uninstall)
$ErrorActionPreference = 'Stop'
if ($TrustDevelopmentCertificate -and $Uninstall) { throw 'Certificate trust and per-user removal are separate operations.' }
$certificate = Join-Path $PSScriptRoot 'Kagami-Development.cer'
if ($TrustDevelopmentCertificate) {
    # Elevated setup trusts the leaf needed to install the bundled Smart View feature.
    $leaf = [Security.Cryptography.X509Certificates.X509Certificate2]::new($certificate)
    if ($leaf.Subject -ne 'CN=Kagami Development Preview' -or $leaf.NotAfter -lt (Get-Date)) { throw 'Unexpected or expired development signing certificate.' }
    Import-Certificate -FilePath $certificate -CertStoreLocation Cert:\LocalMachine\TrustedPeople | Out-Null
    exit
}
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = [Security.Principal.WindowsPrincipal]::new($identity)
if ($principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) { throw 'Manage the Smart View package from a normal, unprivileged desktop user session.' }
$packageName = 'CelestialSora.Kagami.Development'
$publisher = 'CN=Kagami Development Preview'
# No -AllUsers or administrator-account fallback: deployment belongs only to
# the normal desktop user who explicitly invoked this action.
$packages = @(Get-AppxPackage -Name $packageName | Where-Object { $_.Name -eq $packageName -and $_.Publisher -eq $publisher })
if ($Uninstall) {
    foreach ($package in $packages) {
        Remove-AppxPackage -Package $package.PackageFullName
    }
    # Framing/preferences live in unvirtualized HKCU\Software\Kagami\Desktop;
    # leave those values intact. Do not revoke a development certificate that
    # another user or installed development build may still use.
    if ($packages.Count) { Write-Host 'Smart View companion removed for this user. Kagami preferences were preserved.' }
    else { Write-Host 'No Kagami Smart View companion is installed for this user.' }
    exit
}
# Add-AppxPackage updates the same package identity in place. Never uninstall
# first, force a downgrade, or force a running receiver/application to close.
# A running app or non-newer artifact must be resolved explicitly by the user.
Add-AppxPackage -Path (Join-Path $PSScriptRoot 'Kagami-SmartView.msix')
Write-Host 'Smart View companion installed. Open Kagami; reception remains stopped.'

