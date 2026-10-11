#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <wrl/client.h>
#include <iostream>
#include <chrono>
#include "FrameBridge.h"
#include "source/VirtualCameraMediaSource.h"
using Microsoft::WRL::ComPtr;
static void check(HRESULT hr, const char* step) { if (FAILED(hr)) { std::cerr<<step<<": 0x"<<std::hex<<hr<<std::endl; exit(1); } }
static ComPtr<IMFSample> sample(IMFMediaStream* stream) {
    check(stream->RequestSample(nullptr),"request sample");
    auto end=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    while (std::chrono::steady_clock::now()<end) {
        ComPtr<IMFMediaEvent> event;
        HRESULT hr=stream->GetEvent(MF_EVENT_FLAG_NO_WAIT,&event);
        if (hr==MF_E_NO_EVENTS_AVAILABLE) { Sleep(5); continue; }
        check(hr,"stream event");
        MediaEventType type{}; check(event->GetType(&type),"event type");
        if (type==MEMediaSample) {
            PROPVARIANT v{}; check(event->GetValue(&v),"sample event value");
            ComPtr<IMFSample> result; check(v.punkVal->QueryInterface(IID_PPV_ARGS(&result)),"sample interface"); PropVariantClear(&v); return result;
        }
    }
    std::cerr<<"sample timeout\n"; exit(1);
}
static BYTE firstByte(IMFSample* s) {
    ComPtr<IMFMediaBuffer> buffer; check(s->ConvertToContiguousBuffer(&buffer),"contiguous sample");
    BYTE* pixels{}; DWORD size{}; check(buffer->Lock(&pixels,nullptr,&size),"lock sample");
    if(!size){std::cerr<<"empty sample\n";exit(1);}
    BYTE first=pixels[0]; check(buffer->Unlock(),"unlock sample"); return first;
}
static void uniformSample(IMFSample* s,unsigned typeIndex,kagami::OutputFormat format,BYTE expected){
    ComPtr<IMFMediaBuffer> buffer;check(s->GetBufferByIndex(0,&buffer),"sample buffer");
    ComPtr<IMF2DBuffer2> twoD;check(buffer.As(&twoD),"2D sample buffer");
    BYTE *scan{},*base{};LONG stride{};DWORD length{};
    check(twoD->Lock2DSize(MF2DBuffer_LockFlags_Read,&scan,&stride,&base,&length),"read sample rows");
    bool ok=true;
    for(unsigned y=0;y<format.height;++y){
        auto row=scan+ptrdiff_t(y)*stride;unsigned rowBytes=format.width*(typeIndex==0?1:4);
        if(row<base||row+rowBytes>base+length){ok=false;break;}
        for(unsigned x=0;x<rowBytes;++x)if(row[x]!=expected){ok=false;break;}
    }
    if(typeIndex==0)for(unsigned y=0;y<format.height/2;++y){
        auto row=scan+ptrdiff_t(format.height+y)*stride;
        if(row<base||row+format.width>base+length){ok=false;break;}
        for(unsigned x=0;x<format.width;++x)if(row[x]!=128){ok=false;break;}
    }
    check(twoD->Unlock2D(),"unlock read rows");
    if(!ok){std::cerr<<"sample pixels/stride/planes incorrect\n";exit(1);}
}
static void exercise(IClassFactory* factory, unsigned mediaTypeIndex,kagami::OutputFormat format={}) {
    ComPtr<IMFActivate> activate; check(factory->CreateInstance(nullptr,IID_PPV_ARGS(&activate)),"create activate");
    check(activate->SetString(KAGAMI_PIPE_NAME,kagami::pipeName().c_str()),"set local pipe");
    check(activate->SetUINT32(KAGAMI_OUTPUT_WIDTH,format.width),"output width");
    check(activate->SetUINT32(KAGAMI_OUTPUT_HEIGHT,format.height),"output height");
    check(activate->SetUINT32(KAGAMI_OUTPUT_FPS,format.fps),"output FPS");
    ComPtr<IMFMediaSource> source; check(activate->ActivateObject(IID_PPV_ARGS(&source)),"activate actual DLL source");
    ComPtr<IMFMediaSource> reopened; check(activate->ActivateObject(IID_PPV_ARGS(&reopened)),"reopen existing source");
    if (reopened.Get() != source.Get()) { std::cerr<<"activation replaced a live source\n"; exit(1); }
    reopened.Reset();
    ComPtr<IMFSampleAllocatorControl> control; check(source.As(&control),"allocator control");
    ComPtr<IMFVideoSampleAllocator> allocator; check(MFCreateVideoSampleAllocatorEx(IID_PPV_ARGS(&allocator)),"create allocator");
    check(control->SetDefaultAllocator(0,allocator.Get()),"supply allocator");
    ComPtr<IMFPresentationDescriptor> pd; check(source->CreatePresentationDescriptor(&pd),"presentation descriptor");
    check(pd->SelectStream(0),"select stream"); BOOL selected{};
    ComPtr<IMFStreamDescriptor> descriptor; check(pd->GetStreamDescriptorByIndex(0,&selected,&descriptor),"stream descriptor");
    ComPtr<IMFMediaTypeHandler> handler; check(descriptor->GetMediaTypeHandler(&handler),"type handler");
    ComPtr<IMFMediaType> type; check(handler->GetMediaTypeByIndex(mediaTypeIndex,&type),"advertised type");
    UINT32 width{},height{},fps{},denominator{};check(MFGetAttributeSize(type.Get(),MF_MT_FRAME_SIZE,&width,&height),"advertised dimensions");
    check(MFGetAttributeRatio(type.Get(),MF_MT_FRAME_RATE,&fps,&denominator),"advertised frame rate");
    if(width!=format.width||height!=format.height||fps!=format.fps||denominator!=1){std::cerr<<"camera format mismatch\n";exit(1);}
    check(handler->SetCurrentMediaType(type.Get()),"select NV12/RGB32");
    PROPVARIANT position{}; check(source->Start(pd.Get(),nullptr,&position),"source Start");
    ComPtr<IMFMediaEvent> event; check(source->GetEvent(MF_EVENT_FLAG_NO_WAIT,&event),"new stream event");
    PROPVARIANT value{}; check(event->GetValue(&value),"new stream value");
    ComPtr<IMFMediaStream> stream; check(value.punkVal->QueryInterface(IID_PPV_ARGS(&stream)),"media stream"); PropVariantClear(&value);
    BYTE black = mediaTypeIndex == 0 ? 16 : 0;
    auto first=sample(stream.Get()); if (firstByte(first.Get())!=black) { std::cerr<<"initial black incorrect\n"; exit(1); }
    uniformSample(first.Get(),mediaTypeIndex,format,black);
    LONGLONG duration{};check(first->GetSampleDuration(&duration),"sample duration");if(duration!=10000000/format.fps){std::cerr<<"sample rate mismatch\n";exit(1);}
    kagami::FrameWriter writer;
    if (!writer.start() || !writer.push(std::vector<BYTE>(size_t(format.width)*format.height*4,73),format)) { std::cerr<<"writer failed\n"; exit(1); }
    BYTE expected=mediaTypeIndex==0 ? 79 : 73;
    bool received=false;
    for (int i=0;i<100;++i) {
        auto next=sample(stream.Get());
        if(firstByte(next.Get())==expected){uniformSample(next.Get(),mediaTypeIndex,format,expected);received=true;break;}
    }
    if (!received) { std::cerr<<"native sample did not contain pipe pixels\n"; exit(1); }
    if(mediaTypeIndex==0){
        std::vector<BYTE> colors(size_t(format.width)*format.height*4,0);
        for(unsigned y=0;y<format.height;++y)for(unsigned x=0;x<format.width;++x){
            const size_t i=(size_t(y)*format.width+x)*4;
            // Three red pixels and one blue pixel per 2x2 block.
            colors[i+((y%2==1&&x%2==0)?0:2)]=255;
        }
        if(!writer.push(colors,format)){std::cerr<<"color writer failed\n";exit(1);}
        bool converted=false;
        for(int i=0;i<100;++i){
            auto next=sample(stream.Get());if(firstByte(next.Get())!=82)continue;
            ComPtr<IMFMediaBuffer> buffer;check(next->GetBufferByIndex(0,&buffer),"color buffer");
            ComPtr<IMF2DBuffer2> twoD;check(buffer.As(&twoD),"color 2D buffer");
            BYTE *scan{},*base{};LONG stride{};DWORD length{};
            check(twoD->Lock2DSize(MF2DBuffer_LockFlags_Read,&scan,&stride,&base,&length),"color read rows");
            auto uv=scan+ptrdiff_t(format.height)*stride;
            bool correct=scan[0]==82&&scan[1]==82&&scan[stride]==41&&scan[stride+1]==82&&uv[0]==128&&uv[1]==207;
            check(twoD->Unlock2D(),"color unlock rows");
            if(!correct){std::cerr<<"NV12 2x2 chroma averaging incorrect\n";exit(1);}converted=true;break;
        }
        if(!converted){std::cerr<<"NV12 color sample timeout\n";exit(1);}
    }
    writer.stop(); bool cleared=false;
    for (int i=0;i<100;++i) {
        auto next=sample(stream.Get());
        if(firstByte(next.Get())==black){uniformSample(next.Get(),mediaTypeIndex,format,black);cleared=true;break;}
    }
    if (!cleared) { std::cerr<<"native sample did not clear on EOF\n"; exit(1); }
    check(source->Stop(),"source Stop"); check(activate->ShutdownObject(),"shutdown activation");
    std::cout<<(mediaTypeIndex==0?"NV12":"RGB32")<<": actual DLL activation, allocator, samples, local pixels, EOF black, Stop/Shutdown passed\n";
}
int main() {
    check(CoInitializeEx(nullptr,COINIT_MULTITHREADED),"COM"); check(MFStartup(MF_VERSION),"MFStartup");
    HMODULE dll=LoadLibraryW(L"dist/KagamiCamera.dll"); if (!dll) { std::cerr<<"load DLL failed\n"; return 1; }
    using GetFactory=HRESULT(__stdcall*)(REFCLSID,REFIID,void**);
    auto get=reinterpret_cast<GetFactory>(GetProcAddress(dll,"DllGetClassObject")); if (!get) return 1;
    { ComPtr<IClassFactory> factory; check(get(CLSID_VirtualCameraMediaSource,IID_PPV_ARGS(&factory)),"DLL factory");
      for(auto format:{kagami::OutputFormat{},kagami::OutputFormat{1920,1080,60},kagami::OutputFormat{720,1280,24}})for(unsigned type=0;type<2;++type)exercise(factory.Get(),type,format);
    }
    FreeLibrary(dll); MFShutdown(); CoUninitialize();
}
