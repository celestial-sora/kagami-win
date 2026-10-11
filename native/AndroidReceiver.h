#pragma once
#include <string>
#include "ReceiverWindow.h"
namespace kagami {
class AndroidReceiver {
    HANDLE process{},job{};
    DWORD pid{};
    HWND window{};
    ULONGLONG started{};
    ReceiverWindow capture;
    struct WindowSearch {DWORD pid;HWND found;};
    static BOOL CALLBACK findWindow(HWND hwnd,LPARAM value) {
        auto search=reinterpret_cast<WindowSearch*>(value);DWORD owner{};GetWindowThreadProcessId(hwnd,&owner);
        if(owner==search->pid&&IsWindowVisible(hwnd)&&GetWindow(hwnd,GW_OWNER)==nullptr){search->found=hwnd;return FALSE;}return TRUE;
    }
public:
    ~AndroidReceiver(){stop();}
    bool active() const{return process!=nullptr;}
    void stop() noexcept {
        capture.stop();window=nullptr;
        if(job){TerminateJobObject(job,0);CloseHandle(job);job=nullptr;}
        if(process){CloseHandle(process);process=nullptr;}pid=0;
    }
    void start(const std::wstring& serial) {
        stop();
        if(serial.empty()||serial.size()>180||serial.find_first_not_of(L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._:-")!=std::wstring::npos)
            throw winrt::hresult_error(E_INVALIDARG,L"Enter an already authorized ADB device serial or debugging IP:port.");
        wchar_t path[32768]{};
        if(!SearchPathW(nullptr,L"scrcpy.exe",nullptr,ARRAYSIZE(path),path,nullptr))throw winrt::hresult_error(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),L"Install Genymobile.scrcpy through winget, then restart Kagami.");
        std::wstring command=L"\""+std::wstring(path)+L"\" --serial="+serial+L" --no-audio --no-control --max-size=1920 --max-fps=30 --window-title=Kagami-Android-input --window-borderless --window-width=1280 --window-height=720 --no-window-aspect-ratio-lock --background-color=000000";
        job=CreateJobObjectW(nullptr,nullptr);if(!job)throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()));
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if(!SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limits,sizeof(limits))){stop();throw winrt::hresult_error(E_FAIL);}
        STARTUPINFOW info{};info.cb=sizeof(info);PROCESS_INFORMATION child{};
        // Start suspended so no receiver can escape the job before ownership is established.
        std::wstring directory=path;directory=directory.substr(0,directory.find_last_of(L"\\/"));
        if(!CreateProcessW(path,command.data(),nullptr,nullptr,FALSE,CREATE_SUSPENDED|CREATE_NO_WINDOW,nullptr,directory.c_str(),&info,&child)){auto error=GetLastError();stop();throw winrt::hresult_error(HRESULT_FROM_WIN32(error));}
        process=child.hProcess;pid=child.dwProcessId;
        if(!AssignProcessToJobObject(job,process)){TerminateProcess(process,1);CloseHandle(child.hThread);stop();throw winrt::hresult_error(E_ACCESSDENIED,L"Cannot own the Android receiver process.");}
        if(ResumeThread(child.hThread)==DWORD(-1)){CloseHandle(child.hThread);stop();throw winrt::hresult_error(E_FAIL);}
        CloseHandle(child.hThread);started=GetTickCount64();
    }
    std::vector<BYTE> take() {
        if(!process)return {};
        if(WaitForSingleObject(process,0)!=WAIT_TIMEOUT){stop();throw winrt::hresult_error(E_FAIL,L"Android receiver stopped. Unlock the phone, authorize ADB, and check scrcpy 3.0+.");}
        if(!window){WindowSearch search{pid,nullptr};EnumWindows(findWindow,reinterpret_cast<LPARAM>(&search));
            if(!search.found){if(GetTickCount64()-started>15000){stop();throw winrt::hresult_error(HRESULT_FROM_WIN32(WAIT_TIMEOUT),L"No Android receiver window appeared. Check ADB authorization and scrcpy.");}return {};}
            window=search.found;capture.start(window,pid);
        }
        if(!IsWindow(window)||IsIconic(window)){stop();throw winrt::hresult_error(E_FAIL,L"Android receiver closed or minimized. Output cleared; restart reception explicitly.");}
        return capture.take();
    }
};
}
