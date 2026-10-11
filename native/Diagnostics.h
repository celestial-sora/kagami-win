#pragma once
#include <windows.h>
#include <string>
#include "PackageActivation.h"
namespace kagami {
inline void showDiagnostics(HWND parent) {
    std::wstring report=L"Kagami local checks\n\n";
    wchar_t executable[32768]{};GetModuleFileNameW(nullptr,executable,_countof(executable));
    std::wstring directory=executable;directory.resize(directory.find_last_of(L"\\/"));
    auto file=[&](const wchar_t* label,const std::wstring& path){auto attributes=GetFileAttributesW(path.c_str());report+=label;report+=(attributes!=INVALID_FILE_ATTRIBUTES&&!(attributes&FILE_ATTRIBUTE_DIRECTORY))?L": found\n":L": missing — reinstall Kagami Setup\n";};
    file(L"AirPlay backend",directory+L"\\airplay\\bin\\kagami-uxplay.exe");
    file(L"AirPlay plugin scanner",directory+L"\\airplay\\libexec\\gstreamer-1.0\\gst-plugin-scanner.exe");
    wchar_t path[32768]{};
    report+=SearchPathW(nullptr,L"scrcpy.exe",nullptr,_countof(path),path,nullptr)?L"scrcpy: found in PATH (4.0+ required)\n":L"scrcpy: missing — select Android dependency in Setup\n";
    report+=SearchPathW(nullptr,L"adb.exe",nullptr,_countof(path),path,nullptr)?L"ADB: found in PATH\n":L"ADB: missing — install scrcpy dependency, then restart Kagami\n";
    HKEY key{};auto registered=RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Classes\\CLSID\\{542A79E7-54AC-48F9-B948-4620292BCC85}\\InprocServer32",0,KEY_QUERY_VALUE|KEY_WOW64_64KEY,&key)==ERROR_SUCCESS;
    if(key)RegCloseKey(key);
    report+=registered?L"Kagami camera source: registered\n":L"Kagami camera source: missing — reinstall Kagami Setup\n";
    report+=packageFamily().empty()?L"Smart View package context: absent — install the optional companion\n":L"Smart View package context: present\n";
    report+=L"\nReception checks:\n• AirPlay: phone and PC must share the local network; allow Kagami when Windows asks about firewall access.\n• Smart View: Windows Wireless Display and compatible Wi-Fi drivers/hardware are required; Start reports receiver capability errors.\n• Android: unlock the device and explicitly authorize ADB; Refresh in USB / ADB setup lists authorized devices.\n• Camera apps: permit desktop camera access in Windows Settings and select Kagami. After changing format, reopen the camera app.\n\nThese checks do not start reception or confirm phone/audio/app compatibility.";
    MessageBoxW(parent,report.c_str(),L"Kagami · Check setup",MB_OK|MB_ICONINFORMATION);
}
}
