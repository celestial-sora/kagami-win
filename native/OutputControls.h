#pragma once
namespace outputControls {
inline HWND window{},dimensions{},rate{};inline HFONT font{};inline HWND host{};
struct Size {unsigned w,h;const wchar_t* label;};
inline const Size sizes[]{{1920,1080,L"1920 × 1080 (16:9)"},{1280,720,L"1280 × 720 (16:9)"},{960,540,L"960 × 540 (16:9)"},{640,480,L"640 × 480 (4:3)"},{1080,1920,L"1080 × 1920 (portrait)"},{720,1280,L"720 × 1280 (portrait)"},{1080,1080,L"1080 × 1080 (square)"}};
inline const unsigned rates[]{15,24,30,60};
inline LRESULT CALLBACK proc(HWND hwnd,UINT message,WPARAM w,LPARAM l) {
    if(message==WM_COMMAND&&LOWORD(w)==401){
        if(ui::enabled||ui::running){MessageBoxW(hwnd,L"Remove the Kagami camera first, then change its output format and Enable camera again.",L"Camera format",MB_ICONINFORMATION);return 0;}
        auto size=SendMessageW(dimensions,CB_GETCURSEL,0,0),fps=SendMessageW(rate,CB_GETCURSEL,0,0);
        if(size>=0&&size<_countof(sizes)&&fps>=0&&fps<_countof(rates)){ui::format={sizes[size].w,sizes[size].h,rates[fps]};framingControls::save();PostMessageW(host,WM_COMMAND,114,0);DestroyWindow(hwnd);}return 0;
    }
    if(message==WM_CLOSE||(message==WM_COMMAND&&LOWORD(w)==402)){DestroyWindow(hwnd);return 0;}
    if(message==WM_DESTROY){window=nullptr;if(font){DeleteObject(font);font=nullptr;}return 0;}
    return DefWindowProcW(hwnd,message,w,l);
}
inline void show(HWND parent){
    if(window){SetForegroundWindow(window);return;}host=parent;
    WNDCLASSW cls{};cls.lpfnWndProc=proc;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"Kagami.Output";cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);RegisterClassW(&cls);
    window=CreateWindowW(cls.lpszClassName,L"Kagami · Camera output",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,550,320,parent,nullptr,cls.hInstance,nullptr);
    float dpi=GetDpiForWindow(window)/96.f;font=CreateFontW(int(-15*dpi),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    auto control=[&](const wchar_t* kind,const wchar_t* label,DWORD style,int x,int y,int width,int height,int id=0){auto child=CreateWindowW(kind,label,WS_CHILD|WS_VISIBLE|style,int(x*dpi),int(y*dpi),int(width*dpi),int(height*dpi),window,reinterpret_cast<HMENU>(INT_PTR(id)),cls.hInstance,nullptr);SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);return child;};
    control(L"STATIC",L"Remove camera before changing its format. Enable it again and reopen your camera app afterwards. Frame rate is a target; reception depends on the phone and PC.",0,20,16,500,76);
    control(L"STATIC",L"Resolution",0,20,108,100,30);dimensions=control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,136,105,360,240);
    for(unsigned i=0;i<_countof(sizes);++i){SendMessageW(dimensions,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(sizes[i].label));if(sizes[i].w==ui::format.width&&sizes[i].h==ui::format.height)SendMessageW(dimensions,CB_SETCURSEL,i,0);}
    control(L"STATIC",L"Frame rate",0,20,156,100,30);rate=control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,136,153,360,150);
    for(unsigned i=0;i<_countof(rates);++i){wchar_t label[24];swprintf_s(label,L"%u FPS",rates[i]);SendMessageW(rate,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label));if(rates[i]==ui::format.fps)SendMessageW(rate,CB_SETCURSEL,i,0);}
    control(L"BUTTON",L"Cancel",WS_TABSTOP,252,226,110,36,402);control(L"BUTTON",L"Apply",WS_TABSTOP|BS_DEFPUSHBUTTON,378,226,118,36,401);
    SetWindowPos(window,nullptr,0,0,int(550*dpi),int(320*dpi),SWP_NOMOVE|SWP_NOZORDER);ShowWindow(window,SW_SHOW);
}
}
