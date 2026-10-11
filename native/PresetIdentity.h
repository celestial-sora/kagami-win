#pragma once
#include <string>
namespace kagami {
inline bool presetLabelValid(const std::wstring& label) {
    return !label.empty()&&label.size()<=64&&label.find_first_of(L"\\/\r\n\t") == std::wstring::npos&&label.find(wchar_t(0))==std::wstring::npos&&label.find_first_not_of(L" ")!=std::wstring::npos;
}
// TCP/IP and mDNS ADB targets are transient network identities, not hardware IDs.
inline bool stableUsbIdentity(const std::wstring& serial) {
    if(serial.empty()||serial.size()>180||serial.front()==L'-'||serial.find_first_of(L":[]")!=std::wstring::npos)return false;
    if(serial.find(L'.')!=std::wstring::npos&&serial.find_first_not_of(L"0123456789.")==std::wstring::npos)return false;
    auto lower=serial;for(auto& c:lower)if(c>=L'A'&&c<=L'Z')c+=L'a'-L'A';
    if(lower.find(L"_adb-tls-")!=std::wstring::npos||lower.find(L"._tcp")!=std::wstring::npos)return false;
    return serial.find_first_not_of(L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-")==std::wstring::npos;
}
inline std::wstring androidPresetKey(const std::wstring& serial,const std::wstring& label,unsigned orientation) {
    if(!stableUsbIdentity(serial)||!presetLabelValid(label)||(orientation!=1&&orientation!=2))return {};
    return L"Software\\Kagami\\AndroidPresets\\"+serial+L"\\"+label+(orientation==1?L"\\Portrait":L"\\Landscape");
}
}
