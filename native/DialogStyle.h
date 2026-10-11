#pragma once
// Standard native control rendering, with the same dark palette as the desktop.
namespace dialogStyle {
inline HBRUSH background(){static HBRUSH brush=CreateSolidBrush(RGB(29,27,39));return brush;}
inline HBRUSH field(){static HBRUSH brush=CreateSolidBrush(RGB(48,45,60));return brush;}
inline void title(HWND hwnd){auto module=LoadLibraryW(L"dwmapi.dll");if(module){using SetAttribute=HRESULT(WINAPI*)(HWND,DWORD,LPCVOID,DWORD);auto set=reinterpret_cast<SetAttribute>(GetProcAddress(module,"DwmSetWindowAttribute"));BOOL dark=TRUE;if(set)set(hwnd,20,&dark,sizeof(dark));FreeLibrary(module);}}
inline void button(HWND hwnd){auto style=GetWindowLongPtrW(hwnd,GWL_STYLE);auto type=style&BS_TYPEMASK;if(type==BS_PUSHBUTTON||type==BS_DEFPUSHBUTTON)SetWindowLongPtrW(hwnd,GWL_STYLE,(style&~BS_TYPEMASK)|BS_OWNERDRAW);}
inline bool message(UINT msg,WPARAM w,LPARAM l,LRESULT& result){
    if(msg==WM_CTLCOLORSTATIC||msg==WM_CTLCOLOREDIT||msg==WM_CTLCOLORLISTBOX||msg==WM_CTLCOLORBTN){auto dc=reinterpret_cast<HDC>(w);SetTextColor(dc,RGB(237,225,247));wchar_t kind[16]{};GetClassNameW(reinterpret_cast<HWND>(l),kind,16);bool input=msg==WM_CTLCOLOREDIT||msg==WM_CTLCOLORLISTBOX||wcscmp(kind,L"Edit")==0;SetBkColor(dc,input?RGB(48,45,60):RGB(29,27,39));result=reinterpret_cast<LRESULT>(input?field():background());return true;}
    if(msg==WM_ERASEBKGND){RECT area{};GetClipBox(reinterpret_cast<HDC>(w),&area);FillRect(reinterpret_cast<HDC>(w),&area,background());result=1;return true;}
    if(msg==WM_DRAWITEM){auto draw=reinterpret_cast<DRAWITEMSTRUCT*>(l);if(draw->CtlType!=ODT_BUTTON)return false;
        auto brush=CreateSolidBrush((draw->itemState&ODS_SELECTED)?RGB(109,59,157):RGB(63,53,81));FillRect(draw->hDC,&draw->rcItem,brush);DeleteObject(brush);
        SetBkMode(draw->hDC,TRANSPARENT);SetTextColor(draw->hDC,(draw->itemState&ODS_DISABLED)?RGB(133,121,145):RGB(242,229,255));
        wchar_t text[128]{};GetWindowTextW(draw->hwndItem,text,128);auto font=reinterpret_cast<HFONT>(SendMessageW(draw->hwndItem,WM_GETFONT,0,0));auto old=font?SelectObject(draw->hDC,font):nullptr;
        RECT rect=draw->rcItem;DrawTextW(draw->hDC,text,-1,&rect,DT_CENTER|DT_VCENTER|DT_SINGLELINE);if(old)SelectObject(draw->hDC,old);
        if(draw->itemState&ODS_FOCUS){InflateRect(&rect,-3,-3);DrawFocusRect(draw->hDC,&rect);}result=TRUE;return true;
    }
    return false;
}
}
