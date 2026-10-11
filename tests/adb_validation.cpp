#include <windows.h>
#include <winrt/base.h>
#include <atomic>
#include <string>
#include <vector>
#include <cassert>
#include "AdbCommand.h"

// These fixtures validate only parsing and rejected/pre-cancelled commands.
// No ADB executable is resolved, no process is created, and no network is used.
int main() {
    for(const auto* target:{L"R58M123456A",L"device-name_1.2",L"10.0.0.2:37001",L"[fd00::1]:5555",L"adb-device._adb-tls-connect._tcp"})
        assert(kagami::validAdbTarget(target));
    for(const auto* target:{L"",L"-s",L"serial name",L"serial\n",L"\"quoted\"",L"a&b",L"a/b",L"a\\b",L"device;command",L"device=other"})
        assert(!kagami::validAdbTarget(target));
    assert(kagami::validAdbTarget(std::wstring(180,L'a')));
    assert(!kagami::validAdbTarget(std::wstring(181,L'a')));

    for(const auto* endpoint:{L"10.0.0.2:37001",L"192.168.1.2:5555",L"172.16.0.2:1",L"172.31.255.254:65535",L"169.254.1.2:5555",L"[fd00::1]:5555",L"[FC00:1:2:3:4:5:6:7]:5555",L"[fe80::1]:5555",L"[febf::1]:65535",L"[fd00::]:1"})
        assert(kagami::validAdbEndpoint(endpoint));
    for(const auto* endpoint:{
        L"",L"localhost:5555",L"127.0.0.1:5555",L"0.0.0.0:5555",L"8.8.8.8:5555",L"224.0.0.1:5555",
        L"172.15.0.2:5555",L"172.32.0.2:5555",L"192.169.1.2:5555",L"169.253.1.2:5555",
        L"10.0.0.2:0",L"10.0.0.2:65536",L"10.0.0.2:999999",L"10.0.0.2:-1",L"10.0.0.2:+5555",L"10.0.0.2:abc",
        L"10.0.0.2",L"10.0.0.2:",L":5555",L"10.0.0:5555",L"10.0.0.2.3:5555",L"10.0.0.256:5555",L"10..0.2:5555",
        L" 10.0.0.2:5555",L"10.0.0.2:5555\n",L"10.0.0.2:5555 other",L"-bad:5555",L"10.0.0.2:5555&command",
        L"[::1]:5555",L"[::]:5555",L"[ff02::1]:5555",L"[2001:db8::1]:5555",L"[fc::1]:5555",L"[fe7f::1]:5555",L"[fec0::1]:5555",
        L"fd00::1:5555",L"[fd00::1:5555",L"[fd00::1]5555",L"[fd00:::1]:5555",L"[fd00::1::2]:5555",L"[fd00::1:]:5555",
        L"[fd00:1:2:3:4:5:6]:5555",L"[fd00:1:2:3:4:5:6:7:8]:5555",L"[fd00::1:2:3:4:5:6:7]:5555",L"[fd00:12345::1]:5555",L"[fd00::g]:5555"})
        assert(!kagami::validAdbEndpoint(endpoint));

    std::atomic<bool> cancelled{false};
    auto rejected=[&](const std::wstring& action,const std::wstring& target,const std::string& code){
        bool invalid=false;
        try {kagami::runAdb(action,target,code,cancelled);}
        catch(const winrt::hresult_error& error){invalid=error.code()==E_INVALIDARG;}
        assert(invalid);
    };
    for(const auto* code:{"","12345","1234567","abc123","12345\n"})
        rejected(L"pair",L"10.0.0.2:37001",code);
    rejected(L"pair",L"8.8.8.8:37001","123456");
    rejected(L"pair",L"10.0.0.2:0","123456");
    rejected(L"connect",L"10.0.0.2:37001","123456");
    rejected(L"devices",L"","123456");
    rejected(L"shell",L"10.0.0.2:37001","");
    cancelled=true;
    assert(kagami::runAdb(L"pair",L"10.0.0.2:37001","123456",cancelled).find(L"cancelled")!=std::wstring::npos);
    std::string code="123456";
    {kagami::SecretClear clear{code};}
    for(auto byte:code)assert(byte=='\0');
}
