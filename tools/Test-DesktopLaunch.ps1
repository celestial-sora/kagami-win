[CmdletBinding()]
param(
    [string]$AppPath = (Join-Path $PSScriptRoot '../dist/Kagami.exe'),
    [switch]$KeepOpen
)
$ErrorActionPreference = 'Stop'
# Opt-in interactive regression test. Needs a real Windows Explorer desktop.
# Opens a stopped app only; does not install, register or enable a camera.
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class KagamiLaunchToken {
    [DllImport("advapi32.dll", SetLastError=true)]
    static extern bool OpenProcessToken(IntPtr process, uint access, out IntPtr token);
    [DllImport("advapi32.dll", SetLastError=true)]
    static extern bool GetTokenInformation(IntPtr token, int type, out int value, int length, out int returned);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr handle);
    public static bool IsElevated(IntPtr process) {
        IntPtr token;
        if (!OpenProcessToken(process, 8, out token)) throw new System.ComponentModel.Win32Exception();
        try {
            int value, returned;
            if (!GetTokenInformation(token, 20, out value, 4, out returned)) throw new System.ComponentModel.Win32Exception();
            return value != 0;
        } finally { CloseHandle(token); }
    }
}
'@
$app = (Resolve-Path -LiteralPath $AppPath).Path
$existing = @(Get-Process Kagami -ErrorAction SilentlyContinue | ForEach-Object Id)
$helper = Start-Process -FilePath $app -ArgumentList '--launch-desktop' -PassThru
$child = $null
try {
    for ($attempt = 0; $attempt -lt 40; $attempt++) {
        $child = Get-Process Kagami -ErrorAction SilentlyContinue | Where-Object {
            $_.Id -ne $helper.Id -and $_.Id -notin $existing -and $_.Path -eq $app
        } | Select-Object -First 1
        if ($child) { break }
        Start-Sleep -Milliseconds 250
    }
    if (!$child) { throw 'Desktop launcher did not create the app.' }
    if (!$helper.WaitForExit(5000) -or $helper.ExitCode -ne 0) { throw 'Desktop launcher failed.' }
    if ([KagamiLaunchToken]::IsElevated($child.Handle)) { throw 'Launched app is elevated.' }
    Write-Output "PASS: desktop app PID $($child.Id) is not elevated; launcher exited successfully."
} finally {
    if ($child -and !$KeepOpen) { $null = $child.CloseMainWindow() }
    if (!$helper.HasExited) { $null = $helper.CloseMainWindow() }
}
