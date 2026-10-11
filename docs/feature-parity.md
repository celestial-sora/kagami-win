# Feature comparison with the Ubuntu reference

This matrix compares the local upstream README and `apps/receiver/kagami_receiver` with the current Windows development implementation. “Implemented” describes code paths; real phones, installed camera consumers and audio still need the acceptance checks in [validation](validation.md). It does not mean every upstream behavior is identical.

| Upstream feature | Windows implementation | Difference / acceptance |
| :-- | :-- | :-- |
| Screenshot desktop layout and connection tiles | Native dark Win32/GDI+ panels, selected transport, status, source/output previews and help steps. | Platform controls/window decorations differ; DPI/interaction acceptance pending. |
| Explicit start/stop and waiting state | Main UI Start/Stop, black initial/disconnected camera output. | No automatic capture or reconnect; real transport lifecycle pending. |
| AirPlay / UxPlay | Pinned modified Windows UxPlay, internal mDNS, owned renderer capture, bounded media and protocol disconnect events. | Apple devices/discovery/firewall acceptance pending; keep renderer open. |
| AirPlay desktop audio | WASAPI output intended for OBS Desktop Audio. | No Kagami audio device; sound/AV sync pending. |
| Experimental Smart View / MiracleCast | Native Windows MiracastReceiver with packaged CoreApplicationView companion and main-UI preview/control. | Uses Windows hardware/driver capabilities, not Linux MiracleCast. Companion currently visible; Galaxy/package acceptance pending. |
| USB Android / scrcpy | Owned scrcpy 4.0+ child, explicitly selected authorized device; optional unchecked winget dependency task in EXE setup. | Audio/control/recording disabled; verify dependency version; real Android pending. |
| Authorized ADB wireless debugging | Refresh devices, private-IP Pair/Connect UI, cancellable bounded commands. | Pairing and debugging ports differ; ADB retains authorization, Kagami saves no endpoint/code. Live pairing pending. |
| Numeric and drag crop | Normalized fields and dragging within the fitted source preview. | Physical crop interaction pending. |
| Aspect crop | Free, 16:9, 4:3, 1:1 and portrait 9:16. | Uses the normalized fixed input canvas. |
| Rotation / mirror / fit-fill | Shared frame processing for main preview and camera output. | Synthetic pixel checks; phone orientation acceptance pending. |
| Framing guide / OBS safe area | Preview overlays, stored toggle preferences. | Overlays are guides and are not burned into output. |
| Source and virtual-camera output previews | Main UI switches between source and processed output. | One view at a time; physical framing acceptance pending. |
| Device/app/orientation presets | Named framing presets with Save/Load/Delete. | **Partial parity:** manual labels/selection; upstream automatic serial/app/frame-dimension matching is absent. |
| Remember last explicit choices | Local crop, rotation, mirror, fit/fill, guides, selected transport and output format. | No persisted capture state/wireless identity; Windows HKCU storage rather than upstream JSON. |
| Dimensions and FPS | Seven selectable formats and 15/24/30/60 target rates, selected NV12/RGB32 camera advertisement. | **Partial parity:** upstream permits broader numeric settings; Windows uses bounded presets. Apply through Remove/change/Enable/reopen. All transports normalize to 1280×720, so larger output upscales. |
| Linux input/output V4L2 nodes | Owned receiver frame input and private Media Foundation software camera. | Windows has no `/dev/video` input/output selectors and never writes physical cameras. |
| Check/setup diagnostics | Native status/errors, ADB device refresh, Miracast capability/error reporting and build preflight. | **Partial parity:** no complete upstream Linux “Check” dependency/network workflow or automatic host configuration. |
| Local operation, no account/mobile Kagami app | Local transport/IPC; no cloud, account or browser/URL/QR pairing. | Android fallback requires explicit ADB authorization, as upstream does. |
| Installer / dependencies / rollback | Inno EXE, winget build dependencies and optional unchecked OBS/Android tasks, optional development Smart View MSIX. | **Partial parity:** public WinGet availability, stable signing, optional-package cleanup and automatic previous-version rollback are not implemented/validated. |
| Approved icon artwork and credit | Preserved RGBA artwork, padded Windows icons and package assets. | Owner celestial-sora; contributor celestail-sora. |

The automatic preset matching, broader numeric output settings, full diagnostics/rollback and remaining installation acceptance above must stay visible as differences. Windows-specific transport code is a port of the feature intent, not reuse of Linux V4L2, Avahi/systemctl, MiracleCast, polkit or process-group operations. The development preview must not be described as complete upstream parity or validated Apple/Galaxy/OBS/Discord support.
