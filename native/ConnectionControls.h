#pragma once
#include "AdbCommand.h"
#include "DialogStyle.h"
#include <memory>
#include <thread>
#include <mutex>
namespace connectionControls {
inline HWND window{},serial{},endpoint{},code{},result{},startButton{},commands[3]{};
inline HFONT font{};inline HWND host{};inline std::wstring selected;
inline std::thread worker;inline std::atomic<bool> cancelled{true};inline std::mutex lock;inline std::wstring response;
inline LRESULT CALLBACK proc(HWND hwnd,UINT msg,WPARAM w,LPARAM l) {
    LRESULT styled{};if(dialogStyle::message(msg,w,l,styled))return styled;
    if(msg==WM_COMMAND){
        auto id=LOWORD(w);if(id==IDOK)id=301;else if(id==IDCANCEL)id=302;
        if(id==301){if(!IsWindowEnabled(startButton))return 0;wchar_t value[200]{};GetWindowTextW(serial,value,200);if(!kagami::validAdbTarget(value)){SetWindowTextW(result,L"Enter an authorized serial or connected debugging IP:port.");return 0;}selected=value;PostMessageW(host,WM_COMMAND,109,0);DestroyWindow(hwnd);return 0;}
        if(id==302){DestroyWindow(hwnd);return 0;}
        if(id>=303&&id<=305){
            if(worker.joinable())worker.join();wchar_t address[200]{};GetWindowTextW(endpoint,address,200);
            wchar_t pin[16]{};GetWindowTextW(code,pin,16);SetWindowTextW(code,L"");std::string secret;kagami::SecretClear clearSecret{secret};
            if(id==304){for(auto p=pin;*p;++p){if(*p<L'0'||*p>L'9'){SetWindowTextW(result,L"Pairing code must contain six digits.");SecureZeroMemory(pin,sizeof(pin));return 0;}secret+=char(*p);}if(secret.size()!=6){SetWindowTextW(result,L"Enter the six-digit pairing code.");SecureZeroMemory(pin,sizeof(pin));return 0;}}
            SecureZeroMemory(pin,sizeof(pin));
            if(id!=303&&!kagami::validAdbEndpoint(address)){SetWindowTextW(result,L"Use a private IP:port shown in Android Wireless debugging.");return 0;}cancelled=false;
            for(auto button:commands)EnableWindow(button,FALSE);EnableWindow(startButton,FALSE);SetWindowTextW(result,L"Working… Cancel closes this operation.");
            try{
                auto secretHolder=std::shared_ptr<std::string>(new std::string(secret),[](std::string* value){if(!value->empty())SecureZeroMemory(value->data(),value->size());delete value;});
                worker=std::thread([hwnd,id,address=std::wstring(address),secretHolder]{
                std::wstring text;try{text=kagami::runAdb(id==303?L"devices":id==304?L"pair":L"connect",address,*secretHolder,cancelled);}catch(winrt::hresult_error const& e){text=e.message();}catch(...){text=L"ADB operation failed.";}
                {std::lock_guard guard(lock);response=std::move(text);}if(!cancelled)PostMessageW(hwnd,WM_APP+1,0,0);
            });}catch(...){cancelled=true;for(auto button:commands)EnableWindow(button,TRUE);EnableWindow(startButton,TRUE);SetWindowTextW(result,L"Cannot create an ADB worker. Please try again.");}return 0;
        }
    }
    if(msg==WM_APP+1){if(worker.joinable())worker.join();std::lock_guard guard(lock);SetWindowTextW(result,response.c_str());response.clear();for(auto button:commands)EnableWindow(button,TRUE);EnableWindow(startButton,TRUE);return 0;}
    if(msg==WM_CLOSE){DestroyWindow(hwnd);return 0;}
    if(msg==WM_DESTROY){cancelled=true;if(worker.joinable())worker.join();SetWindowTextW(code,L"");window=nullptr;{std::lock_guard guard(lock);response.clear();}if(font){DeleteObject(font);font=nullptr;}return 0;}
    return DefWindowProcW(hwnd,msg,w,l);
}
inline void show(HWND parent) {
    if(window){SetForegroundWindow(window);return;}host=parent;
    WNDCLASSW cls{};cls.lpfnWndProc=proc;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"Kagami.Android";cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.hbrBackground=dialogStyle::background();RegisterClassW(&cls);
    window=CreateWindowW(cls.lpszClassName,L"Kagami · USB / ADB Wi-Fi",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,650,620,parent,nullptr,cls.hInstance,nullptr);
    if(!window)return;dialogStyle::title(window);
    float dpi=GetDpiForWindow(window)/96.f;font=CreateFontW(int(-15*dpi),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    auto control=[&](const wchar_t* type,const wchar_t* title,DWORD style,int x,int y,int width,int height,int id=0){auto child=CreateWindowW(type,title,WS_CHILD|WS_VISIBLE|style,int(x*dpi),int(y*dpi),int(width*dpi),int(height*dpi),window,reinterpret_cast<HMENU>(INT_PTR(id)),cls.hInstance,nullptr);SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);if(wcscmp(type,L"BUTTON")==0)dialogStyle::button(child);return child;};
    control(L"STATIC",L"USB: enable USB debugging and accept authorization on the phone. Wi-Fi: enable Wireless debugging, Pair using its pairing port/code, then Connect using its separate debugging port. ADB manages authorization; Kagami saves no addresses or codes.",0,20,16,600,100);
    commands[0]=control(L"BUTTON",L"Refresh ADB devices",WS_TABSTOP,20,124,190,34,303);
    control(L"STATIC",L"Pair / connect IP:port",0,20,176,185,28);endpoint=control(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,210,173,390,30);
    control(L"STATIC",L"Pairing code",0,20,218,185,28);code=control(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_PASSWORD|ES_AUTOHSCROLL,210,215,150,30);SendMessageW(code,EM_SETLIMITTEXT,6,0);
    commands[1]=control(L"BUTTON",L"Pair",WS_TABSTOP,378,214,98,34,304);commands[2]=control(L"BUTTON",L"Connect",WS_TABSTOP,490,214,110,34,305);
    result=control(L"EDIT",L"Refresh devices, then enter an authorized device below.",WS_BORDER|ES_MULTILINE|ES_READONLY|WS_VSCROLL,20,264,580,170);
    control(L"STATIC",L"Device serial / connected IP:port",0,20,451,580,28);serial=control(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,20,480,580,30);
    control(L"BUTTON",L"Cancel",WS_TABSTOP,352,528,110,35,302);startButton=control(L"BUTTON",L"Start Android",WS_TABSTOP|BS_DEFPUSHBUTTON,474,528,126,35,301);
    SendMessageW(serial,EM_SETLIMITTEXT,180,0);SendMessageW(endpoint,EM_SETLIMITTEXT,180,0);
    SetWindowPos(window,nullptr,0,0,int(650*dpi),int(620*dpi),SWP_NOMOVE|SWP_NOZORDER);ShowWindow(window,SW_SHOW);SetFocus(serial);
}
}
