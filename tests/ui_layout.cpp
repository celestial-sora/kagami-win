#include <windows.h>
#include "FrameBridge.h"
#include "DesktopUI.h"
#include <cassert>
int wmain() {
    Gdiplus::GdiplusStartupInput input;ULONG_PTR token{};assert(Gdiplus::GdiplusStartup(&token,&input,nullptr)==Gdiplus::Ok);
    {
        HWND window=CreateWindowW(L"STATIC",L"Kagami layout check",WS_POPUP,0,0,1400,900,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);assert(window);
        const wchar_t* labels[]{L"Enable camera",L"Test pattern",L"Stop",L"Remove camera",L"Framing controls",L"Framing guide: Off",L"Safe area: Off",L"Show virtual camera output",L"USB / ADB Wi-Fi setup",L"Mirror output: Off",L"Start AirPlay",L"Smart View",L"Change format",L"AirPlay"};
        for(int i=0;i<14;++i)ui::buttons[i]=CreateWindowW(L"BUTTON",labels[i],WS_CHILD|BS_OWNERDRAW,0,0,0,0,window,reinterpret_cast<HMENU>(INT_PTR(i==13?115:i>=9?101+i:100+i)),GetModuleHandleW(nullptr),nullptr);
        ui::buttons[14]=CreateWindowW(L"BUTTON",L"Check",WS_CHILD|BS_OWNERDRAW,0,0,0,0,window,reinterpret_cast<HMENU>(117),GetModuleHandleW(nullptr),nullptr);
        for(auto size:{SIZE{1024,768},SIZE{1920,1080},SIZE{1400,900}}){
            SetWindowPos(window,nullptr,0,0,size.cx,size.cy,SWP_NOMOVE|SWP_NOZORDER);ui::layout(window);
            for(auto button:ui::buttons){RECT bounds;GetWindowRect(button,&bounds);MapWindowPoints(nullptr,window,reinterpret_cast<POINT*>(&bounds),2);
                assert(bounds.left>=0&&bounds.top>=0&&bounds.right<=size.cx&&bounds.bottom<=size.cy&&bounds.right>bounds.left&&bounds.bottom>bounds.top);
            }
        }
        HDC dc=GetDC(window),memory=CreateCompatibleDC(dc);HBITMAP bitmap=CreateCompatibleBitmap(dc,1400,900);auto old=SelectObject(memory,bitmap);
        ui::scale=1;ui::paint(window,memory);assert(ui::previewRect.Width>0&&ui::previewRect.Height>0);
        for(auto button:ui::buttons){RECT r;GetWindowRect(button,&r);MapWindowPoints(nullptr,window,reinterpret_cast<POINT*>(&r),2);assert(r.left>=0&&r.top>=0&&r.right<=1400&&r.bottom<=900);
            int saved=SaveDC(memory);SetViewportOrgEx(memory,r.left,r.top,nullptr);DRAWITEMSTRUCT item{};item.CtlID=GetDlgCtrlID(button);item.hwndItem=button;item.hDC=memory;item.rcItem={0,0,r.right-r.left,r.bottom-r.top};if(!IsWindowEnabled(button))item.itemState=ODS_DISABLED;ui::drawButton(&item);RestoreDC(memory,saved);
        }
        {
            Gdiplus::Bitmap picture(bitmap,nullptr);UINT count{},size{};Gdiplus::GetImageEncodersSize(&count,&size);
            std::vector<BYTE> encoders(size);auto codecs=reinterpret_cast<Gdiplus::ImageCodecInfo*>(encoders.data());Gdiplus::GetImageEncoders(count,size,codecs);bool saved=false;
            for(UINT i=0;i<count;++i)if(wcscmp(codecs[i].MimeType,L"image/png")==0){assert(picture.Save(L"build/ui-preview.png",&codecs[i].Clsid,nullptr)==Gdiplus::Ok);saved=true;}assert(saved);
        }
        SelectObject(memory,old);DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(window,dc);DestroyWindow(window);
    }
    Gdiplus::GdiplusShutdown(token);
}
