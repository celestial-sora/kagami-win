[CmdletBinding()]
param([ValidateSet('OBS','Android')][string]$Dependency = 'OBS')
$ErrorActionPreference = 'Stop'
try {
    $packageManager = Get-Command winget.exe -ErrorAction Stop
    $packageId = if ($Dependency -eq 'Android') { 'Genymobile.scrcpy' } else { 'OBSProject.OBSStudio' }
    & $packageManager.Source install --id $packageId --exact --source winget --accept-package-agreements --accept-source-agreements
    if ($LASTEXITCODE -ne 0) { throw "winget returned $LASTEXITCODE. The dependency may already be installed; check its version separately (scrcpy needs 4.0+)." }
} catch {
    Add-Type -AssemblyName System.Windows.Forms
    [System.Windows.Forms.MessageBox]::Show("Optional $Dependency installation did not complete: $($_.Exception.Message)`nKagami is installed. Check Microsoft App Installer and install the selected dependency separately if needed.", 'Kagami dependencies') | Out-Null
    exit 1
}
