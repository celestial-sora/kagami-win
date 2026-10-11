#include <windows.h>
#include <mfapi.h>
#include <mfvirtualcamera.h>
#include <wrl/client.h>
#include <shellapi.h>
#include <string>
#include "FrameBridge.h"
#include "source/VirtualCameraMediaSource.h"
using Microsoft::WRL::ComPtr;
static ComPtr<IMFVirtualCamera> camera;
static kagami::FrameWriter writer;
static HWND status;
static unsigned tick;
static bool elevated() {
    HANDLE token{}; TOKEN_ELEVATION value{}; DWORD bytes{};
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return true;
    bool result = !GetTokenInformation(token, TokenElevation, &value, sizeof(value), &bytes) || value.TokenIsElevated;
    CloseHandle(token); return result;
}
static HRESULT openCamera() {
    if (camera) return S_OK;
    return MFCreateVirtualCamera(MFVirtualCameraType_SoftwareCameraSource, MFVirtualCameraLifetime_System,
        MFVirtualCameraAccess_CurrentUser, L"Kagami", L"{542A79E7-54AC-48F9-B948-4620292BCC85}", nullptr, 0, &camera);
}
static void report(HRESULT hr, const wchar_t* success) {
    if (SUCCEEDED(hr)) SetWindowTextW(status, success);
    else { wchar_t message[300]; swprintf_s(message, L"Operation failed (0x%08X). Install Kagami Setup, and check Windows camera privacy settings.", static_cast<unsigned>(hr)); SetWindowTextW(status, message); }
}
static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM w, LPARAM l) {
    switch (message) {
    case WM_CREATE:
        CreateWindowW(L"STATIC", L"Kagami for Windows · Native camera development preview", WS_CHILD | WS_VISIBLE, 22, 20, 620, 24, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"STATIC", L"Camera output: 1280 × 720 at 30 FPS\nAirPlay, Smart View and USB inputs are still under development.", WS_CHILD | WS_VISIBLE, 22, 55, 620, 55, hwnd, nullptr, nullptr, nullptr);
        {
        const wchar_t* labels[]{L"Enable camera", L"Test pattern", L"Stop", L"Remove camera"};
        for (int i=0; i<4; ++i) CreateWindowW(L"BUTTON", labels[i], WS_CHILD | WS_VISIBLE, 22 + i*154, 120, 145, 34, hwnd, reinterpret_cast<HMENU>(INT_PTR(100+i)), nullptr, nullptr);
        }
        status = CreateWindowW(L"STATIC", L"Stopped. No phone, camera or desktop capture has started.", WS_CHILD | WS_VISIBLE, 22, 180, 610, 90, hwnd, nullptr, nullptr, nullptr);
        return 0;
    case WM_COMMAND:
        if (LOWORD(w) == 100) {
            HRESULT hr = openCamera();
            if (SUCCEEDED(hr)) hr = camera->SetString(KAGAMI_PIPE_NAME, kagami::pipeName().c_str());
            if (SUCCEEDED(hr)) hr = camera->Start(nullptr);
            report(hr, L"Camera enabled. Choose Kagami Windows Virtual Camera in OBS or Discord. Waiting output is black.");
        } else if (LOWORD(w) == 101) {
            if (!camera) { SetWindowTextW(status, L"Enable the Kagami camera first."); return 0; }
            if (!writer.start()) { SetWindowTextW(status, L"Cannot own the Kagami output. Close the other Kagami producer first."); return 0; }
            tick = 0; SetTimer(hwnd, 1, 33, nullptr);
            SetWindowTextW(status, L"Synthetic test pattern running. This checks camera output, not phone compatibility.");
        } else if (LOWORD(w) == 102) {
            KillTimer(hwnd, 1); writer.stop(); SetWindowTextW(status, L"Stopped. Enabled camera returns black frames.");
        } else if (LOWORD(w) == 103) {
            KillTimer(hwnd, 1); writer.stop();
            HRESULT hr = openCamera(); if (SUCCEEDED(hr)) hr = camera->Remove(); camera.Reset();
            report(hr, L"Kagami camera removed for this user.");
        }
        return 0;
    case WM_TIMER: {
        std::vector<BYTE> pixels(kagami::frameBytes);
        for (unsigned y=0; y<kagami::height; ++y) for (unsigned x=0; x<kagami::width; ++x) {
            size_t i=(y*kagami::width+x)*4;
            pixels[i]=BYTE((x+tick*6)%256); pixels[i+1]=BYTE(y%256); pixels[i+2]=BYTE(tick%256); pixels[i+3]=255;
        }
        writer.push(pixels); ++tick; return 0;
    }
    case WM_DESTROY:
        KillTimer(hwnd, 1); writer.stop();
        if (camera) camera->Shutdown(); camera.Reset(); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, message, w, l);
}
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR args, int show) {
    if (wcscmp(args, L"--remove-camera") != 0 && elevated()) {
        MessageBoxW(nullptr, L"Run Kagami as your desktop user, not as administrator.", L"Kagami", MB_ICONERROR); return 2;
    }
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return 2;
    hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) { CoUninitialize(); return 2; }
    if (wcscmp(args, L"--remove-camera") == 0) {
        hr=openCamera(); if (SUCCEEDED(hr)) hr=camera->Remove(); camera.Reset();
        MFShutdown(); CoUninitialize(); return FAILED(hr) ? 2 : 0;
    }
    WNDCLASSW cls{}; cls.lpfnWndProc=windowProc; cls.hInstance=instance; cls.lpszClassName=L"io.kagami.Host";
    cls.hCursor=LoadCursorW(nullptr, IDC_ARROW); cls.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);
    cls.hIcon=LoadIconW(instance, MAKEINTRESOURCEW(1)); RegisterClassW(&cls);
    HWND hwnd=CreateWindowW(cls.lpszClassName,L"Kagami",WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,CW_USEDEFAULT,680,340,nullptr,nullptr,instance,nullptr);
    if (!hwnd) { MFShutdown(); CoUninitialize(); return 2; }
    ShowWindow(hwnd,show); MSG msg{};
    while (GetMessageW(&msg,nullptr,0,0)>0) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    MFShutdown(); CoUninitialize(); return static_cast<int>(msg.wParam);
}
