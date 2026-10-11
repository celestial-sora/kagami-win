#pragma once
#include "FrameBridge.h"
namespace kagami::smart {
enum class State : uint32_t { Listening=1, Video=2, Pin=3, Failed=4 };
struct Packet { uint32_t magic=0x31564D4B; State state=State::Listening; uint32_t bytes=0; HRESULT error=S_OK; wchar_t detail[96]{}; };
inline bool valid(const Packet& p) {
    return p.magic==0x31564D4B && unsigned(p.state)>=1 && unsigned(p.state)<=4 &&
        p.bytes==(p.state==State::Video?frameBytes:0) && p.detail[95]==0;
}
inline bool validEndpoint(const std::wstring& value) {
    const std::wstring prefix=L"\\\\.\\pipe\\LOCAL\\Kagami.SmartView.";
    if(value.size()!=prefix.size()+36 || value.compare(0,prefix.size(),prefix))return false;
    for(size_t i=prefix.size();i<value.size();++i)if(!iswxdigit(value[i])&&value[i]!=L'-')return false;
    return true;
}
}
