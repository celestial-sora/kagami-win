# Windows validation

This is a development preview. Source implementation, compilation, synthetic frames and installer packing never establish Apple/Galaxy/OBS/Discord compatibility.

## Evidence and current CI status

The earlier native UI foundation at commit `94712136d57109930f776481bcdd4c90cdb2b378` passed [feature CI 38130880924](https://github.com/celestial-sora/kagami-win/actions/runs/38130880924), including native compilation, direct actual-DLL tests, local frame IPC and development EXE packing. Its rendered UI artifact was visually inspected. That run predates the complete receiver and dynamic-format changes and does not validate them.

The current expanded development branch must pass its own complete CI before integration. At this documentation update, that CI is still being evaluated; do not infer a passing result from the earlier run. Feature artifacts are development artifacts, and no stable release or public WinGet catalog availability is claimed.

[Expanded CI 38133750576](https://github.com/celestial-sora/kagami-win/actions/runs/38133750576) passed the portable AirPlay plugin probes but then failed native compilation in the output-format dialog. That compile issue has a local fix; the full expanded build, tests and packaging still require a successful subsequent run.

## Automated checks

| Check | What it exercises | What it cannot establish |
| :-- | :-- | :-- |
| Frame bridge | Real local named-pipe delivery, exclusive producer, initial slate, dimension/pixel atomicity, landscape/portrait resizing, invalid sizes/rates, EOF/reconnect and bounded Stop with absent/stalled consumers. | Installed FrameServer identity access or concurrent camera consumers. |
| Actual camera DLL | Direct activation of the built DLL with an MF allocator; advertised NV12/RGB32 dimensions and sample durations at default 1280×720@30, 1920×1080@60 and 720×1280@24; addressed rows/planes, local pixels, chroma conversion, EOF black, Stop/Shutdown. | Registered camera enumeration, consumer negotiation or effective hardware throughput. |
| Framing | Crop, fit/fill, rotation, mirror and invalid normalized coordinates using synthetic pixels. | Phone orientation, real crop interaction or subjective image quality. |
| UI layout | Native off-screen layout/control bounds and a rendered screenshot artifact. | Complete keyboard, DPI, multi-monitor or interactive usability acceptance. |
| Smart View protocol / ADB validation | Bounded packet/endpoint validation and permitted command inputs. | Package activation, receiver driver support, wireless pairing or live media. |
| AirPlay runtime preflight | Pinned patched backend build, required portable GStreamer plugin loading, source/license/runtime packaging. | Discovery through real firewall/network conditions, Apple reception or sound. |
| EXE/MSIX packaging | Native compilation, manifest packing/development signing and installer production. | Successful installation, trust/registration, upgrade/uninstall or end-user launch. |

These checks do not register a camera, alter host networking, enable reception or record frames/audio. Direct DLL tests use synthetic in-memory pixels and a local pipe. The MF sample test is not an OBS/Discord test.

## Required physical and interactive acceptance

- Install the EXE on a normal Windows 11 x64 desktop, verify fixed Program Files camera registration, unelevated launch, stopped initial state, preferences retained across upgrade and uninstall behavior.
- Explicitly select the optional Smart View task; verify leaf certificate TrustedPeople scope, per-user MSIX activation, packaged main UI, unsupported-hardware messages and no automatic receiver start. Confirm optional package/certificate cleanup separately.
- Enable Kagami and enumerate/negotiate it in OBS and Discord. Check FrameServer service access, RGB32/NV12 formats, landscape/portrait/square changes after remove/re-enable/reopen and concurrent consumers.
- Exercise real iPhone/iPad/Mac AirPlay, Galaxy Smart View and authorized Android USB/wireless debugging. Check pairing cancellation, renderer/companion close/minimize, main-app Stop/close/crash, producer EOF, reconnect requiring explicit Start, and black cleared camera output.
- Confirm AirPlay WASAPI and Smart View MediaPlayer desktop sound, OBS Desktop Audio routing, AV sync, mute/failure handling and no surviving child audio after Stop.
- Measure actual received/output FPS, latency, resizing/orientation, long-session memory use and transport timeouts. Output target rate alone is not proof of effective throughput.
- Compare the reference screenshot at 100%/125%/150%/200% DPI; inspect keyboard focus, minimum window size, dark popups, multi-monitor transitions and screen-reader labels.
- Inspect setup from an administrator terminal and a normal Explorer desktop. Stable distribution, trusted stable signing, public download verification and WinGet catalog acceptance remain separate gates.

## Previous host observations

Earlier foundation checks on Windows build 26300 compiled with Visual Studio 2022 Build Tools 17.14.41, Windows SDK 10.0.26100, pinned C++/WinRT/WIL and Inno Setup 6.7.3. Foundation direct DLL/IPC checks passed; they did not test current phone receivers.

Interactive launch investigation found UAC disabled (`EnableLUA=0`) and an elevated Explorer. Setup's `runasoriginaluser` cannot create an unprivileged media process from that desktop. The launch helper checks Explorer elevation and refuses that case. The maintainer authorized enabling UAC; a restarted normal desktop is needed for acceptance. `tools/Test-DesktopLaunch.ps1` is an explicit interactive token check, not headless CI.

Previous three-file WinGet candidate validation succeeded locally with WinGet 1.11.510. Artifact URLs, mutable latest-download URLs and HTTP URLs are rejected by manifest generation. Public release download verification, submission and installer acceptance remain pending.
