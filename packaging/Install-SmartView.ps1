[CmdletBinding()]
param([switch]$TrustDevelopmentCertificate)
$ErrorActionPreference = 'Stop'
$certificate = Join-Path $PSScriptRoot 'Kagami-Development.cer'
if ($TrustDevelopmentCertificate) {
    # Elevated setup invokes this only after the unchecked task is selected.
    $leaf = [Security.Cryptography.X509Certificates.X509Certificate2]::new($certificate)
    if ($leaf.Subject -ne 'CN=Kagami Development Preview' -or $leaf.NotAfter -lt (Get-Date)) { throw 'Unexpected or expired development signing certificate.' }
    Import-Certificate -FilePath $certificate -CertStoreLocation Cert:\LocalMachine\TrustedPeople | Out-Null
    exit
}
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = [Security.Principal.WindowsPrincipal]::new($identity)
if ($principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) { throw 'Install the Smart View package from a normal, unprivileged desktop user session.' }
Add-AppxPackage -Path (Join-Path $PSScriptRoot 'Kagami-SmartView.msix')
Write-Host 'Smart View companion installed. Open Kagami; reception remains stopped.'
