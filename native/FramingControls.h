#pragma once
#include "Framing.h"
namespace framingControls {
inline kagami::Framing value;
inline HWND window{},fields[4]{},rotation{},fit{},aspect{},mirror{};
inline HFONT font{};
inline void save() {
    HKEY key{};
    if(RegCreateKeyExW(HKEY_CURRENT_USER,L"Software\\Kagami\\Desktop",0,nullptr,0,KEY_SET_VALUE,nullptr,&key,nullptr)!=ERROR_SUCCESS)return;
    const double crop[]{value.x,value.y,value.w,value.h};
    RegSetValueExW(key,L"Crop",0,REG_BINARY,reinterpret_cast<const BYTE*>(crop),sizeof(crop));
    const wchar_t* names[]{L"Rotation",L"Mirror",L"Fill",L"Guide",L"SafeArea"};
    DWORD values[]{value.rotation,value.mirror,value.fill,ui::guide,ui::safe};
    for(int i=0;i<5;++i)RegSetValueExW(key,names[i],0,REG_DWORD,reinterpret_cast<BYTE*>(&values[i]),sizeof(DWORD));
    RegCloseKey(key);
}
inline void load() {
    HKEY key{};if(RegOpenKeyExW(HKEY_CURRENT_USER,L"Software\\Kagami\\Desktop",0,KEY_QUERY_VALUE,&key)!=ERROR_SUCCESS)return;
    double crop[4]{};DWORD size=sizeof(crop);
    if(RegGetValueW(key,nullptr,L"Crop",RRF_RT_REG_BINARY,nullptr,crop,&size)==ERROR_SUCCESS&&size==sizeof(crop)) {value.x=crop[0];value.y=crop[1];value.w=crop[2];value.h=crop[3];}
    const wchar_t* names[]{L"Rotation",L"Mirror",L"Fill",L"Guide",L"SafeArea"};DWORD values[5]{};
    for(int i=0;i<5;++i){size=sizeof(DWORD);RegGetValueW(key,nullptr,names[i],RRF_RT_REG_DWORD,nullptr,&values[i],&size);}
    value.rotation=values[0];value.mirror=values[1]!=0;value.fill=values[2]!=0;ui::guide=values[3]!=0;ui::safe=values[4]!=0;
    RegCloseKey(key);if(!value.valid())value={};
}
inline void populate() {
    double values[]{value.x,value.y,value.w,value.h};
    for(int i=0;i<4;++i){wchar_t number[40];swprintf_s(number,L"%.4f",values[i]);SetWindowTextW(fields[i],number);}
    SendMessageW(rotation,CB_SETCURSEL,value.rotation,0);SendMessageW(fit,CB_SETCURSEL,value.fill?1:0,0);SendMessageW(mirror,BM_SETCHECK,value.mirror?BST_CHECKED:BST_UNCHECKED,0);
}
inline LRESULT CALLBACK proc(HWND hwnd,UINT msg,WPARAM w,LPARAM l) {
    switch(msg) {
    case WM_COMMAND:
        if(LOWORD(w)==200) {
            kagami::Framing next=value;double* numbers[]{&next.x,&next.y,&next.w,&next.h};
            for(int i=0;i<4;++i){wchar_t number[40];GetWindowTextW(fields[i],number,40);wchar_t* end{};*numbers[i]=wcstod(number,&end);if(end==number||*end){MessageBoxW(hwnd,L"Use a decimal value for each crop coordinate.",L"Framing",MB_ICONWARNING);return 0;}}
            next.rotation=unsigned(SendMessageW(rotation,CB_GETCURSEL,0,0));next.fill=SendMessageW(fit,CB_GETCURSEL,0,0)==1;next.mirror=SendMessageW(mirror,BM_GETCHECK,0,0)==BST_CHECKED;
            if(!next.valid()){MessageBoxW(hwnd,L"Crop coordinates must remain inside 0–1, with positive width and height.",L"Framing",MB_ICONWARNING);return 0;}
            value=next;save();DestroyWindow(hwnd);return 0;
        }
        if(LOWORD(w)==201) {auto original=value;value={};populate();value=original;return 0;}
        if(LOWORD(w)==202) {DestroyWindow(hwnd);return 0;}
        if(LOWORD(w)==203&&HIWORD(w)==CBN_SELCHANGE) {
            int selected=int(SendMessageW(aspect,CB_GETCURSEL,0,0));
            double ratios[]{0,16.0/9,4.0/3,1,9.0/16};
            if(selected>0&&selected<5){kagami::Framing preset=value;kagami::centeredAspect(preset,ratios[selected]);double numbers[]{preset.x,preset.y,preset.w,preset.h};for(int i=0;i<4;++i){wchar_t number[40];swprintf_s(number,L"%.4f",numbers[i]);SetWindowTextW(fields[i],number);}}return 0;
        }
        break;
    case WM_CLOSE: DestroyWindow(hwnd);return 0;
    case WM_DESTROY: window=nullptr; if(font){DeleteObject(font);font=nullptr;}return 0;
    }
    return DefWindowProcW(hwnd,msg,w,l);
}
inline void show(HWND parent) {
    if(window){SetForegroundWindow(window);return;}
    WNDCLASSW cls{};cls.lpfnWndProc=proc;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"Kagami.Framing";cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);RegisterClassW(&cls);
    window=CreateWindowW(cls.lpszClassName,L"Kagami · Framing controls",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,520,450,parent,nullptr,cls.hInstance,nullptr);
    if(!window)return;
    float dpi=GetDpiForWindow(window)/96.f;
    font=CreateFontW(int(-15*dpi),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    auto control=[&](const wchar_t* kind,const wchar_t* label,DWORD style,int x,int y,int width,int height,int id=0){HWND child=CreateWindowW(kind,label,WS_CHILD|WS_VISIBLE|style,int(x*dpi),int(y*dpi),int(width*dpi),int(height*dpi),window,reinterpret_cast<HMENU>(INT_PTR(id)),cls.hInstance,nullptr);SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);return child;};
    control(L"STATIC",L"Crop the source, then rotate / mirror and fit the camera.",0,20,16,460,44);
    const wchar_t* labels[]{L"Left (0–1)",L"Top (0–1)",L"Width (0–1)",L"Height (0–1)"};
    for(int i=0;i<4;++i){int x=20+(i%2)*235,y=64+(i/2)*42;control(L"STATIC",labels[i],0,x,y,115,26);fields[i]=control(L"EDIT",L"",WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,x+119,y,95,28);}
    control(L"STATIC",L"Aspect crop",0,20,158,115,26);aspect=control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,139,155,320,180,203);
    const wchar_t* aspects[]{L"Free crop",L"16:9",L"4:3",L"1:1",L"Portrait 9:16"};for(auto label:aspects)SendMessageW(aspect,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label));SendMessageW(aspect,CB_SETCURSEL,0,0);
    control(L"STATIC",L"Rotation",0,20,203,115,26);rotation=control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,139,200,320,160);
    const wchar_t* rotations[]{L"0°",L"90° clockwise",L"180°",L"270° clockwise"};for(auto label:rotations)SendMessageW(rotation,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label));
    control(L"STATIC",L"Scale",0,20,248,115,26);fit=control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,139,245,320,120);SendMessageW(fit,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Fit (letterbox)"));SendMessageW(fit,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Fill (center crop)"));
    mirror=control(L"BUTTON",L"Mirror preview and output",WS_TABSTOP|BS_AUTOCHECKBOX,20,292,440,28);
    control(L"BUTTON",L"Reset",WS_TABSTOP,20,348,110,36,201);control(L"BUTTON",L"Cancel",WS_TABSTOP,222,348,110,36,202);control(L"BUTTON",L"Apply",WS_TABSTOP|BS_DEFPUSHBUTTON,344,348,115,36,200);
    SetWindowPos(window,nullptr,0,0,int(520*dpi),int(450*dpi),SWP_NOMOVE|SWP_NOZORDER);populate();ShowWindow(window,SW_SHOW);
}
}
