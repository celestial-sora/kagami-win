#pragma once
#include <userenv.h>
#include "SmartViewProtocol.h"
#include "PackageActivation.h"
#pragma comment(lib,"userenv.lib")
namespace kagami {
class SmartViewReceiver {
    HANDLE pipe=INVALID_HANDLE_VALUE;
    std::thread reader;
    std::atomic<bool> stopped{true},ended{false};
    std::atomic<DWORD> expectedPid{0};
    std::mutex lock;
    smart::Packet status;
    std::vector<BYTE> latest;
    ULONGLONG started{};
    std::atomic<ULONGLONG> received{0};
public:
    ~SmartViewReceiver(){stop();}
    bool active()const{return !stopped;}
    void stop()noexcept {
        stopped=true;if(reader.joinable())reader.join();
        if(pipe!=INVALID_HANDLE_VALUE){DisconnectNamedPipe(pipe);CloseHandle(pipe);pipe=INVALID_HANDLE_VALUE;}
        std::lock_guard guard(lock);latest.clear();status={};expectedPid=0;
    }
    void start() {
        stop();auto family=packageFamily();
        if(family.empty())throw winrt::hresult_error(E_FAIL,L"Install the optional Smart View companion from Kagami Setup, then reopen Kagami.");
        GUID id{};winrt::check_hresult(CoCreateGuid(&id));wchar_t guid[40]{};StringFromGUID2(id,guid,40);
        std::wstring name=L"\\\\.\\pipe\\LOCAL\\Kagami.SmartView."+std::wstring(guid+1,36);
        PSID packageSid{};winrt::check_hresult(HRESULT_FROM_WIN32(DeriveAppContainerSidFromAppContainerName(family.c_str(),&packageSid)));
        LPWSTR sidText{};if(!ConvertSidToStringSidW(packageSid,&sidText)){FreeSid(packageSid);throw winrt::hresult_error(E_FAIL);}FreeSid(packageSid);
        std::wstring acl=L"D:P(A;;GA;;;"+userSid()+L")(A;;GRGW;;;"+sidText+L")S:(ML;;NW;;;LW)";LocalFree(sidText);
        PSECURITY_DESCRIPTOR descriptor{};
        if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(acl.c_str(),SDDL_REVISION_1,&descriptor,nullptr))throw winrt::hresult_error(E_ACCESSDENIED);
        SECURITY_ATTRIBUTES sa{sizeof(sa),descriptor,FALSE};
        pipe=CreateNamedPipeW(name.c_str(),PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED|FILE_FLAG_FIRST_PIPE_INSTANCE,
            PIPE_TYPE_BYTE|PIPE_WAIT|PIPE_REJECT_REMOTE_CLIENTS,1,1,frameBytes+sizeof(smart::Packet),0,&sa);LocalFree(descriptor);
        if(pipe==INVALID_HANDLE_VALUE)throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()));
        stopped=false;ended=false;started=GetTickCount64();received=started;
        try {
            reader=std::thread([this] {
                OVERLAPPED ov{};ov.hEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);
                BOOL connected=ov.hEvent?ConnectNamedPipe(pipe,&ov):FALSE;DWORD error=GetLastError();
                if(ov.hEvent&&!connected&&error==ERROR_IO_PENDING){
                    while(WaitForSingleObject(ov.hEvent,50)==WAIT_TIMEOUT&&!stopped&&GetTickCount64()-started<15000){}
                    if(stopped||GetTickCount64()-started>=15000)CancelIoEx(pipe,&ov);
                    DWORD ignored{};connected=GetOverlappedResult(pipe,&ov,&ignored,TRUE);
                }else if(ov.hEvent&&!connected&&error==ERROR_PIPE_CONNECTED)connected=TRUE;
                if(ov.hEvent)CloseHandle(ov.hEvent);
                while(!stopped&&!expectedPid&&GetTickCount64()-started<15000)Sleep(10);
                ULONG pid{};
                if(connected&&GetNamedPipeClientProcessId(pipe,&pid)&&pid==expectedPid&&pid) {
                    smart::Packet packet;std::vector<BYTE> pixels(frameBytes);
                    while(!stopped&&transfer(pipe,&packet,sizeof(packet),false,stopped)&&smart::valid(packet)) {
                        if(packet.bytes&&!transfer(pipe,pixels.data(),packet.bytes,false,stopped))break;
                        std::lock_guard guard(lock);status=packet;if(packet.bytes)latest=pixels;else latest.clear();received=GetTickCount64();
                    }
                }
                {std::lock_guard guard(lock);latest.clear();}ended=true;
            });
            expectedPid=activatePackage(family,L"Receiver",name+L" "+std::to_wstring(GetCurrentProcessId()));
        }catch(...){stop();throw;}
    }
    std::vector<BYTE> take(std::wstring& message) {
        if(ended)throw winrt::hresult_error(E_FAIL,L"Smart View receiver closed or disconnected. Output cleared; restart explicitly.");
        if(GetTickCount64()-received>15000)throw winrt::hresult_error(HRESULT_FROM_WIN32(WAIT_TIMEOUT),L"Smart View receiver timed out. Output cleared; restart explicitly.");
        std::lock_guard guard(lock);
        if(status.state==smart::State::Failed)throw winrt::hresult_error(status.error,status.detail);
        message=status.detail;
        return latest;
    }
};
}
