# Windows port foundation

User selected a native Kagami camera and winget/.exe installation. New repository: celestial-sora/kagami-win. Windows 11 x64 is the initial target, because MFCreateVirtualCamera requires build 22000+. Windows 10 is not currently supported.

This first batch supplies an actual Media Foundation media source, an explicit camera enable/remove UI, a synthetic output test, restricted local latest-frame IPC and an Inno Setup .exe recipe. Camera output is fixed at 1280x720, 30 FPS (RGB32/NV12). Input transports and crop/rotate/mirror UI are not implemented in this foundation. Do not describe this batch as a working phone receiver.

Backend derives from Microsoft Windows-Camera's MIT-licensed SimpleMediaSource, pinned at 626f8b19c5f367602f2e89c6b314573d3776c9df. Physical-camera wrappers, association requests and default synthetic imagery were removed. Own CLSID: {542A79E7-54AC-48F9-B948-4620292BCC85}. Private activation attribute sets a per-user local pipe. Black is the default on startup and producer EOF; synthetic frames require the Test pattern button.

Next: verify installed camera with OBS/Discord on a consenting host; add Windows AirPlay owned UxPlay with WASAPI desktop audio and explicit protocol disconnect events; investigate native Miracast reception without silently changing Wi-Fi; connect authorized USB/ADB fallback to the same framing/output engine. Do not reuse Linux V4L2, fcntl, Avahi/systemctl, polkit or process-group code directly. AirPlay must keep a fixed letterboxed 1280x720 input canvas. No automatic capture/reconnect or persisted wireless identities/secrets.

Contributor: celestail-sora.
