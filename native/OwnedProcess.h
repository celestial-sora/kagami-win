#pragma once
#include <atomic>
#include <thread>
#include <string>
#include <vector>
namespace kagami {
class OwnedProcess {
    HANDLE process{},job{},output{};
    DWORD id{};
    std::thread reader;
public:
    std::atomic<bool> ready{false},video{false},disconnected{false},audioFailed{false};
    ~OwnedProcess(){stop();}
    DWORD pid()const{return id;}
    bool active()const{return process!=nullptr;}
    bool exited()const{return process&&WaitForSingleObject(process,0)!=WAIT_TIMEOUT;}
    void stop() noexcept {
        if(job){TerminateJobObject(job,0);CloseHandle(job);job=nullptr;}
        if(reader.joinable())reader.join();
        if(output){CloseHandle(output);output=nullptr;}
        if(process){CloseHandle(process);process=nullptr;}
        id=0;ready=false;video=false;disconnected=false;audioFailed=false;
    }
    void start(const std::wstring& path,std::wstring command,const std::wstring& directory) {
        stop();HANDLE write{},input{};
        SECURITY_ATTRIBUTES security{sizeof(security),nullptr,TRUE};
        if(!CreatePipe(&output,&write,&security,4096))throw winrt::hresult_error(HRESULT_FROM_WIN32(GetLastError()));
        SetHandleInformation(output,HANDLE_FLAG_INHERIT,0);
        input=CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,OPEN_EXISTING,0,nullptr);
        job=CreateJobObjectW(nullptr,nullptr);
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if(input==INVALID_HANDLE_VALUE||!job||!SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limits,sizeof(limits))){CloseHandle(write);if(input!=INVALID_HANDLE_VALUE)CloseHandle(input);stop();throw winrt::hresult_error(E_FAIL);}
        STARTUPINFOEXW info{};info.StartupInfo.cb=sizeof(info);info.StartupInfo.dwFlags=STARTF_USESTDHANDLES;info.StartupInfo.hStdInput=input;info.StartupInfo.hStdOutput=info.StartupInfo.hStdError=write;
        SIZE_T bytes{};InitializeProcThreadAttributeList(nullptr,1,0,&bytes);std::vector<BYTE> attributes(bytes);info.lpAttributeList=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
        if(!InitializeProcThreadAttributeList(info.lpAttributeList,1,0,&bytes)){CloseHandle(write);CloseHandle(input);stop();throw winrt::hresult_error(E_FAIL);}
        HANDLE handles[]{write,input};PROCESS_INFORMATION child{};
        bool success=UpdateProcThreadAttribute(info.lpAttributeList,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,handles,sizeof(handles),nullptr,nullptr)&&CreateProcessW(path.c_str(),command.data(),nullptr,nullptr,TRUE,CREATE_SUSPENDED|CREATE_NO_WINDOW|EXTENDED_STARTUPINFO_PRESENT,nullptr,directory.c_str(),&info.StartupInfo,&child);
        DWORD error=GetLastError();DeleteProcThreadAttributeList(info.lpAttributeList);CloseHandle(write);CloseHandle(input);
        if(!success){stop();throw winrt::hresult_error(HRESULT_FROM_WIN32(error));}
        process=child.hProcess;id=child.dwProcessId;
        if(!AssignProcessToJobObject(job,process)){TerminateProcess(process,1);CloseHandle(child.hThread);stop();throw winrt::hresult_error(E_ACCESSDENIED);}
        reader=std::thread([this]{char buffer[512];std::string line;bool overflow=false;DWORD count{};
            while(ReadFile(output,buffer,sizeof(buffer),&count,nullptr)&&count)for(DWORD i=0;i<count;++i){
                char c=buffer[i];if(c=='\n'){
                    if(!overflow){if(line=="KAGAMI_AIRPLAY_BUILD_V1")ready=true;if(line=="KAGAMI_AIRPLAY_VIDEO")video=true;if(line=="KAGAMI_AIRPLAY_DISCONNECTED")disconnected=true;if(line=="KAGAMI_AIRPLAY_AUDIO_FAILED")audioFailed=true;}
                    line.clear();overflow=false;
                }else if(c!='\r'){if(line.size()<512)line+=c;else overflow=true;}
            }
        });
        DWORD resumed=ResumeThread(child.hThread);CloseHandle(child.hThread);
        if(resumed==DWORD(-1)){stop();throw winrt::hresult_error(E_FAIL);}
    }
};
}
