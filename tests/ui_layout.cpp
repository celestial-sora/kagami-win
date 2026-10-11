#include <windows.h>
#include "FrameBridge.h"
#include "DesktopUI.h"
#include <cassert>
int wmain() {
    Gdiplus::GdiplusStartupInput input;ULONG_PTR token{};assert(Gdiplus::GdiplusStartup(&token,&input,nullptr)==Gdiplus::Ok);
    HWND window=CreateWindowW(L"STATIC",L"Kagami layout check",WS_POPUP,0,0,1400,900,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);assert(window);
    HDC dc=GetDC(window),memory=CreateCompatibleDC(dc);HBITMAP bitmap=CreateCompatibleBitmap(dc,1400,900);auto old=SelectObject(memory,bitmap);
    ui::scale=1;ui::paint(window,memory);assert(ui::previewRect.Width>0&&ui::previewRect.Height>0);
    Gdiplus::Bitmap picture(bitmap,nullptr);UINT count{},size{};Gdiplus::GetImageEncodersSize(&count,&size);
    std::vector<BYTE> encoders(size);auto codecs=reinterpret_cast<Gdiplus::ImageCodecInfo*>(encoders.data());Gdiplus::GetImageEncoders(count,size,codecs);
    for(UINT i=0;i<count;++i)if(wcscmp(codecs[i].MimeType,L"image/png")==0)assert(picture.Save(L"build/ui-preview.png",&codecs[i].Clsid,nullptr)==Gdiplus::Ok);
    SelectObject(memory,old);DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(window,dc);DestroyWindow(window);
    // Picture destructor must run before GDI+ shutdown; process teardown releases the token.
}
