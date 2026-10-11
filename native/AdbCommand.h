#pragma once
#include <windows.h>
#include <winrt/base.h>
#include <algorithm>
#include <atomic>
#include <string>
#include <vector>
namespace kagami {
inline std::wstring adbPath() {
    wchar_t path[32768]{};
    if(SearchPathW(nullptr,L"adb.exe",nullptr,_countof(path),path,nullptr))return path;
    if(SearchPathW(nullptr,L"scrcpy.exe",nullptr,_countof(path),path,nullptr)) {
        std::wstring sibling=path;sibling=sibling.substr(0,sibling.find_last_of(L"\\/"))+L"\\adb.exe";
        auto attributes=GetFileAttributesW(sibling.c_str());if(attributes!=INVALID_FILE_ATTRIBUTES&&!(attributes&FILE_ATTRIBUTE_DIRECTORY))return sibling;
    }
    throw winrt::hresult_error(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),L"Install Genymobile.scrcpy via winget, then restart Kagami.");
}
inline bool validAdbTarget(const std::wstring& target) {
    return !target.empty()&&target.front()!=L'-'&&target.size()<=180&&target.find_first_not_of(L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._:-[]")==std::wstring::npos;
}
// Wireless debugging uses the explicit private IP and port shown by Android.
inline bool validAdbEndpoint(const std::wstring& target) {
    auto colon=target.find_last_of(L':');
    if(colon==std::wstring::npos||colon==0||colon+1==target.size())return false;
    auto port=target.substr(colon+1);if(port.size()>5||port.find_first_not_of(L"0123456789")!=std::wstring::npos)return false;
    unsigned number=0;for(auto c:port)number=number*10+(c-L'0');if(!number||number>65535)return false;
    auto ip=target.substr(0,colon);
    if(ip.front()==L'['&&ip.back()==L']') {
        ip=ip.substr(1,ip.size()-2);if(ip.empty()||ip.find_first_not_of(L"0123456789abcdefABCDEF:")!=std::wstring::npos)return false;
        // ULA and link-local only; reject malformed compressed/group counts.
        std::wstring lower=ip;for(auto& c:lower)if(c>=L'A'&&c<=L'F')c+=L'a'-L'A';
        auto firstEnd=lower.find(L':');if(firstEnd==std::wstring::npos||firstEnd>4)return false;
        unsigned first=0;for(size_t j=0;j<firstEnd;++j)first=first*16+(lower[j]<=L'9'?lower[j]-L'0':lower[j]-L'a'+10);
        if((first&0xfe00)!=0xfc00&&(first&0xffc0)!=0xfe80)return false;
        if(ip.back()==L':'&&!ip.ends_with(L"::"))return false;
        auto compressed=ip.find(L"::");if(compressed!=std::wstring::npos&&ip.find(L"::",compressed+2)!=std::wstring::npos)return false;
        unsigned groups=0;size_t pos=0;
        while(pos<ip.size()){if(ip[pos]==L':'){++pos;continue;}auto end=ip.find(L':',pos);if(end==std::wstring::npos)end=ip.size();if(end-pos>4)return false;++groups;pos=end;}
        if(ip.find(L":::")!=std::wstring::npos)return false;
        return compressed==std::wstring::npos?groups==8&&ip.front()!=L':'&&ip.back()!=L':':groups<8;
    }
    unsigned octets[4]{};size_t pos=0;
    for(unsigned i=0;i<4;++i){size_t end=ip.find(L'.',pos);if(i==3)end=ip.size();else if(end==std::wstring::npos)return false;
        if(end<=pos||end-pos>3)return false;
        for(size_t j=pos;j<end;++j){if(ip[j]<L'0'||ip[j]>L'9')return false;octets[i]=octets[i]*10+(ip[j]-L'0');}if(octets[i]>255)return false;pos=end+1;}
    return octets[0]==10||(octets[0]==172&&octets[1]>=16&&octets[1]<=31)||(octets[0]==192&&octets[1]==168)||(octets[0]==169&&octets[1]==254);
}
struct SecretClear {std::string& value;~SecretClear(){if(!value.empty())SecureZeroMemory(value.data(),value.size());}};
// No shell, log files or credentials in argv. Only this client is cancelled;
// the user's normal ADB server and its device authorization remain ADB-owned.
inline std::wstring runAdb(const std::wstring& action,const std::wstring& target,std::string secret,std::atomic<bool>& cancelled) {
    SecretClear clearSecret{secret};
    if(cancelled)return L"ADB operation cancelled. Nothing started receiving.";
    if(action==L"pair"&&(secret.size()!=6||secret.find_first_not_of("0123456789")!=std::string::npos))throw winrt::hresult_error(E_INVALIDARG,L"Enter the six-digit Android pairing code.");
    if(action!=L"pair"&&!secret.empty())throw winrt::hresult_error(E_INVALIDARG);
    if((action!=L"devices"&&action!=L"pair"&&action!=L"connect")||(action!=L"devices"&&!validAdbEndpoint(target)))throw winrt::hresult_error(E_INVALIDARG,L"Enter the Wireless debugging IP:port shown on the phone.");
    auto path=adbPath();std::wstring command=L"\""+path+L"\" "+action+(action==L"devices"?L" -l":L" "+target);
    HANDLE read{},write{},inputRead{},inputWrite{},process{};
    struct Cleanup {HANDLE& a;HANDLE& b;HANDLE& c;HANDLE& d;HANDLE& e;~Cleanup(){if(e&&WaitForSingleObject(e,0)==WAIT_TIMEOUT)TerminateProcess(e,1);for(auto h:{a,b,c,d,e})if(h&&h!=INVALID_HANDLE_VALUE)CloseHandle(h);}} cleanup{read,write,inputRead,inputWrite,process};
    SECURITY_ATTRIBUTES sa{sizeof(sa),nullptr,TRUE};
    if(!CreatePipe(&read,&write,&sa,4096)||!CreatePipe(&inputRead,&inputWrite,&sa,64))throw winrt::hresult_error(E_FAIL);
    if(!SetHandleInformation(read,HANDLE_FLAG_INHERIT,0)||!SetHandleInformation(inputWrite,HANDLE_FLAG_INHERIT,0))throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()));
    STARTUPINFOEXW startup{};startup.StartupInfo.cb=sizeof(startup);startup.StartupInfo.dwFlags=STARTF_USESTDHANDLES;startup.StartupInfo.hStdOutput=startup.StartupInfo.hStdError=write;startup.StartupInfo.hStdInput=inputRead;
    SIZE_T size{};InitializeProcThreadAttributeList(nullptr,1,0,&size);std::vector<BYTE> attributes(size);startup.lpAttributeList=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
    if(!InitializeProcThreadAttributeList(startup.lpAttributeList,1,0,&size))throw winrt::hresult_error(E_FAIL);
    HANDLE handles[]{write,inputRead};PROCESS_INFORMATION child{};
    bool launched=UpdateProcThreadAttribute(startup.lpAttributeList,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,handles,sizeof(handles),nullptr,nullptr)&&CreateProcessW(path.c_str(),command.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW|EXTENDED_STARTUPINFO_PRESENT,nullptr,nullptr,&startup.StartupInfo,&child);
    auto error=GetLastError();DeleteProcThreadAttributeList(startup.lpAttributeList);
    if(!launched)throw winrt::hresult_error(HRESULT_FROM_WIN32(error));
    process=child.hProcess;CloseHandle(child.hThread);CloseHandle(write);write=nullptr;CloseHandle(inputRead);inputRead=nullptr;
    if(!secret.empty()){secret+='\n';DWORD count{};if(!WriteFile(inputWrite,secret.data(),DWORD(secret.size()),&count,nullptr)||count!=secret.size()){TerminateProcess(process,1);throw winrt::hresult_error(E_FAIL,L"Cannot send pairing code to ADB.");}SecureZeroMemory(secret.data(),secret.size());secret.clear();}
    CloseHandle(inputWrite);inputWrite=nullptr;
    std::string output;ULONGLONG start=GetTickCount64();
    while(!cancelled&&GetTickCount64()-start<15000){
        DWORD available{};if(PeekNamedPipe(read,nullptr,0,nullptr,&available,nullptr)&&available){char buffer[512];DWORD count{};if(ReadFile(read,buffer,(std::min)(available,DWORD(sizeof(buffer))),&count,nullptr)&&count){if(output.size()+count<=16384)output.append(buffer,count);}}
        else if(WaitForSingleObject(process,0)!=WAIT_TIMEOUT)break;
        Sleep(10);
    }
    if(cancelled||GetTickCount64()-start>=15000){TerminateProcess(process,1);return L"ADB operation cancelled or timed out. Nothing started receiving.";}
    DWORD exit{};GetExitCodeProcess(process,&exit);
    if(action==L"pair")return !exit&&output.find("Successfully paired")!=std::string::npos?L"Android pairing completed. Connect using the separate debugging IP:port.":L"Pairing failed. Use a fresh code and the pairing port shown by Android.";
    int length=MultiByteToWideChar(CP_UTF8,0,output.data(),int(output.size()),nullptr,0);std::wstring result(length,L'\0');MultiByteToWideChar(CP_UTF8,0,output.data(),int(output.size()),result.data(),length);
    if(!exit&&action==L"connect"&&result.find(L"connected to")==std::wstring::npos)return L"Cannot connect. Check the debugging port and local network.";
    if(result.empty())result=exit?L"ADB failed. Check phone authorization and debugging ports.":L"ADB operation completed.";
    return result;
}
}
