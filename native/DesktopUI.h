#pragma once
#include <gdiplus.h>
#include <algorithm>
#include <cstring>
#include <ole2.h>
#pragma comment(lib,"gdiplus.lib")
#pragma comment(lib,"ole32.lib")
namespace ui {
using namespace Gdiplus;
inline HWND buttons[15]{};
inline float scale=1,sidebar=358,vertical=1,rightStart=406;
inline bool running=false,android=false,airplay=false,smartview=false,enabled=false,guide=false,safe=false,output=false,mirrored=false;
inline int selectedSource=0;
inline kagami::OutputFormat format;
inline std::wstring message=L"Stopped. Click Enable camera, then start receiving.";
inline std::vector<BYTE> preview,source;
inline RectF previewRect,imageRect,cropRect(0,0,1,1);
inline Color ink(255,241,228,250),muted(255,177,164,196),border(255,68,63,79);
inline float y(float value){return 114+(value-114)*vertical;}
inline void path(GraphicsPath& p,RectF r,float diameter=18){
    diameter=(std::min)(diameter,(std::min)(r.Width,r.Height));
    p.AddArc(r.X,r.Y,diameter,diameter,180,90);p.AddArc(r.GetRight()-diameter,r.Y,diameter,diameter,270,90);
    p.AddArc(r.GetRight()-diameter,r.GetBottom()-diameter,diameter,diameter,0,90);p.AddArc(r.X,r.GetBottom()-diameter,diameter,diameter,90,90);p.CloseFigure();
}
inline void rounded(Graphics& g,RectF r,Color color,bool outline=false,Color stroke=border){GraphicsPath p;path(p,r);SolidBrush b(color);g.FillPath(&b,&p);if(outline){Pen pen(stroke);g.DrawPath(&pen,&p);}}
inline void text(Graphics& g,const wchar_t* value,float x,float top,float width,float height,float size=15,bool bold=false,Color color=ink,bool center=false){
    Font font(L"Segoe UI",size,bold?FontStyleBold:FontStyleRegular,UnitPixel);SolidBrush brush(color);StringFormat f;
    f.SetTrimming(StringTrimmingEllipsisWord);if(center)f.SetAlignment(StringAlignmentCenter);
    g.DrawString(value,-1,&font,RectF(x,top,width,height),&f,&brush);
}
inline void monitor(Graphics& g,float x,float top,float size){
    Pen pen(muted,2.5f);g.DrawRectangle(&pen,x,top,size,size*.65f);g.DrawLine(&pen,x+size*.35f,top+size*.65f,x+size*.35f,top+size*.85f);g.DrawLine(&pen,x+size*.65f,top+size*.65f,x+size*.65f,top+size*.85f);g.DrawLine(&pen,x+size*.2f,top+size*.85f,x+size*.8f,top+size*.85f);
}
inline void sourceIcon(Graphics& g,float x,float top,float size,bool cast){
    const auto module=GetModuleHandleW(nullptr);
    const auto resource=FindResourceW(module,MAKEINTRESOURCEW(cast?202:201),RT_RCDATA);
    if(!resource)return;
    const DWORD bytes=SizeofResource(module,resource);
    const auto data=LockResource(LoadResource(module,resource));if(!data||!bytes)return;
    HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,bytes);if(!memory)return;
    void* copy=GlobalLock(memory);if(!copy){GlobalFree(memory);return;}
    memcpy(copy,data,bytes);GlobalUnlock(memory);
    IStream* stream{};
    if(FAILED(CreateStreamOnHGlobal(memory,TRUE,&stream))){GlobalFree(memory);return;}
    {
        // GDI+ retains its stream while decoding. Both are scoped to this draw,
        // with the image destroyed first, before app/test GDI+ shutdown.
        Image image(stream);
        if(image.GetLastStatus()==Ok){
            const auto saved=g.Save();g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
            g.DrawImage(&image,RectF(x,top,size,size));g.Restore(saved);
        }
    }
    stream->Release();
}
inline void place(int i,float x,float top,float width,float height){MoveWindow(buttons[i],int(x*scale),int(top*scale),int(width*scale),int(height*scale),TRUE);}
inline void layout(HWND hwnd){
    RECT r;GetClientRect(hwnd,&r);float dpi=GetDpiForWindow(hwnd)/96.f;
    scale=dpi*(std::min)(1.f,(std::min)(r.right/(1100.f*dpi),r.bottom/(900.f*dpi)));
    if(scale<=0)return;float width=r.right/scale,height=r.bottom/scale;
    sidebar=std::clamp(width*.28125f,358.f,540.f);rightStart=sidebar+48;vertical=(height-148)/908;
    place(10,width-552,28,132,44);place(0,width-410,28,132,44);place(1,width-268,28,126,44);place(2,width-132,28,110,44);
    float tile=(sidebar-46)/2;place(13,34,y(164),tile,116*vertical);place(11,48+tile,y(164),tile,116*vertical);place(8,34,y(290),sidebar-32,28*vertical);
    place(3,30,y(976),(sidebar-38)/2,34*vertical);place(4,44+(sidebar-38)/2,y(976),(sidebar-38)/2,34*vertical);
    place(5,30,y(814),sidebar-24,36*vertical);place(6,30,y(862),sidebar-24,36*vertical);place(9,30,y(910),sidebar-24,36*vertical);
    float input=18+sidebar*.29f;place(12,input,y(635),sidebar-(input-18)-18,30*vertical);
    place(14,sidebar-70,y(480),72,28*vertical);
    place(7,rightStart,height-34,width-rightStart-18,26);
    EnableWindow(buttons[1],enabled&&!running);EnableWindow(buttons[0],!running);EnableWindow(buttons[8],enabled&&!running);EnableWindow(buttons[2],running);EnableWindow(buttons[10],enabled&&!running);EnableWindow(buttons[11],!running);EnableWindow(buttons[13],!running);EnableWindow(buttons[12],!running);
    SetWindowTextW(buttons[10],selectedSource==1?L"Start Smart View":L"Start AirPlay");
    SetWindowTextW(buttons[11],selectedSource==1?L"Smart View (selected)":L"Smart View");SetWindowTextW(buttons[13],selectedSource==0?L"AirPlay (selected)":L"AirPlay");
    wchar_t label[90];swprintf_s(label,L"%u × %u · %u FPS",format.width,format.height,format.fps);SetWindowTextW(buttons[12],label);
}
inline void paint(HWND hwnd,HDC dc){
    RECT r;GetClientRect(hwnd,&r);if(!r.right||!r.bottom)return;Bitmap buffer(r.right,r.bottom,PixelFormat32bppPARGB);Graphics g(&buffer);
    g.SetSmoothingMode(SmoothingModeAntiAlias);g.Clear(Color(255,25,24,34));g.ScaleTransform(scale,scale);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    float width=r.right/scale,height=r.bottom/scale,right=width-rightStart,inner=rightStart+16,input=18+sidebar*.29f;
    SolidBrush top(Color(255,33,31,44));g.FillRectangle(&top,0.f,0.f,width,96.f);
    text(g,L"Kagami · 鏡",88,19,300,36,28,true);text(g,L"Phone screen → Virtual Camera",89,56,330,26,15,false,muted);
    auto panel=[&](float top,float h,Color c){rounded(g,RectF(18,y(top),sidebar,h*vertical),c,true);};
    panel(114,216,Color(255,33,32,43));text(g,L"Connection Source",34,y(128),sidebar-32,28,20,true);
    panel(346,104,Color(255,51,48,62));
    const wchar_t* state=smartview?(source.empty()?L"●  Waiting for Smart View":L"●  Smart View receiving"):airplay?(source.empty()?L"●  Waiting for AirPlay":L"●  AirPlay receiving"):android?L"●  Android receiver":running?L"●  Test pattern running":L"●  Not connected";
    text(g,state,34,y(361),sidebar-32,26,18,true,Color(255,235,157,247));text(g,message.c_str(),34,y(398),sidebar-32,40*vertical,13,false,muted);
    panel(466,282,Color(255,33,32,43));text(g,L"Camera Settings",34,y(484),sidebar-32,28,20,true);
    const wchar_t* fields[]{L"Input",L"Output",L"Resolution",L"Framerate"};
    const wchar_t* values[]{selectedSource==1?L"Smart View / Miracast":L"AirPlay · USB / ADB",L"Kagami",L"",L""};
    for(int i=0;i<4;++i){float top=y(533+i*48);text(g,fields[i],34,top+5,input-48,26,15);if(i<3){rounded(g,RectF(input,top,sidebar-(input-18)-18,34*vertical),Color(255,43,41,54),true);text(g,values[i],input+12,top+5,sidebar-(input-18)-42,26,15,false,i==0?muted:ink);}}
    // The format button spans the resolution/framerate choice without starting media.
    wchar_t fps[48];swprintf_s(fps,L"%u FPS target",format.fps);text(g,fps,input,y(677),sidebar-(input-18)-24,26,15,false,muted);
    panel(764,184,Color(255,33,32,43));text(g,L"Preview Options",34,y(779),sidebar-32,28,20,true);
    panel(964,58,Color(255,33,32,43));text(g,L"Development preview · celestail-sora",30,height-27,sidebar-24,22,12,false,muted);
    Pen divider(border);g.DrawLine(&divider,sidebar+32,114.f,sidebar+32,height-18);
    float ph=height-320;rounded(g,RectF(rightStart,114,right-18,ph),Color(255,33,32,43),true);
    text(g,output?L"▣  Virtual camera output":L"▣  Live Preview",inner,130,330,28,20,true);
    rounded(g,RectF(width-213,126,182,30),Color(255,43,38,56),true);wchar_t badge[80];swprintf_s(badge,L"%u × %u · %u FPS",format.width,format.height,format.fps);text(g,badge,width-209,132,174,24,13,false,muted,true);
    previewRect=RectF(inner,166,right-50,ph-68);imageRect=previewRect;
    GraphicsPath screenPath;path(screenPath,previewRect,28);PathGradientBrush backdrop(&screenPath);backdrop.SetCenterColor(Color(255,66,42,84));Color edge(255,31,25,45);int count=1;backdrop.SetSurroundColors(&edge,&count);g.FillPath(&backdrop,&screenPath);
    auto saved=g.Save();g.SetClip(&screenPath);
    auto& shown=output?preview:source;unsigned sw=output?format.width:kagami::width,sh=output?format.height:kagami::height;
    if(running&&shown.size()==size_t(sw)*sh*4){
        Bitmap frame(sw,sh,sw*4,PixelFormat32bppRGB,shown.data());float factor=(std::min)(previewRect.Width/sw,previewRect.Height/sh);
        imageRect=RectF(previewRect.X+(previewRect.Width-sw*factor)/2,previewRect.Y+(previewRect.Height-sh*factor)/2,sw*factor,sh*factor);g.DrawImage(&frame,imageRect);
        if(!output){Pen selection(Color(255,235,157,247),2);g.DrawRectangle(&selection,imageRect.X+cropRect.X*imageRect.Width,imageRect.Y+cropRect.Y*imageRect.Height,cropRect.Width*imageRect.Width,cropRect.Height*imageRect.Height);}
    }else{float center=previewRect.X+previewRect.Width/2,cy=previewRect.Y+previewRect.Height/2;monitor(g,center-29,cy-72,58);text(g,L"Your phone screen appears here",center-240,cy-1,480,32,20,true,ink,true);text(g,L"Start receiving, then connect to Kagami.",center-240,cy+34,480,28,15,false,muted,true);}
    if(guide){Pen line(Color(160,240,220,250));for(int i=1;i<3;++i){float x=imageRect.X+imageRect.Width*i/3,top=imageRect.Y+imageRect.Height*i/3;g.DrawLine(&line,x,imageRect.Y,x,imageRect.GetBottom());g.DrawLine(&line,imageRect.X,top,imageRect.GetRight(),top);}}
    if(safe){Pen line(Color(200,240,220,250));line.SetDashStyle(DashStyleDash);g.DrawRectangle(&line,imageRect.X+imageRect.Width*.05f,imageRect.Y+imageRect.Height*.05f,imageRect.Width*.9f,imageRect.Height*.9f);}g.Restore(saved);
    rounded(g,RectF(rightStart,height-184,right-18,142),Color(255,33,32,43),true);text(g,L"How to Use",inner,height-171,300,26,20,true);
    float column=(right-58)/3;const wchar_t* headings[]{L"Start AirPlay / Smart View",L"Adjust Crop (Optional)",L"Use in OBS / Apps"};const wchar_t* descriptions[]{L"Enable camera, start receiving, then mirror your phone. USB / ADB setup is also available.",L"Drag the source preview to crop. Open framing controls for rotation, mirror and presets.",L"Select Kagami as your camera. Wireless receiver audio uses Desktop Audio."};
    for(int i=0;i<3;++i){float x=inner+i*column;SolidBrush step(i==0?Color(255,89,81,255):i==1?Color(255,172,69,235):Color(255,230,87,218));g.FillEllipse(&step,x,height-139,34.f,34.f);wchar_t n[]{wchar_t(L'1'+i),0};text(g,n,x,height-137,34,32,22,true,ink,true);if(i==0)sourceIcon(g,x+54,height-133,28,false);else monitor(g,x+54,height-134,28);text(g,headings[i],x,height-99,column-16,27,15,true);text(g,descriptions[i],x,height-72,column-16,28,13,false,muted);}
    Graphics screen(dc);screen.DrawImage(&buffer,0,0);HICON icon=LoadIconW(GetModuleHandleW(nullptr),MAKEINTRESOURCEW(1));DrawIconEx(dc,int(26*scale),int(22*scale),icon,int(48*scale),int(48*scale),0,nullptr,DI_NORMAL);
}
inline void drawButton(DRAWITEMSTRUCT* item){
    Graphics g(item->hDC);g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);float width=float(item->rcItem.right),height=float(item->rcItem.bottom);SolidBrush bg(Color(255,33,32,43));g.FillRectangle(&bg,0.f,0.f,width,height);
    bool disabled=(item->itemState&ODS_DISABLED)!=0,pressed=(item->itemState&ODS_SELECTED)!=0;
    if(item->CtlID==112||item->CtlID==115){
        bool cast=item->CtlID==112,selected=selectedSource==(cast?1:0);g.ScaleTransform(scale,scale);width/=scale;height/=scale;
        rounded(g,RectF(1,1,width-2,height-2),selected?Color(255,86,56,98):Color(255,47,44,59),true,selected?Color(255,231,148,247):border);
        sourceIcon(g,16,14,30,cast);text(g,cast?L"Smart View":L"AirPlay",16,height-48,width-30,25,20,true);text(g,cast?L"Samsung / Android":L"iPhone / iPad / Mac",16,height-22,width-24,21,13,false,muted);
        Pen radio(ink);if(selected){SolidBrush dot(ink);g.FillEllipse(&dot,width-26,20.f,10.f,10.f);}else g.DrawEllipse(&radio,width-26,20.f,10.f,10.f);
    }else if(item->CtlID==105||item->CtlID==106||item->CtlID==110){
        bool on=item->CtlID==105?guide:item->CtlID==106?safe:mirrored;const wchar_t* label=item->CtlID==105?L"Show Framing Guide":item->CtlID==106?L"Show Safe Area (OBS)":L"Mirror Preview / Output";
        g.ScaleTransform(scale,scale);width/=scale;height/=scale;text(g,label,8,(height-24)/2,width-70,26,15);rounded(g,RectF(width-58,(height-28)/2,54,28),on?Color(255,162,81,250):Color(255,81,77,96));SolidBrush thumb(ink);g.FillEllipse(&thumb,width-(on?28.f:55.f),(height-22)/2,22.f,22.f);
    }else{
        Color color=disabled?Color(255,42,39,51):pressed?Color(255,113,55,160):item->CtlID==111?Color(255,168,75,237):Color(255,57,52,69);rounded(g,RectF(1,1,width-2,height-2),color,true);wchar_t label[128];GetWindowTextW(item->hwndItem,label,128);Font font(L"Segoe UI",14*scale,FontStyleRegular,UnitPixel);SolidBrush brush(disabled?Color(255,130,121,145):ink);StringFormat f;f.SetAlignment(StringAlignmentCenter);f.SetLineAlignment(StringAlignmentCenter);g.DrawString(label,-1,&font,RectF(0,0,width,height),&f,&brush);
    }
    if(item->itemState&ODS_FOCUS){g.ResetTransform();RECT focus=item->rcItem;InflateRect(&focus,-3,-3);DrawFocusRect(item->hDC,&focus);}
}
inline void refresh(HWND hwnd){layout(hwnd);InvalidateRect(hwnd,nullptr,FALSE);}
}
