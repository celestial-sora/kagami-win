#include <windows.h>
#include "FrameBridge.h"
#include "DesktopUI.h"
#include <cassert>
int wmain() {
    Gdiplus::GdiplusStartupInput input;ULONG_PTR token{};assert(Gdiplus::GdiplusStartup(&token,&input,nullptr)==Gdiplus::Ok);
    {
        HWND window=CreateWindowW(L"STATIC",L"Kagami layout check",WS_POPUP,0,0,1400,900,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);assert(window);
        const wchar_t* labels[]{L"Enable camera",L"Test pattern",L"Stop",L"Remove camera",L"Framing controls",L"Framing guide: Off",L"Safe area: Off",L"Show virtual camera output",L"USB / ADB Wi-Fi setup",L"Mirror output: Off",L"Start AirPlay"};
        for(int i=0;i<11;++i)ui::buttons[i]=CreateWindowW(L"BUTTON",labels[i],WS_CHILD|BS_OWNERDRAW,0,0,0,0,window,reinterpret_cast<HMENU>(INT_PTR(i>=9?101+i:100+i)),GetModuleHandleW(nullptr),nullptr);
        ui::layout(window);
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
