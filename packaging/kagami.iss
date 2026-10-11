[Setup]
AppId={{5CBA3D97-48A0-4B98-9E87-F058C488C072}
AppName=Kagami Windows Development Preview
AppVersion=0.1.0
AppPublisher=celestial-sora
AppPublisherURL=https://github.com/celestial-sora/kagami-win
DefaultDirName={autopf}\Kagami
DefaultGroupName=Kagami
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.22000
PrivilegesRequired=admin
OutputDir=..\dist
OutputBaseFilename=Kagami-Setup-0.1.0-dev-x64
SetupIconFile=kagami.ico
UninstallDisplayIcon={app}\Kagami.exe
Compression=lzma2
SolidCompression=yes
CloseApplications=yes
RestartApplications=no
LicenseFile=..\LICENSE

[Files]
Source: "..\dist\Kagami.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\KagamiCamera.dll"; DestDir: "{app}"; Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\dist\LICENSE*.txt"; DestDir: "{app}"
Source: "..\dist\airplay\*"; DestDir: "{app}\airplay"; Flags: recursesubdirs createallsubdirs
Source: "Install-OptionalDependencies.ps1"; DestDir: "{app}"
Source: "..\dist\smartview\*"; DestDir: "{app}\smartview"
Source: "SmartView-lifecycle.md"; DestDir: "{app}\smartview"

[Registry]
Root: HKLM; Subkey: "Software\Classes\CLSID\{{542A79E7-54AC-48F9-B948-4620292BCC85}"; ValueType: string; ValueData: "Kagami Media Source"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Classes\CLSID\{{542A79E7-54AC-48F9-B948-4620292BCC85}\InprocServer32"; ValueType: string; ValueData: "{app}\KagamiCamera.dll"
Root: HKLM; Subkey: "Software\Classes\CLSID\{{542A79E7-54AC-48F9-B948-4620292BCC85}\InprocServer32"; ValueType: string; ValueName: "ThreadingModel"; ValueData: "Both"

[Tasks]
Name: "obs"; Description: "Install OBS Studio via winget for camera testing (optional)"; Flags: unchecked
Name: "android"; Description: "Install scrcpy / ADB via winget for Android reception (requires scrcpy 4.0+)"; Flags: unchecked
Name: "smartview"; Description: "Install Smart View companion and trust this build's development signing certificate (optional)"; Flags: unchecked

[Icons]
Name: "{group}\Kagami"; Filename: "{app}\Kagami.exe"
Name: "{group}\Remove Smart View companion (current user)"; Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\smartview\Install-SmartView.ps1"" -Uninstall"; Comment: "Remove the optional companion for this desktop user; preserve Kagami preferences"

[Run]
Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\Install-OptionalDependencies.ps1"" -Dependency Android"; Tasks: android; Flags: runasoriginaluser waituntilterminated; StatusMsg: "Installing optional Android dependencies through winget..."
Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\smartview\Install-SmartView.ps1"" -TrustDevelopmentCertificate"; Tasks: smartview; Flags: runhidden waituntilterminated; StatusMsg: "Trusting the selected development signing certificate..."
Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\smartview\Install-SmartView.ps1"""; Tasks: smartview; Flags: runasoriginaluser waituntilterminated; StatusMsg: "Installing Smart View for the desktop user..."
Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\Install-OptionalDependencies.ps1"""; Tasks: obs; Flags: runasoriginaluser waituntilterminated; StatusMsg: "Installing optional OBS Studio through winget..."
Filename: "{app}\Kagami.exe"; Parameters: "--launch-desktop"; Description: "Open Kagami (capture stays stopped)"; Flags: nowait postinstall skipifsilent runasoriginaluser

[UninstallRun]
Filename: "{app}\Kagami.exe"; Parameters: "--remove-camera"; Flags: runhidden waituntilterminated; RunOnceId: "RemoveCurrentUserKagamiCamera"
