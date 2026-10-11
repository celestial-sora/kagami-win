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
    BYTE first=pixels[0]; check(buffer->Unlock(),"unlock sample"); return first;
}
static void exercise(IClassFactory* factory, unsigned mediaTypeIndex) {
    ComPtr<IMFActivate> activate; check(factory->CreateInstance(nullptr,IID_PPV_ARGS(&activate)),"create activate");
    check(activate->SetString(KAGAMI_PIPE_NAME,kagami::pipeName().c_str()),"set local pipe");
    ComPtr<IMFMediaSource> source; check(activate->ActivateObject(IID_PPV_ARGS(&source)),"activate actual DLL source");
    ComPtr<IMFSampleAllocatorControl> control; check(source.As(&control),"allocator control");
    ComPtr<IMFVideoSampleAllocator> allocator; check(MFCreateVideoSampleAllocatorEx(IID_PPV_ARGS(&allocator)),"create allocator");
    check(control->SetDefaultAllocator(0,allocator.Get()),"supply allocator");
    ComPtr<IMFPresentationDescriptor> pd; check(source->CreatePresentationDescriptor(&pd),"presentation descriptor");
    check(pd->SelectStream(0),"select stream"); BOOL selected{};
    ComPtr<IMFStreamDescriptor> descriptor; check(pd->GetStreamDescriptorByIndex(0,&selected,&descriptor),"stream descriptor");
    ComPtr<IMFMediaTypeHandler> handler; check(descriptor->GetMediaTypeHandler(&handler),"type handler");
    ComPtr<IMFMediaType> type; check(handler->GetMediaTypeByIndex(mediaTypeIndex,&type),"advertised type");
    check(handler->SetCurrentMediaType(type.Get()),"select NV12/RGB32");
    PROPVARIANT position{}; check(source->Start(pd.Get(),nullptr,&position),"source Start");
    ComPtr<IMFMediaEvent> event; check(source->GetEvent(MF_EVENT_FLAG_NO_WAIT,&event),"new stream event");
    PROPVARIANT value{}; check(event->GetValue(&value),"new stream value");
    ComPtr<IMFMediaStream> stream; check(value.punkVal->QueryInterface(IID_PPV_ARGS(&stream)),"media stream"); PropVariantClear(&value);
    BYTE black = mediaTypeIndex == 0 ? 16 : 0;
    auto first=sample(stream.Get()); if (firstByte(first.Get())!=black) { std::cerr<<"initial black incorrect\n"; exit(1); }
    kagami::FrameWriter writer;
    if (!writer.start() || !writer.push(std::vector<BYTE>(kagami::frameBytes,73))) { std::cerr<<"writer failed\n"; exit(1); }
    BYTE expected=mediaTypeIndex==0 ? 79 : 73;
    bool received=false;
    for (int i=0;i<100;++i) if (firstByte(sample(stream.Get()).Get())==expected) { received=true; break; }
    if (!received) { std::cerr<<"native sample did not contain pipe pixels\n"; exit(1); }
    writer.stop(); bool cleared=false;
    for (int i=0;i<100;++i) if (firstByte(sample(stream.Get()).Get())==black) { cleared=true; break; }
    if (!cleared) { std::cerr<<"native sample did not clear on EOF\n"; exit(1); }
    check(source->Stop(),"source Stop"); check(activate->ShutdownObject(),"shutdown activation");
    std::cout<<(mediaTypeIndex==0?"NV12":"RGB32")<<": actual DLL activation, allocator, samples, local pixels, EOF black, Stop/Shutdown passed\n";
}
int main() {
    check(CoInitializeEx(nullptr,COINIT_MULTITHREADED),"COM"); check(MFStartup(MF_VERSION),"MFStartup");
    HMODULE dll=LoadLibraryW(L"dist/KagamiCamera.dll"); if (!dll) { std::cerr<<"load DLL failed\n"; return 1; }
    using GetFactory=HRESULT(__stdcall*)(REFCLSID,REFIID,void**);
    auto get=reinterpret_cast<GetFactory>(GetProcAddress(dll,"DllGetClassObject")); if (!get) return 1;
    { ComPtr<IClassFactory> factory; check(get(CLSID_VirtualCameraMediaSource,IID_PPV_ARGS(&factory)),"DLL factory"); exercise(factory.Get(),0); exercise(factory.Get(),1); }
    FreeLibrary(dll); MFShutdown(); CoUninitialize();
}
