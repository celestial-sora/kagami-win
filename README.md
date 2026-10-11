# Kagami for Windows

Native Windows port of [Kagami](https://github.com/celestial-sora/kagami), targeting **Windows 11 x64**. This development preview adds the reference desktop UI, phone receiver integrations, framing and selectable camera output. Real phone, installed camera and desktop audio acceptance remains pending; see [validation](docs/validation.md) and the [feature comparison](docs/feature-parity.md).

The main window contains transport selection, Start/Stop, connection status, source and virtual camera previews, framing controls, preview guides and output settings. Capture never starts on installation, launch or preference restore. Frames remain in memory; Kagami makes no frame/audio recordings and requires no cloud account, browser/URL/QR pairing or Android Kagami app.

## Install and try

Use the setup EXE from a successful [feature CI run](https://github.com/celestial-sora/kagami-win/actions/workflows/checks.yml). Artifacts are development builds, not stable releases. `winget install celestial-sora.Kagami` is the intended distribution command but **is not available in the public catalog yet**; see [WinGet distribution](docs/winget.md).

Setup elevates to install/register the fixed native DLL under Program Files. Desktop/media run as the normal user. Optional OBS, Android dependency and Smart View tasks are unchecked. Installation never enables the camera or starts mirroring. Local preferences and framing presets are retained.

1. Open Kagami normally and click **Enable camera**.
2. Select a transport and start it explicitly. Connect using the instructions below.
3. Drag the source preview to crop, or open **Framing controls** for aspect, rotation, mirror, fit/fill and named presets. Toggle framing/safe-area guides as needed.
4. Select **Kagami Windows Virtual Camera** in your camera application. Installed OBS/Discord operation remains an acceptance check.
5. **Stop** clears output to black. **Remove camera** removes the current user's Kagami camera.

| Transport | Preparation | Current limitation |
| :-- | :-- | :-- |
| AirPlay | Start AirPlay, then select Kagami in Apple Screen Mirroring on the same local network. | Owned UxPlay and WASAPI desktop audio are integrated; Apple devices and AV sync need testing. Keep the receiver window open. |
| Smart View | Select the optional Smart View setup task, reopen Kagami, select Smart View and Start. The PC needs Windows Miracast receiver support. | Reception uses a packaged companion window. Controls and processed preview remain in Kagami; keep the companion open. Galaxy reception and package activation need testing. |
| USB / ADB Wi-Fi | Install scrcpy 4.0+ and ADB, enable debugging, authorize the PC on the phone, then open USB / ADB setup and refresh devices. | Wireless debugging requires explicit Pair using its pairing port/code, then Connect using its separate debugging port. Select that authorized device. Kagami accepts private IP endpoints and saves no addresses/codes. Android audio is disabled. |

Select setup's optional Android dependency task to install `Genymobile.scrcpy` through winget, or run `winget install --id Genymobile.scrcpy --exact --source winget` separately. Verify scrcpy 4.0+ and restart Kagami after installation. ADB itself manages Android authorization independently of Kagami.

The optional Smart View task explicitly trusts the shipped **public development signing leaf certificate** in machine `TrustedPeople`, then installs the MSIX for the normal desktop user. It does not add a root certificate. Each build uses an ephemeral signing key, deletes that private key after signing, and ships only the public certificate. Stable release signing is separate. EXE uninstall currently does not remove the optional per-user package or certificate; their cleanup and upgrades remain acceptance items.

## Output format and audio

Output choices are 1920×1080, 1280×720, 960×540, 640×480, 1080×1920, 720×1280 and 1080×1080 at target rates 15/24/30/60 FPS. Apply changes with **Remove camera → Change format → Enable camera → reopen the consumer**. The source advertises RGB32 and NV12 at the selected dimensions/rate. Input is normalized to 1280×720, so larger outputs upscale that canvas. A 60 FPS output does not guarantee 60 distinct received phone frames per second.

AirPlay audio uses Windows desktop output; Smart View delegates playback to Windows MediaPlayer. These routes are intended for desktop-audio capture in apps such as OBS and create no separate Kagami audio device. Actual sound, routing and AV sync remain unverified.

## Build

Build dependencies use winget; C++/WinRT and WIL are pinned and SHA-256 checked:

```powershell
pwsh -File tools/build.ps1 -InstallBuildDependencies -AirPlay -Installer
```

With Visual Studio C++ Build Tools, Inno Setup and MSYS2 installed, use `pwsh -File tools/build.ps1 -AirPlay -Installer`. The AirPlay step updates MSYS2 UCRT64 packages and builds the pinned modified backend. End users need no Python, npm, Visual Studio or MSYS2 for the bundled AirPlay runtime.

Outputs include `dist/Kagami.exe`, `dist/KagamiCamera.dll`, `dist/KagamiSmartView.exe`, `dist/airplay/`, `dist/smartview/Kagami-SmartView.msix`, its public development certificate, `dist/Kagami-Setup-0.1.0-dev-x64.exe` and `build/SHA256SUMS`. AirPlay artifacts include licenses, package inventory and corresponding modified UxPlay source.

Kagami requires an unprivileged Explorer desktop. With UAC disabled, an administrator account's Explorer may be elevated; enable UAC and restart before using that configuration. Media refuses elevated launch. `tools/Test-DesktopLaunch.ps1` is an opt-in interactive token check and never enables a camera.

## Provenance and release policy

The source derives from Microsoft's MIT-licensed Windows-Camera sample at `626f8b19c5f367602f2e89c6b314573d3776c9df`; see [provenance](native/source/PROVENANCE.md). AirPlay uses modified UxPlay pinned to `ec203121355220dd90278a369eeabb0b8ba4f14a`. Approved upstream artwork preserves alpha and is padded to a square for Windows icons. Linux V4L2/MiracleCast installation remains in the Ubuntu repository.

Changes publish to a feature branch, pass CI, then integrate main once without a PR. Stable releases require separate guarded main CI approval. See [handoff](docs/handoff.md) for boundaries and open work.

Maintainer: **celestial-sora**. Contributor: **celestail-sora**.
