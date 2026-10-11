#pragma once
#include <appmodel.h>
#include <shobjidl.h>
#include <wrl/client.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Management.Deployment.h>
#pragma comment(lib,"windowsapp.lib")
namespace kagami {
inline std::wstring packageFamily() {
    UINT32 size{};if(GetCurrentPackageFamilyName(&size,nullptr)!=ERROR_INSUFFICIENT_BUFFER)return {};
    std::wstring family(size,L'\0');if(GetCurrentPackageFamilyName(&size,family.data())!=ERROR_SUCCESS)return {};
    family.resize(size-1);return family;
}
inline DWORD activatePackage(const std::wstring& family,const wchar_t* app,const std::wstring& args) {
    Microsoft::WRL::ComPtr<IApplicationActivationManager> activation;
    winrt::check_hresult(CoCreateInstance(CLSID_ApplicationActivationManager,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&activation)));
    DWORD pid{};winrt::check_hresult(activation->ActivateApplication((family+L"!"+app).c_str(),args.c_str(),AO_NONE,&pid));return pid;
}
// The EXE remains the setup/launch entry point. A registered companion contains
// both the unprivileged desktop and the CoreApplicationView receiver.
inline bool launchRegisteredDesktop() {
    if(!packageFamily().empty())return false;
    try {
        winrt::Windows::Management::Deployment::PackageManager manager;
        for(auto const& package:manager.FindPackagesForUser(L"")) {
            if(package.Id().Name()==L"CelestialSora.Kagami.Development" && package.Id().Publisher()==L"CN=Kagami Development Preview") {
                activatePackage(package.Id().FamilyName().c_str(),L"Desktop",L"");return true;
            }
        }
    }catch(...){}
    return false;
}
}
