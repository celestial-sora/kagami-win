#pragma once
namespace connectionControls {
inline HWND window{},serial{};
inline HFONT font{};
inline HWND host{};
inline std::wstring selected;
inline LRESULT CALLBACK proc(HWND hwnd,UINT msg,WPARAM w,LPARAM l) {
    if(msg==WM_COMMAND&&LOWORD(w)==301){wchar_t value[200]{};GetWindowTextW(serial,value,200);selected=value;PostMessageW(host,WM_COMMAND,109,0);DestroyWindow(hwnd);return 0;}
    if(msg==WM_COMMAND&&LOWORD(w)==302){DestroyWindow(hwnd);return 0;}
    if(msg==WM_CLOSE){DestroyWindow(hwnd);return 0;}
    if(msg==WM_DESTROY){window=nullptr;if(font){DeleteObject(font);font=nullptr;}return 0;}
    return DefWindowProcW(hwnd,msg,w,l);
}
inline void show(HWND parent) {
    if(window){SetForegroundWindow(window);return;}host=parent;
    WNDCLASSW cls{};cls.lpfnWndProc=proc;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"Kagami.Android";cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);RegisterClassW(&cls);
    window=CreateWindowW(cls.lpszClassName,L"Kagami · USB / ADB Wi-Fi",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,570,290,parent,nullptr,cls.hInstance,nullptr);
    float dpi=GetDpiForWindow(window)/96.f;font=CreateFontW(int(-15*dpi),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    auto control=[&](const wchar_t* type,const wchar_t* title,DWORD style,int x,int y,int width,int height,int id=0){auto child=CreateWindowW(type,title,WS_CHILD|WS_VISIBLE|style,int(x*dpi),int(y*dpi),int(width*dpi),int(height*dpi),window,reinterpret_cast<HMENU>(INT_PTR(id)),cls.hInstance,nullptr);SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);return child;};
    control(L"STATIC",L"Use an already authorized Android device from adb devices -l. For Wi-Fi, pair and connect with Android Wireless debugging first. Kagami does not change networks or store device identities.",0,20,16,520,85);
    control(L"STATIC",L"Device serial / debugging IP:port",0,20,108,520,28);
    serial=control(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,20,139,520,30);
    control(L"BUTTON",L"Cancel",WS_TABSTOP,292,202,110,35,302);control(L"BUTTON",L"Start Android",WS_TABSTOP|BS_DEFPUSHBUTTON,414,202,126,35,301);
    SetWindowPos(window,nullptr,0,0,int(570*dpi),int(290*dpi),SWP_NOMOVE|SWP_NOZORDER);ShowWindow(window,SW_SHOW);SetFocus(serial);
}
}
