# Kagami for Windows

Native Windows port of [Kagami](https://github.com/celestial-sora/kagami), targeting **Windows 11 x64**. Development preview: this batch implements the native Kagami virtual camera foundation and `.exe` packaging. **Phone reception is not implemented yet.**

The camera appears as **Kagami Windows Virtual Camera**, independently of OBS' output camera. An owned local pipe carries complete latest BGRX frames in memory. Waiting/disconnected output is black. No phone, desktop or physical camera capture starts automatically. A synthetic pattern is available only through an explicit button.

## Build

Install build dependencies through winget, then build the native camera, app, actual IPC contracts and setup executable:

```powershell
winget install --id Microsoft.VisualStudio.2022.BuildTools --exact --source winget --override "--quiet --wait --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
winget install --id JRSoftware.InnoSetup --exact --source winget
pwsh -File tools/build.ps1 -Installer
```

Or `pwsh -File tools/build.ps1 -InstallBuildDependencies -Installer`. C++/WinRT and WIL header packages are version-pinned and SHA-256 checked. The native runtime is bundled statically; end users do not need Python, npm or Visual Studio. Builds produce `dist/Kagami.exe`, `dist/KagamiCamera.dll`, `dist/Kagami-Setup-0.1.0-dev-x64.exe` and `build/SHA256SUMS`.

Setup requests elevation to place/register the native source under protected Program Files. It launches the desktop with the original user's privileges; media refuses administrator execution. Optional OBS installation uses winget and is unchecked by default. Installation does not enable a camera or start mirroring. User settings directories are never removed by setup/uninstall.

After installation, open Kagami normally, click **Enable camera**, select **Kagami Windows Virtual Camera** in OBS/Discord, then optionally click **Test pattern**. **Stop** ends the producer; enabled camera output returns black. **Remove camera** removes only the current user's Kagami instance. Installed camera/consumer operation remains a manual acceptance check.

## Scope and provenance

AirPlay, native Samsung Smart View, USB/authorized ADB Wi-Fi, framing controls, dimensions/FPS selection and remembered desktop choices are follow-up port work. No compatibility is inferred from compilation or synthetic output.

The source lifecycle is derived from Microsoft's MIT-licensed Windows-Camera sample at commit `626f8b19c5f367602f2e89c6b314573d3776c9df`. See [provenance](native/source/PROVENANCE.md), [handoff](docs/handoff.md) and [validation](docs/validation.md). Camera registration API requires [Windows build 22000+](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nf-mfvirtualcamera-mfcreatevirtualcamera).

Maintainer: **celestial-sora**. Contributor: **celestail-sora**.
