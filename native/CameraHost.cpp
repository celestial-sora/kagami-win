#include <windows.h>
#include <mfapi.h>
#include <mfvirtualcamera.h>
#include <wrl/client.h>
#include <shellapi.h>
#include <roapi.h>
#include <string>
#include "FrameBridge.h"
#include "DesktopLaunch.h"
#include "DesktopUI.h"
#include "FramingControls.h"
#include "AndroidReceiver.h"
#include "ConnectionControls.h"
#include "AirPlayReceiver.h"
#include "SmartViewReceiver.h"
#include "OutputControls.h"
#include "source/VirtualCameraMediaSource.h"
using Microsoft::WRL::ComPtr;
static ComPtr<IMFVirtualCamera> camera;
static kagami::FrameWriter writer;
static kagami::AndroidReceiver android;
static kagami::AirPlayReceiver airplay;
static kagami::SmartViewReceiver smartview;
static HWND host;
static void setStatus(const wchar_t* value) { ui::message = value; if(host) ui::refresh(host); }
static unsigned tick;
static bool dragging=false;
static Gdiplus::PointF dragStart;
static void stopMedia(HWND hwnd) {
    KillTimer(hwnd,1);writer.stop();ui::running=false;ui::android=false;ui::airplay=false;ui::smartview=false;ui::preview.clear();ui::source.clear();android.stop();airplay.stop();smartview.stop();
}
static void syncCrop() {
    auto& f=framingControls::value;
    ui::cropRect=Gdiplus::RectF(float(f.x),float(f.y),float(f.w),float(f.h));
    ui::mirrored=f.mirror;
    if(ui::buttons[9])SetWindowTextW(ui::buttons[9],f.mirror?L"Mirror output: On":L"Mirror output: Off");
}
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
    if (SUCCEEDED(hr)) setStatus(success);
    else { wchar_t message[300]; swprintf_s(message, L"Operation failed (0x%08X). Install Kagami Setup, and check Windows camera privacy settings.", static_cast<unsigned>(hr)); setStatus(message); }
}
static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM w, LPARAM l) {
    switch (message) {
    case WM_CREATE: {
        host=hwnd; framingControls::load();
        const wchar_t* labels[]{L"Enable camera", L"Test pattern", L"Stop", L"Remove camera", L"Framing controls", L"Framing guide: Off", L"Safe area: Off",L"Show virtual camera output",L"USB / ADB Wi-Fi setup",L"Mirror output: Off",L"Start AirPlay",L"Start Smart View",L"Change format"};
        for (int i=0;i<13;++i) ui::buttons[i]=CreateWindowW(L"BUTTON",labels[i],WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,0,0,0,0,hwnd,reinterpret_cast<HMENU>(INT_PTR(i>=9?101+i:100+i)),nullptr,nullptr);
        syncCrop();
        SetWindowTextW(ui::buttons[5],ui::guide?L"Framing guide: On":L"Framing guide: Off");
        SetWindowTextW(ui::buttons[6],ui::safe?L"Safe area: On":L"Safe area: Off");
        ui::refresh(hwnd); return 0;
    }
    case WM_SIZE: ui::refresh(hwnd); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_DRAWITEM: ui::drawButton(reinterpret_cast<DRAWITEMSTRUCT*>(l)); return TRUE;
    case WM_PAINT: { syncCrop(); PAINTSTRUCT paint; HDC dc=BeginPaint(hwnd,&paint); ui::paint(hwnd,dc); EndPaint(hwnd,&paint); return 0; }
    case WM_LBUTTONDOWN: {
        Gdiplus::PointF point(float(short(LOWORD(l)))/ui::scale,float(short(HIWORD(l)))/ui::scale);
        if(ui::running&&!ui::output&&ui::imageRect.Contains(point)) {dragging=true;dragStart=point;SetCapture(hwnd);} return 0;
    }
    case WM_LBUTTONUP:
        if(dragging) {
            dragging=false;ReleaseCapture();
            float x=float(short(LOWORD(l)))/ui::scale,y=float(short(HIWORD(l)))/ui::scale;
            auto r=ui::imageRect;
            x=(std::clamp)(x,r.X,r.GetRight()); y=(std::clamp)(y,r.Y,r.GetBottom());
            if(std::abs(x-dragStart.X)>4&&std::abs(y-dragStart.Y)>4) {
                auto& f=framingControls::value;
                f.x=((std::min)(x,dragStart.X)-r.X)/r.Width; f.y=((std::min)(y,dragStart.Y)-r.Y)/r.Height;
                f.w=std::abs(x-dragStart.X)/r.Width; f.h=std::abs(y-dragStart.Y)/r.Height;
                framingControls::save();InvalidateRect(hwnd,nullptr,FALSE);
            }
        }return 0;
    case WM_CAPTURECHANGED: dragging=false;return 0;
    case WM_GETMINMAXINFO: {
        auto info=reinterpret_cast<MINMAXINFO*>(l);
        float dpi=GetDpiForWindow(hwnd)/96.f;
        info->ptMinTrackSize={LONG(1100*dpi),LONG(900*dpi)}; return 0;
    }
    case WM_DPICHANGED: {
        auto bounds=reinterpret_cast<RECT*>(l);
        SetWindowPos(hwnd,nullptr,bounds->left,bounds->top,bounds->right-bounds->left,bounds->bottom-bounds->top,SWP_NOZORDER|SWP_NOACTIVATE);
        ui::refresh(hwnd); return 0;
    }
    case WM_COMMAND:
        if (LOWORD(w) == 100) {
            HRESULT hr = openCamera();
            if (SUCCEEDED(hr)) hr = camera->SetString(KAGAMI_PIPE_NAME, kagami::pipeName().c_str());
            if (SUCCEEDED(hr)) hr = camera->SetUINT32(KAGAMI_OUTPUT_WIDTH,ui::format.width);
            if (SUCCEEDED(hr)) hr = camera->SetUINT32(KAGAMI_OUTPUT_HEIGHT,ui::format.height);
            if (SUCCEEDED(hr)) hr = camera->SetUINT32(KAGAMI_OUTPUT_FPS,ui::format.fps);
            if (SUCCEEDED(hr)) hr = camera->Start(nullptr);
            ui::enabled = SUCCEEDED(hr); report(hr, L"Camera enabled. Waiting output is black.");
        } else if (LOWORD(w) == 101) {
            if (!ui::enabled) { setStatus(L"Enable the Kagami camera first."); return 0; }
            if (!writer.start()) { setStatus(L"Cannot own the Kagami output. Close the other Kagami producer first."); return 0; }
            tick = 0;
            if(!SetTimer(hwnd,1,(std::max)(1u,1000/ui::format.fps),nullptr)) {writer.stop();setStatus(L"Cannot start the preview timer.");return 0;}
            ui::running = true;
            setStatus(L"Synthetic test pattern running. This checks camera output, not phone compatibility.");
        } else if (LOWORD(w) == 102) {
            stopMedia(hwnd); setStatus(L"Stopped. Enabled camera returns black frames.");
        } else if (LOWORD(w) == 103) {
            stopMedia(hwnd);
            HRESULT hr = openCamera(); if (SUCCEEDED(hr)) hr = camera->Remove();
            if(SUCCEEDED(hr)) {camera.Reset();ui::enabled=false;}
            report(hr, L"Kagami camera removed for this user.");
        } else if (LOWORD(w) == 104) {
            framingControls::show(hwnd);
        } else if (LOWORD(w) == 105) {
            ui::guide=!ui::guide; SetWindowTextW(ui::buttons[5],ui::guide?L"Framing guide: On":L"Framing guide: Off"); framingControls::save();
        } else if (LOWORD(w) == 106) {
            ui::safe=!ui::safe; SetWindowTextW(ui::buttons[6],ui::safe?L"Safe area: On":L"Safe area: Off"); framingControls::save();
        } else if (LOWORD(w) == 107) {
            ui::output=!ui::output;SetWindowTextW(ui::buttons[7],ui::output?L"Show source preview":L"Show virtual camera output");
        } else if (LOWORD(w) == 108) {
            connectionControls::show(hwnd);
        } else if (LOWORD(w) == 109) {
            if(!ui::enabled||ui::running) {setStatus(L"Enable the camera and stop the current session first.");return 0;}
            try {
                if(!writer.start()){setStatus(L"Another Kagami producer owns the camera output.");return 0;}
                android.start(connectionControls::selected);
                if(!SetTimer(hwnd,1,(std::max)(1u,1000/ui::format.fps),nullptr))throw winrt::hresult_error(E_FAIL);
                ui::running=true;ui::android=true;setStatus(L"Starting the authorized Android receiver. Keep its window open.");
            } catch(const winrt::hresult_error& error) {stopMedia(hwnd);setStatus(error.message().c_str());}
            connectionControls::selected.clear();
        } else if(LOWORD(w)==110) {
            framingControls::value.mirror=!framingControls::value.mirror;framingControls::save();syncCrop();
        } else if(LOWORD(w)==111) {
            if(!ui::enabled||ui::running){setStatus(L"Enable the Kagami camera and stop the current session first.");return 0;}
            try {
                if(!writer.start()){setStatus(L"Another Kagami producer owns the camera output.");return 0;}
                airplay.start();if(!SetTimer(hwnd,1,(std::max)(1u,1000/ui::format.fps),nullptr))throw winrt::hresult_error(E_FAIL);
                ui::running=true;ui::airplay=true;setStatus(L"AirPlay starting. Select Kagami in Screen Mirroring on the same local network.");
            }catch(const winrt::hresult_error& error){stopMedia(hwnd);setStatus(error.message().c_str());}
            catch(...){stopMedia(hwnd);setStatus(L"Cannot start the AirPlay receiver.");}
        } else if(LOWORD(w)==112) {
            if(!ui::enabled||ui::running){setStatus(L"Enable the camera and stop the current session first.");return 0;}
            try {
                if(!writer.start()){setStatus(L"Another Kagami producer owns the camera output.");return 0;}
                smartview.start();if(!SetTimer(hwnd,1,(std::max)(1u,1000/ui::format.fps),nullptr))throw winrt::hresult_error(E_FAIL);
                ui::running=true;ui::smartview=true;setStatus(L"Starting Windows Smart View receiver. Keep its window open.");
            }catch(winrt::hresult_error const& error){stopMedia(hwnd);setStatus(error.message().c_str());}
            catch(...){stopMedia(hwnd);setStatus(L"Cannot start the Smart View receiver.");}
        } else if(LOWORD(w)==113) {
            outputControls::show(hwnd);
        } else if(LOWORD(w)==114) {
            setStatus(L"Output format saved. Enable the Kagami camera, then reopen your camera app.");
        }
        ui::refresh(hwnd); return 0;
    case WM_TIMER: {
        if(smartview.active()) {
            try {std::wstring message;auto pixels=smartview.take(message);
                if(pixels.empty()){writer.slate();ui::source.clear();ui::preview.clear();}
                else {ui::source=pixels;ui::preview=kagami::reframe(pixels,kagami::width,kagami::height,framingControls::value,ui::format.width,ui::format.height);writer.push(ui::preview,ui::format);}
                if(!message.empty())setStatus(message.c_str());
            }catch(winrt::hresult_error const& error){stopMedia(hwnd);setStatus(error.message().c_str());}
            catch(...){stopMedia(hwnd);setStatus(L"Smart View processing failed; output cleared.");}
            return 0;
        }
        if(airplay.active()) {
            try {
                auto pixels=airplay.take();if(pixels.empty())return 0;
                ui::source=pixels;ui::preview=kagami::reframe(pixels,kagami::width,kagami::height,framingControls::value,ui::format.width,ui::format.height);
                writer.push(ui::preview,ui::format);setStatus(L"AirPlay video received. Audio plays through Windows desktop output.");
            }catch(const winrt::hresult_error& error){stopMedia(hwnd);setStatus(error.message().c_str());}
            catch(...){stopMedia(hwnd);setStatus(L"AirPlay processing failed; output cleared.");}
            return 0;
        }
        if(android.active()) {
            try {
                auto pixels=android.take();if(pixels.empty())return 0;
                ui::source=pixels;ui::preview=kagami::reframe(pixels,kagami::width,kagami::height,framingControls::value,ui::format.width,ui::format.height);
                writer.push(ui::preview,ui::format);setStatus(L"Android receiver running. Drag the source preview to crop; use Framing controls to rotate or mirror.");
            }catch(const winrt::hresult_error& error){stopMedia(hwnd);setStatus(error.message().c_str());}
            return 0;
        }
        std::vector<BYTE> pixels(kagami::frameBytes);
        for (unsigned y=0; y<kagami::height; ++y) for (unsigned x=0; x<kagami::width; ++x) {
            size_t i=(y*kagami::width+x)*4;
            pixels[i]=BYTE((x+tick*6)%256); pixels[i+1]=BYTE(y%256); pixels[i+2]=BYTE(tick%256); pixels[i+3]=255;
        }
        ui::source=pixels; pixels = kagami::reframe(pixels,kagami::width,kagami::height,framingControls::value,ui::format.width,ui::format.height); writer.push(pixels,ui::format); ui::preview = std::move(pixels); ++tick; InvalidateRect(hwnd,nullptr,FALSE); return 0;
    }
    case WM_DESTROY:
        stopMedia(hwnd);
        if (camera) camera->Shutdown(); camera.Reset(); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, message, w, l);
}
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR args, int show) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    Gdiplus::GdiplusStartupInput graphicsInput; ULONG_PTR graphicsToken;
    if (Gdiplus::GdiplusStartup(&graphicsToken,&graphicsInput,nullptr)!=Gdiplus::Ok) return 2;
    struct GraphicsLifetime { ULONG_PTR token; ~GraphicsLifetime() { Gdiplus::GdiplusShutdown(token); } } graphicsLifetime{graphicsToken};
    if (wcscmp(args, L"--launch-desktop") == 0) {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(hr)) return 2;
        wchar_t path[32768]{};
        DWORD length = GetModuleFileNameW(nullptr, path, ARRAYSIZE(path));
        hr = length && length < ARRAYSIZE(path) ? kagami::launchOnDesktop(path, show) : E_FAIL;
        if (FAILED(hr)) MessageBoxW(nullptr,
            hr == E_ACCESSDENIED ?
            L"Kagami needs a desktop running without administrator privileges. Enable User Account Control (UAC) and restart Windows if your desktop is running as administrator." :
            L"Kagami is installed. Open Kagami from the Windows Start menu with your desktop user account.",
            L"Kagami", MB_ICONINFORMATION);
        CoUninitialize();
        return FAILED(hr) ? 2 : 0;
    }
    if (wcscmp(args, L"--remove-camera") != 0 && elevated()) {
        MessageBoxW(nullptr,
            L"Kagami needs a desktop running without administrator privileges. If UAC is disabled, enable User Account Control and restart Windows. Then open Kagami from Start without Run as administrator.",
            L"Kagami", MB_ICONERROR); return 2;
    }
    HRESULT hr = RoInitialize(RO_INIT_SINGLETHREADED);
    if (FAILED(hr)) return 2;
    hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) { RoUninitialize(); return 2; }
    if (wcscmp(args, L"--remove-camera") == 0) {
        hr=openCamera(); if (SUCCEEDED(hr)) hr=camera->Remove(); camera.Reset();
        MFShutdown(); RoUninitialize(); return FAILED(hr) ? 2 : 0;
    }
    if(kagami::launchRegisteredDesktop()){MFShutdown();RoUninitialize();return 0;}
    WNDCLASSW cls{}; cls.lpfnWndProc=windowProc; cls.hInstance=instance; cls.lpszClassName=L"io.kagami.Host";
    cls.hCursor=LoadCursorW(nullptr, IDC_ARROW); cls.hbrBackground=nullptr;
    cls.hIcon=LoadIconW(instance, MAKEINTRESOURCEW(1)); RegisterClassW(&cls);
    HWND hwnd=CreateWindowW(cls.lpszClassName,L"Kagami",WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT,CW_USEDEFAULT,1400,900,nullptr,nullptr,instance,nullptr);
    if (!hwnd) { MFShutdown(); RoUninitialize(); return 2; }
    ShowWindow(hwnd,show); MSG msg{};
    while (GetMessageW(&msg,nullptr,0,0)>0) { if (!(outputControls::window&&IsDialogMessageW(outputControls::window,&msg))&&!(connectionControls::window&&IsDialogMessageW(connectionControls::window,&msg))&&!(framingControls::window&&IsDialogMessageW(framingControls::window,&msg))&&!IsDialogMessageW(hwnd,&msg)) { TranslateMessage(&msg); DispatchMessageW(&msg); } }
    MFShutdown(); RoUninitialize(); return static_cast<int>(msg.wParam);
}
