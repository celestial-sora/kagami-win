# Windows port handoff

The reference desktop layout and feature families have native implementations in the current development branch. The Ubuntu reference remains [celestial-sora/kagami](https://github.com/celestial-sora/kagami). This port targets Windows 11 x64 (build 22000+) using a Media Foundation software virtual camera. Implementation does not prove physical receiver or installed-consumer compatibility; read [validation](validation.md) and [feature parity](feature-parity.md).

## Desktop and camera

Win32/GDI+ provides transport selection, explicit Start/Stop, status, source/output previews, source-preview crop dragging, numeric/aspect crop, rotation, mirror, fit/fill, guides, custom numeric output dimensions and named framing presets. Kagami enables its camera when the normal desktop UI launches but does not start a receiver or restore capture state. The Camera On/Off toggle confirms before disabling and stops active media. HKCU `Software\Kagami\Desktop`, `Software\Kagami\Presets` and `Software\Kagami\AndroidPresets` retain choices. Phone addresses and pairing codes are never restored. Saving while receiving a stable authorized USB serial binds the chosen label to portrait/landscape orientation; matching restores saved framing after orientation changes. App labels are manual, wireless identities are excluded, and physical orientation behavior remains unverified.

Camera identity remains `{542A79E7-54AC-48F9-B948-4620292BCC85}`. Never associate physical cameras or use physical-camera wrappers. The fixed DLL installs under protected Program Files with setup elevation only. Desktop/media remain unprivileged. Private activation attributes carry the producer pipe and width/height/FPS. The source advertises NV12/RGB32, default 1280×720@30. Format changes require Remove, change, Enable and consumer reopen.

Input produces a letterboxed 1280×720 canvas; shared framing scales to selected output. Larger output is upscaled. IPC validates even dimensions, a 1920×1080 pixel budget and rates 15/24/30/60. Initial output is black; producer Stop/close/EOF clears it. State retains one latest frame with bounded transport buffers. Output-pipe ACL permits the producer user and read access for Windows FrameServer service identities; remote clients are rejected. No frame/audio recordings.

## Transports

AirPlay launches pinned modified UxPlay in an owned kill-on-close job. Internal mDNS avoids mandatory Bonjour. GStreamer renders into a native window and uses WASAPI desktop audio. Windows Graphics Capture targets only that owned renderer and retains the system capture border. Personal UxPlay config is ignored; logs are not persisted. Explicit video/disconnect/audio-failure events govern lifecycle. Close/minimize, EOF or failure clears output and requires explicit restart. Corresponding modified source, licenses and package inventory ship with artifacts.

Smart View uses Windows MiracastReceiver in an AppContainer CoreApplicationView companion. Its MSIX also includes the packaged full-trust desktop, with registry-write virtualization disabled to preserve preferences. The EXE remains the main installer; the camera DLL remains in Program Files. Installation never starts reception. Main-UI Start activates the helper, applies the Kagami receiver name when Windows permits it, and checks actual Miracast listening status before reporting that it is discoverable. The helper's MediaPlayer frame-server path supplies the fixed canvas to shared framing/output. Keep the visible companion window open; hidden/background reception has not been validated. Windows Wi-Fi Direct/Miracast support and OS policy can still prevent discovery.

Smart View IPC uses a random local `LOCAL` pipe, the desktop-user SID and specific package SID, authenticated peer PID, bounded packets/one latest frame and stop/heartbeat lifecycle. Parent EOF/Stop must end reception and clear output. Hardware/driver limitations are reported. Kagami never silently changes Wi-Fi, installs Wireless Display or enables host discovery.

Android launches explicitly authorized scrcpy 4.0+ in an owned kill-on-close job with audio/control/recording disabled. Capture targets only that child's window. Setup provides bounded cancellable ADB devices/pair/connect operations with private IP endpoints. Kagami saves no endpoint/code; authorization belongs to ADB. EXE setup offers unchecked OBS and Android dependency tasks; Smart View installs with Kagami. The Android task runs `Install-OptionalDependencies.ps1 -Dependency Android` to install `Genymobile.scrcpy` through winget as the normal desktop user; verify scrcpy 4.0+ after installation.

Check setup reports local AirPlay executable/scanner presence, ADB/scrcpy PATH availability, the exact Kagami camera CLSID registration and package context. It supplies manual Wi-Fi/firewall/hardware/privacy guidance; Start reports runtime capability errors. Checks neither start reception nor automatically configure host networking and do not prove compatibility.

## Packaging and remaining work

`tools/build.ps1 -AirPlay -Installer` builds the desktop/DLL, transport tests, AirPlay runtime, development-signed Smart View MSIX and Inno EXE. `-InstallBuildDependencies` requests winget dependency installation. Setup always trusts only the shipped development leaf in machine TrustedPeople, then registers Smart View for the unprivileged desktop user; reception remains stopped. The build deletes the private signing key. Stable signing requires separate guarded release approval. The normal-user **Remove Smart View receiver (current user)** shortcut removes only the exact current-user development package; EXE uninstall does not remove per-user registrations automatically. Certificates remain. Read [receiver lifecycle](../packaging/SmartView-lifecycle.md); installed cleanup and upgrades need acceptance.

Publish through the GitHub connector to a `codex/` feature branch, wait for successful CI, then integrate main once. No PR; development artifacts are not stable releases. Required failing/incomplete CI prevents integration.

Open acceptance: installed camera/FrameServer access, OBS/Discord negotiation, Apple/Galaxy/Android devices, Smart View activation/window lifecycle, audio/AV sync, effective rates/latency, upgrades and setup/uninstall. Feature differences remain in the matrix. No browser/URL/QR pairing, cloud/account or mandatory Android Kagami app.

Contributor: **celestail-sora**. Repository owner: **celestial-sora**.

