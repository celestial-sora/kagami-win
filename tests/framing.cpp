#include <windows.h>
#include <cassert>
#include "Framing.h"
int main() {
    std::vector<BYTE> source(3*2*4);
    for(unsigned i=0;i<6;++i)source[i*4]=BYTE(i+1);
    kagami::Framing f;
    auto pixels=kagami::reframe(source,3,2,f,3,2);assert(pixels==source);
    f.mirror=true;pixels=kagami::reframe(source,3,2,f,3,2);assert(pixels[0]==3&&pixels[8]==1&&pixels[12]==6);
    f.mirror=false;f.rotation=1;pixels=kagami::reframe(source,3,2,f,2,3);assert(pixels[0]==4&&pixels[4]==1&&pixels[16]==6&&pixels[20]==3);
    f.rotation=3;pixels=kagami::reframe(source,3,2,f,2,3);assert(pixels[0]==3&&pixels[4]==6&&pixels[16]==1);
    f.rotation=2;pixels=kagami::reframe(source,3,2,f,3,2);assert(pixels[0]==6&&pixels[20]==1);
    f={};f.x=1.0/3;f.w=1.0/3;pixels=kagami::reframe(source,3,2,f,1,2);assert(pixels[0]==2&&pixels[4]==5);
    f={};pixels=kagami::reframe(source,3,2,f,3,4);assert(pixels[0]==0&&pixels[12]==1&&pixels[36]==0);
    f.fill=true;pixels=kagami::reframe(source,3,2,f,1,2);assert(pixels[0]==2&&pixels[4]==5);
    f.w=0;assert(kagami::reframe(source,3,2,f).empty());
    f={};f.x=NAN;assert(kagami::reframe(source,3,2,f).empty());
    f={};kagami::centeredAspect(f,1,1280,720);assert(f.valid()&&f.w<1&&f.h==1);
}
