$ErrorActionPreference = 'Stop'
try {
    $packageManager = Get-Command winget.exe -ErrorAction Stop
    & $packageManager.Source install --id OBSProject.OBSStudio --exact --source winget --accept-package-agreements --accept-source-agreements
    if ($LASTEXITCODE -ne 0) { throw "winget returned $LASTEXITCODE. OBS may already be installed; check its installation separately." }
} catch {
    Add-Type -AssemblyName System.Windows.Forms
    [System.Windows.Forms.MessageBox]::Show("Optional OBS installation did not complete: $($_.Exception.Message)`nKagami is installed. Install Microsoft App Installer/OBS separately if needed.", 'Kagami dependencies') | Out-Null
    exit 1
}
