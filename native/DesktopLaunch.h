#pragma once
#include <windows.h>
#include <exdisp.h>
#include <shlobj.h>
#include <shldisp.h>
#include <shlguid.h>
#include <wrl/client.h>

namespace kagami {
// Use the existing desktop user's Explorer, rather than a restricted admin
// token. An over-the-shoulder UAC login can belong to a different account.
inline HRESULT launchOnDesktop(const wchar_t* executable, int show) {
    using Microsoft::WRL::ComPtr;
    ComPtr<IShellWindows> windows;
    HRESULT hr = CoCreateInstance(CLSID_ShellWindows, nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&windows));
    if (FAILED(hr)) return hr;
    VARIANT empty{};
    long desktopHandle{};
    ComPtr<IDispatch> desktop;
    hr = windows->FindWindowSW(&empty, &empty, SWC_DESKTOP, &desktopHandle, SWFO_NEEDDISPATCH, &desktop);
    if (FAILED(hr)) return hr;
    if (!desktop) return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    ComPtr<IServiceProvider> services;
    hr = desktop.As(&services); if (FAILED(hr)) return hr;
    ComPtr<IShellBrowser> browser;
    hr = services->QueryService(SID_STopLevelBrowser, IID_PPV_ARGS(&browser)); if (FAILED(hr)) return hr;
    HWND shellWindow{};
    hr = browser->GetWindow(&shellWindow); if (FAILED(hr)) return hr;
    DWORD processId{};
    GetWindowThreadProcessId(shellWindow, &processId);
    if (!processId) return E_FAIL;
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (!process) return HRESULT_FROM_WIN32(GetLastError());
    HANDLE token{};
    BOOL opened = OpenProcessToken(process, TOKEN_QUERY, &token);
    DWORD error = opened ? ERROR_SUCCESS : GetLastError();
    CloseHandle(process);
    if (!opened) return HRESULT_FROM_WIN32(error);
    TOKEN_ELEVATION elevation{}; DWORD bytes{};
    BOOL queried = GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &bytes);
    error = queried ? ERROR_SUCCESS : GetLastError();
    CloseHandle(token);
    if (!queried) return HRESULT_FROM_WIN32(error);
    if (elevation.TokenIsElevated) return E_ACCESSDENIED;
    ComPtr<IShellView> view;
    hr = browser->QueryActiveShellView(&view); if (FAILED(hr)) return hr;
    ComPtr<IDispatch> background;
    hr = view->GetItemObject(SVGIO_BACKGROUND, IID_PPV_ARGS(&background)); if (FAILED(hr)) return hr;
    ComPtr<IShellFolderViewDual> folder;
    hr = background.As(&folder); if (FAILED(hr)) return hr;
    ComPtr<IDispatch> application;
    hr = folder->get_Application(&application); if (FAILED(hr)) return hr;
    ComPtr<IShellDispatch2> shell;
    hr = application.As(&shell); if (FAILED(hr)) return hr;
    BSTR path = SysAllocString(executable);
    if (!path) return E_OUTOFMEMORY;
    VARIANT display{}; display.vt = VT_I4; display.lVal = show;
    hr = shell->ShellExecute(path, empty, empty, empty, display);
    SysFreeString(path);
    return hr;
}
}
