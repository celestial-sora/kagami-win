#pragma once
#include "DialogStyle.h"
namespace outputControls {
inline HWND window{},dimensions{},rate{},widthField{},heightField{};inline HFONT font{};inline HWND host{};
inline bool updatingFields=false;
struct Size {unsigned w,h;const wchar_t* label;};
inline const Size sizes[]{{1920,1080,L"1920 × 1080 (16:9)"},{1280,720,L"1280 × 720 (16:9)"},{960,540,L"960 × 540 (16:9)"},{640,480,L"640 × 480 (4:3)"},{1080,1920,L"1080 × 1920 (portrait)"},{720,1280,L"720 × 1280 (portrait)"},{1080,1080,L"1080 × 1080 (square)"}};
inline const unsigned rates[]{15,24,30,60};
inline void setDimensions(unsigned width,unsigned height){
    updatingFields=true;
    SetDlgItemInt(window,403,width,FALSE);SetDlgItemInt(window,404,height,FALSE);
    updatingFields=false;
}
inline bool readDimension(HWND field,unsigned& value){
    wchar_t text[8]{};const int length=GetWindowTextW(field,text,_countof(text));
    if(length<1||length>4)return false;
    value=0;
    for(int i=0;i<length;++i){if(text[i]<L'0'||text[i]>L'9')return false;value=value*10+unsigned(text[i]-L'0');}
    return true;
}
inline LRESULT CALLBACK proc(HWND hwnd,UINT message,WPARAM w,LPARAM l) {
    LRESULT styled{};if(dialogStyle::message(message,w,l,styled))return styled;
    if(message==WM_COMMAND&&LOWORD(w)==IDOK){SendMessageW(hwnd,WM_COMMAND,401,0);return 0;}
    if(message==WM_COMMAND&&LOWORD(w)==IDCANCEL){DestroyWindow(hwnd);return 0;}
    if(message==WM_COMMAND&&LOWORD(w)==405&&HIWORD(w)==CBN_SELCHANGE){
        const auto index=SendMessageW(dimensions,CB_GETCURSEL,0,0);
        if(index>=0&&index<_countof(sizes))setDimensions(sizes[index].w,sizes[index].h);
        return 0;
    }
    if(message==WM_COMMAND&&(LOWORD(w)==403||LOWORD(w)==404)&&HIWORD(w)==EN_CHANGE&&!updatingFields){
        SendMessageW(dimensions,CB_SETCURSEL,_countof(sizes),0);return 0;
    }
    if(message==WM_COMMAND&&LOWORD(w)==401){
        if(ui::enabled||ui::running){MessageBoxW(hwnd,L"Remove the Kagami camera first, then change its output format and turn Camera On again.",L"Camera format",MB_ICONINFORMATION);return 0;}
        const auto fps=SendMessageW(rate,CB_GETCURSEL,0,0);kagami::OutputFormat next;
        if(!readDimension(widthField,next.width)||!readDimension(heightField,next.height)||fps<0||fps>=_countof(rates)){
            MessageBoxW(hwnd,L"Enter whole-number pixel dimensions and select a frame rate.",L"Camera format",MB_ICONWARNING);return 0;
        }
        next.fps=rates[fps];
        if(!next.valid()){
            MessageBoxW(hwnd,L"Use even dimensions: width 320–1920, height 240–1920, with no more than 2,073,600 pixels (1920 × 1080).",L"Camera format",MB_ICONWARNING);return 0;
        }
        ui::format=next;framingControls::save();PostMessageW(host,WM_COMMAND,114,0);DestroyWindow(hwnd);return 0;
    }
    if(message==WM_CLOSE||(message==WM_COMMAND&&LOWORD(w)==402)){DestroyWindow(hwnd);return 0;}
    if(message==WM_DESTROY){window=nullptr;if(font){DeleteObject(font);font=nullptr;}return 0;}
    return DefWindowProcW(hwnd,message,w,l);
}
inline void show(HWND parent){
    if(window){SetForegroundWindow(window);return;}host=parent;
    WNDCLASSW cls{};cls.lpfnWndProc=proc;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"Kagami.Output";cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.hbrBackground=dialogStyle::background();RegisterClassW(&cls);
    window=CreateWindowW(cls.lpszClassName,L"Kagami · Camera output",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,550,420,parent,nullptr,cls.hInstance,nullptr);
    if(!window)return;
    dialogStyle::title(window);
    float dpi=GetDpiForWindow(window)/96.f;font=CreateFontW(int(-15*dpi),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    auto control=[&](const wchar_t* kind,const wchar_t* label,DWORD style,int x,int y,int width,int height,int id=0){auto child=CreateWindowW(kind,label,WS_CHILD|WS_VISIBLE|style,int(x*dpi),int(y*dpi),int(width*dpi),int(height*dpi),window,reinterpret_cast<HMENU>(INT_PTR(id)),cls.hInstance,nullptr);SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);if(wcscmp(kind,L"BUTTON")==0)dialogStyle::button(child);return child;};
    control(L"STATIC",L"Remove camera before changing its format. Enable it again and reopen your camera app afterwards. Frame rate is a target; reception depends on the phone and PC.",0,20,16,500,76);
    control(L"STATIC",L"Preset",0,20,108,100,30);dimensions=control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,136,105,360,240,405);
    int selected=_countof(sizes);
    for(unsigned i=0;i<_countof(sizes);++i){SendMessageW(dimensions,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(sizes[i].label));if(sizes[i].w==ui::format.width&&sizes[i].h==ui::format.height)selected=i;}
    SendMessageW(dimensions,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Custom dimensions"));SendMessageW(dimensions,CB_SETCURSEL,selected,0);
    control(L"STATIC",L"Width",0,20,156,100,30);widthField=control(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_NUMBER|ES_AUTOHSCROLL,136,153,130,30,403);
    control(L"STATIC",L"Height",0,282,156,74,30);heightField=control(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_NUMBER|ES_AUTOHSCROLL,366,153,130,30,404);
    SendMessageW(widthField,EM_SETLIMITTEXT,4,0);SendMessageW(heightField,EM_SETLIMITTEXT,4,0);setDimensions(ui::format.width,ui::format.height);
    control(L"STATIC",L"Even dimensions, up to 1920 per side and 2,073,600 pixels total.",0,20,194,476,44);
    control(L"STATIC",L"Frame rate",0,20,250,100,30);rate=control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,136,247,360,150);
    for(unsigned i=0;i<_countof(rates);++i){wchar_t label[24];swprintf_s(label,L"%u FPS",rates[i]);SendMessageW(rate,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label));if(rates[i]==ui::format.fps)SendMessageW(rate,CB_SETCURSEL,i,0);}
    control(L"BUTTON",L"Cancel",WS_TABSTOP,252,316,110,36,402);control(L"BUTTON",L"Apply",WS_TABSTOP|BS_DEFPUSHBUTTON,378,316,118,36,401);
    SetWindowPos(window,nullptr,0,0,int(550*dpi),int(420*dpi),SWP_NOMOVE|SWP_NOZORDER);ShowWindow(window,SW_SHOW);
}
}

