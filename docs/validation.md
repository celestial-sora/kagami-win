# Windows validation

This is a development foundation, not a supported phone receiver or stable release.

Local Windows build 26300: Visual Studio 2022 Build Tools 17.14.41, Windows SDK 10.0.26100, pinned C++/WinRT generator/WIL and Inno Setup 6.7.3 compile the native DLL, desktop and development setup EXE. Actual named-pipe contracts pass. A separate executable loads the actual camera DLL directly, activates its source, supplies a Media Foundation allocator and reads both advertised NV12 and RGB32 samples: initial black, non-black pixels from the local producer, EOF returning black, Stop and Shutdown all pass. No camera is registered or host network changed by these tests.

Automated checks compile the actual native media source/desktop executable and .exe installer. Bridge checks send complete synthetic BGRX frames through a real local named pipe, check exclusive ownership, initial slate, invalid sizes, disconnect clearing, reconnect and Stop without a consumer. These checks do not register a camera, elevate media, alter networking or record a screen.

Physical acceptance remains open: installed MF camera enumeration/negotiation in OBS and Discord; Windows privacy/FrameServer service access; setup/uninstall and upgrades; concurrent consumers; actual iPhone/iPad/Mac AirPlay; Galaxy Smart View; USB and authorized ADB Wi-Fi; desktop audio and AV sync; real output FPS/long-session latency. Native transports and framing are subsequent work.
