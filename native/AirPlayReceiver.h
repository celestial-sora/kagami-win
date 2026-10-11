#pragma once
#include "OwnedProcess.h"
#include "ReceiverWindow.h"
namespace kagami {
class AirPlayReceiver {
    OwnedProcess child;
    ReceiverWindow capture;
    HWND window{};
    ULONGLONG started{};
    struct Search {DWORD pid;HWND window;};
    static BOOL CALLBACK findWindow(HWND hwnd,LPARAM value){auto search=reinterpret_cast<Search*>(value);DWORD pid{};GetWindowThreadProcessId(hwnd,&pid);if(pid==search->pid&&IsWindowVisible(hwnd)&&GetWindow(hwnd,GW_OWNER)==nullptr){search->window=hwnd;return FALSE;}return TRUE;}
public:
    bool active()const{return child.active();}
    void stop()noexcept{capture.stop();window=nullptr;child.stop();}
    ~AirPlayReceiver(){stop();}
    void start() {
        stop();wchar_t executable[32768]{};GetModuleFileNameW(nullptr,executable,ARRAYSIZE(executable));
        std::wstring directory=executable;directory=directory.substr(0,directory.find_last_of(L"\\/"))+L"\\airplay\\bin";
        std::wstring path=directory+L"\\kagami-uxplay.exe";
        if(GetFileAttributesW(path.c_str())==INVALID_FILE_ATTRIBUTES)throw winrt::hresult_error(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),L"Install the Kagami development setup with its AirPlay backend.");
        std::wstring command=L"\""+path+L"\" -rc NUL -n Kagami -nh -s 1280x720@30 -p 35000 -vs \"d3d11videosink sync=false\" -as \"wasapisink sync=true\"";
        child.start(path,command,directory,true);started=GetTickCount64();
    }
    std::vector<BYTE> take() {
        if(!active())return {};
        if(child.exited())throw winrt::hresult_error(E_FAIL,L"AirPlay backend stopped. Check discovery, Windows firewall and audio output.");
        if(child.disconnected)throw winrt::hresult_error(E_FAIL,L"AirPlay disconnected. Output cleared. Restart reception explicitly.");
        if(child.audioFailed)throw winrt::hresult_error(E_FAIL,L"AirPlay desktop audio failed. Check the Windows sound output and restart reception.");
        if(!child.ready){if(GetTickCount64()-started>5000)throw winrt::hresult_error(E_FAIL,L"AirPlay backend does not report the required Kagami lifecycle contract.");return {};}
        if(!child.video)return {};
        if(!window){Search search{child.pid(),nullptr};EnumWindows(findWindow,reinterpret_cast<LPARAM>(&search));if(!search.window)return {};window=search.window;capture.start(window,child.pid());}
        if(!IsWindow(window)||IsIconic(window))throw winrt::hresult_error(E_FAIL,L"AirPlay receiver window closed or minimized. Output cleared; restart reception.");
        return capture.take();
    }
};
}
