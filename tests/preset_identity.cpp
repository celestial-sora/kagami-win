#include <cassert>
#include <string>
#include "PresetIdentity.h"
int main(){
    assert(kagami::stableUsbIdentity(L"R58M123456A"));
    assert(kagami::stableUsbIdentity(std::wstring(180,L'A')));
    for(auto serial:{L"",L"-s",L"10.0.0.2:5555",L"10.0.0.2",L"[fd00::1]:5555",L"adb-device._adb-tls-connect._tcp",L"ADB-device._ADB-TLS-PAIRING._TCP",L"device name",L"device\\other"})assert(!kagami::stableUsbIdentity(serial));
    assert(!kagami::stableUsbIdentity(std::wstring(181,L'A')));
    assert(kagami::presetLabelValid(L"Default"));
    assert(kagami::presetLabelValid(L"Discord · portrait"));
    assert(kagami::presetLabelValid(std::wstring(64,L'A')));
    for(auto label:{L"",L" ",L"app/path",L"app\\path",L"app\nother",L"app\r",L"app\tother"})assert(!kagami::presetLabelValid(label));
    assert(!kagami::presetLabelValid(std::wstring(65,L'A')));
    assert(!kagami::presetLabelValid(std::wstring(L"a\0b",3)));
    auto portrait=kagami::androidPresetKey(L"R58M123456A",L"Discord",1);
    auto landscape=kagami::androidPresetKey(L"R58M123456A",L"Discord",2);
    assert(!portrait.empty()&&!landscape.empty()&&portrait!=landscape);
    assert(portrait!=kagami::androidPresetKey(L"R58M987654B",L"Discord",1));
    assert(portrait!=kagami::androidPresetKey(L"R58M123456A",L"OBS",1));
    assert(portrait.ends_with(L"\\Portrait")&&landscape.ends_with(L"\\Landscape"));
    assert(kagami::androidPresetKey(L"10.0.0.2:5555",L"Discord",1).empty());
    assert(kagami::androidPresetKey(L"10.0.0.2",L"Discord",1).empty());
    assert(kagami::androidPresetKey(L"adb-phone._adb-tls-connect._tcp",L"Discord",2).empty());
    assert(kagami::androidPresetKey(L"R58M123456A",L"../Discord",1).empty());
    assert(kagami::androidPresetKey(L"R58M123456A",L"Discord",0).empty());
    assert(kagami::androidPresetKey(L"R58M123456A",L"Discord",3).empty());
}
