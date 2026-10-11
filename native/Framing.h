#pragma once
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>
namespace kagami {
struct Framing {
    double x=0,y=0,w=1,h=1;
    unsigned rotation=0;
    bool mirror=false,fill=false;
    bool valid() const {
        return std::isfinite(x)&&std::isfinite(y)&&std::isfinite(w)&&std::isfinite(h)&&
            x>=0&&y>=0&&w>0&&h>0&&x+w<=1.000001&&y+h<=1.000001&&rotation<4;
    }
};
// Pure BGRX transformation: crop in source coordinates, rotate, mirror, then fit/fill.
inline std::vector<BYTE> reframe(const std::vector<BYTE>& source,unsigned sw,unsigned sh,const Framing& f,unsigned ow=1280,unsigned oh=720) {
    if(!f.valid()||!sw||!sh||sw>8192||sh>8192||!ow||!oh||ow>8192||oh>8192||source.size()!=size_t(sw)*sh*4) return {};
    unsigned cx=(std::min)(unsigned(f.x*sw),sw-1),cy=(std::min)(unsigned(f.y*sh),sh-1);
    unsigned cw=(std::min)((std::max)(1u,unsigned(f.w*sw)),sw-cx),ch=(std::min)((std::max)(1u,unsigned(f.h*sh)),sh-cy);
    unsigned rw=f.rotation%2?ch:cw,rh=f.rotation%2?cw:ch;
    double sx=double(ow)/rw,sy=double(oh)/rh,factor=f.fill?(std::max)(sx,sy):(std::min)(sx,sy);
    double left=(ow-rw*factor)/2,top=(oh-rh*factor)/2;
    std::vector<BYTE> output(size_t(ow)*oh*4,0);
    for(unsigned y=0;y<oh;++y) for(unsigned x=0;x<ow;++x) {
        int rx=int(std::floor((x+.5-left)/factor)),ry=int(std::floor((y+.5-top)/factor));
        if(rx<0||ry<0||rx>=int(rw)||ry>=int(rh))continue;
        if(f.mirror)rx=rw-1-rx;
        unsigned px=0,py=0;
        switch(f.rotation) {
        case 0:px=rx;py=ry;break;
        case 1:px=ry;py=ch-1-rx;break;
        case 2:px=cw-1-rx;py=ch-1-ry;break;
        case 3:px=cw-1-ry;py=rx;break;
        }
        size_t dest=(size_t(y)*ow+x)*4,src=(size_t(cy+py)*sw+cx+px)*4;
        memcpy(output.data()+dest,source.data()+src,4);
    }
    return output;
}
inline void centeredAspect(Framing& f,double aspect,unsigned sw=1280,unsigned sh=720) {
    f.x=f.y=0; f.w=f.h=1;
    double ratio=double(sw)/sh;
    if(ratio>aspect) {f.w=aspect/ratio;f.x=(1-f.w)/2;}
    else {f.h=ratio/aspect;f.y=(1-f.h)/2;}
}
}
