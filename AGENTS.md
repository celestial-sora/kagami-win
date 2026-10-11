# Kagami Windows maintainer direction

- Native Windows addition; Ubuntu reference remains at celestial-sora/kagami. No browser/URL/QR pairing, cloud/account or mandatory Android Kagami app.
- Windows 11 x64 minimum for Media Foundation software virtual camera. Camera is Kagami, using a private CLSID; never wrap or write to physical cameras.
- User requests winget dependency management and an .exe installer. Install fixed native DLL under protected Program Files with setup elevation only. Desktop/media must run unprivileged. Preserve local preferences and never automatically Start capture.
- No frame/audio recordings. IPC is local only, restricted to the producer user and Windows FrameServer service identities, bounded to one latest frame. Stop/close and producer EOF must clear the image.
- Preserve approved RGBA artwork from the Ubuntu project. Icon conversion pads to a square and preserves alpha.
- Never claim Apple/Galaxy/OBS/Discord compatibility from compilation, installation or synthetic tests. Read docs/handoff.md and docs/validation.md before changes.
- Publish via GitHub connector on a feature branch, wait for successful CI, then integrate main once. No PR. Stable releases require separate guarded main CI approval; development artifacts are not stable releases.
- Contributor credit: celestail-sora. Repo owner: celestial-sora.
