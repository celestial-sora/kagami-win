#include <windows.h>
#include <appmodel.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.ApplicationModel.Activation.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <winrt/Windows.Media.Miracast.h>
#include <winrt/Windows.Media.Playback.h>
#include <winrt/Windows.Media.Core.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <sstream>
#include "SmartViewProtocol.h"
using namespace winrt;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::Media::Miracast;
using namespace winrt::Windows::Media::Playback;
using Microsoft::WRL::ComPtr;
struct ReceiverApp : implements<ReceiverApp,IFrameworkViewSource,IFrameworkView> {
    CoreWindow window{nullptr};CoreApplicationView view{nullptr};
    MiracastReceiverSession session{nullptr};MediaPlayer player{nullptr};
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;
    ComPtr<ID3D11Texture2D> texture,staging;ComPtr<ID3D11RenderTargetView> target;
    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface surface{nullptr};
    std::atomic<bool> stopped{true};std::thread control,transport;HANDLE pipe=INVALID_HANDLE_VALUE;
    std::mutex lock;kagami::smart::Packet packet;std::vector<BYTE> latest;
    IFrameworkView CreateView(){return *this;}
    void Initialize(CoreApplicationView const& value) {
        view=value;
        value.Activated([weak=get_weak()](auto const&,IActivatedEventArgs const& args) {
            if(auto self=weak.get()){self->window.Activate();
                if(args.Kind()==ActivationKind::Launch && self->stopped)self->begin(args.as<LaunchActivatedEventArgs>().Arguments());
            }
        });
        CoreApplication::Suspending([weak=get_weak()](auto const&,auto const&){if(auto self=weak.get())self->finish();});
    }
    void SetWindow(CoreWindow const& value) {
        window=value;
        value.Closed([weak=get_weak()](auto const&,auto const&){if(auto self=weak.get())self->finish();});
        winrt::Windows::UI::ViewManagement::ApplicationView::GetForCurrentView().Title(L"Kagami Smart View · close to stop");
    }
    void Load(hstring const&){}
    void Run(){window.Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessUntilQuit);}
    void Uninitialize(){finish();}
    void notify(kagami::smart::State state,const std::wstring& detail,HRESULT error=S_OK) {
        std::lock_guard guard(lock);packet={};packet.state=state;packet.error=error;
        wcsncpy_s(packet.detail,detail.c_str(),_TRUNCATE);latest.clear();
    }
    void requestExit() noexcept {
        stopped=true;
        try {window.Dispatcher().RunAsync(CoreDispatcherPriority::Normal,[]{CoreApplication::Exit();});}catch(...){}
    }
    void finish() noexcept {
        stopped=true;
        try {if(session)session.Close();}catch(...){}session=nullptr;
        try {if(player)player.Close();}catch(...){}player=nullptr;
        if(control.joinable())control.join();if(transport.joinable())transport.join();
        if(pipe!=INVALID_HANDLE_VALUE){CloseHandle(pipe);pipe=INVALID_HANDLE_VALUE;}
        std::lock_guard guard(lock);latest.clear();surface=nullptr;target.Reset();texture.Reset();staging.Reset();context.Reset();device.Reset();
    }
    fire_and_forget begin(hstring arguments) {
        auto lifetime=get_strong();
        std::wistringstream input(arguments.c_str());std::wstring endpoint,extra;DWORD parent{};input>>endpoint>>parent;
        if(!parent||!kagami::smart::validEndpoint(endpoint)||(input>>extra)){requestExit();co_return;}
        try {
            // Opening the helper from Start never starts reception. Only the desktop
            // that owns this per-session endpoint can authorize the receiver.
            pipe=CreateFileW(endpoint.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,nullptr);
            ULONG server{};
            if(pipe==INVALID_HANDLE_VALUE||!GetNamedPipeServerProcessId(pipe,&server)||server!=parent)throw hresult_error(E_ACCESSDENIED);
            stopped=false;
            control=std::thread([this]{BYTE command{};kagami::transfer(pipe,&command,1,false,stopped);requestExit();});
            transport=std::thread([this]{
                while(!stopped){
                    kagami::smart::Packet outgoing;std::vector<BYTE> pixels;
                    {std::lock_guard guard(lock);outgoing=packet;if(outgoing.state==kagami::smart::State::Video){pixels=latest;outgoing.bytes=kagami::frameBytes;}}
                    if(!kagami::transfer(pipe,&outgoing,sizeof(outgoing),true,stopped)||
                        (outgoing.bytes&&!kagami::transfer(pipe,pixels.data(),outgoing.bytes,true,stopped))){requestExit();break;}
                    Sleep(33);
                }
            });
            MiracastReceiver receiver;
            auto status=co_await receiver.GetStatusAsync();if(stopped)co_return;
            if(status.WiFiStatus()==MiracastReceiverWiFiStatus::MiracastNotSupported)throw hresult_error(E_NOTIMPL,L"This Wi-Fi adapter does not support Miracast reception.");
            auto settings=co_await receiver.GetCurrentSettingsAsync();if(stopped)co_return;
            notify(kagami::smart::State::Listening,L"Smart View: select "+std::wstring(settings.FriendlyName()));
            auto newSession=co_await receiver.CreateSessionAsync(view);if(stopped){newSession.Close();co_return;}session=newSession;
            session.MaxSimultaneousConnections(1);session.AllowConnectionTakeover(false);
            auto weak=get_weak();
            session.ConnectionCreated([weak](auto const&,MiracastReceiverConnectionCreatedEventArgs const& args){
                if(auto self=weak.get()) {
                    if(self->stopped){args.Connection().Disconnect(MiracastReceiverDisconnectReason::ConnectionNotAccepted);return;}
                    if(!args.Pin().empty())self->notify(kagami::smart::State::Pin,L"Enter Smart View PIN: "+std::wstring(args.Pin()));
                }
            });
            session.Disconnected([weak](auto const&,auto const&){if(auto self=weak.get()){self->notify(kagami::smart::State::Failed,L"Smart View disconnected. Restart reception explicitly.",E_ABORT);}});
            session.MediaSourceCreated([weak](auto const&,MiracastReceiverMediaSourceCreatedEventArgs const& args){
                if(auto self=weak.get()){if(self->stopped)return;try{self->play(args.MediaSource());}catch(hresult_error const& e){self->notify(kagami::smart::State::Failed,e.message().c_str(),e.code());}catch(...){self->notify(kagami::smart::State::Failed,L"Cannot decode Smart View media.",E_FAIL);}}
            });
            auto result=co_await session.StartAsync();if(stopped)co_return;
            if(unsigned(result.Status())!=0)throw hresult_error(E_FAIL,L"Windows could not start Smart View. Check Wireless Display and Wi-Fi Direct support.");
        }catch(hresult_error const& e){notify(kagami::smart::State::Failed,e.message().empty()?L"Smart View failed. Check Wireless Display and Windows permissions.":e.message().c_str(),e.code());}
        catch(...){notify(kagami::smart::State::Failed,L"Smart View receiver failed.",E_FAIL);}
    }
    void play(winrt::Windows::Media::Core::MediaSource const& source) {
        check_hresult(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context));
        D3D11_TEXTURE2D_DESC desc{};desc.Width=kagami::width;desc.Height=kagami::height;desc.MipLevels=1;desc.ArraySize=1;desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;desc.SampleDesc.Count=1;desc.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
        check_hresult(device->CreateTexture2D(&desc,nullptr,&texture));check_hresult(device->CreateRenderTargetView(texture.Get(),nullptr,&target));
        desc.BindFlags=0;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;check_hresult(device->CreateTexture2D(&desc,nullptr,&staging));
        ComPtr<IDXGISurface> dxgi;check_hresult(texture.As(&dxgi));com_ptr<IInspectable> inspectable;
        check_hresult(CreateDirect3D11SurfaceFromDXGISurface(dxgi.Get(),inspectable.put()));surface=inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface>();
        player=MediaPlayer();player.CommandManager().IsEnabled(false);player.IsVideoFrameServerEnabled(true);
        auto weak=get_weak();
        player.VideoFrameAvailable([weak](MediaPlayer const& sender,auto const&){if(auto self=weak.get())self->frame(sender);});
        player.MediaFailed([weak](auto const&,auto const&){if(auto self=weak.get())self->notify(kagami::smart::State::Failed,L"Smart View decoder or audio output failed.",E_FAIL);});
        player.MediaEnded([weak](auto const&,auto const&){if(auto self=weak.get())self->notify(kagami::smart::State::Failed,L"Smart View stream ended.",E_ABORT);});
        player.Source(source);player.Play();
    }
    void frame(MediaPlayer const& sender) noexcept {
        try {
            std::unique_lock guard(lock,std::try_to_lock);if(!guard.owns_lock()||stopped||packet.state==kagami::smart::State::Failed)return;
            auto playback=sender.PlaybackSession();double width=playback.NaturalVideoWidth(),height=playback.NaturalVideoHeight();if(width<1||height<1)return;
            double factor=(std::min)(kagami::width/width,kagami::height/height);
            const float black[4]{};context->ClearRenderTargetView(target.Get(),black);
            sender.CopyFrameToVideoSurface(surface,winrt::Windows::Foundation::Rect(float((kagami::width-width*factor)/2),float((kagami::height-height*factor)/2),float(width*factor),float(height*factor)));
            context->CopyResource(staging.Get(),texture.Get());std::vector<BYTE> pixels(kagami::frameBytes);D3D11_MAPPED_SUBRESOURCE mapped{};
            check_hresult(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped));
            for(unsigned y=0;y<kagami::height;++y)memcpy(pixels.data()+size_t(y)*kagami::width*4,static_cast<BYTE*>(mapped.pData)+size_t(y)*mapped.RowPitch,kagami::width*4);
            context->Unmap(staging.Get(),0);latest=std::move(pixels);packet.state=kagami::smart::State::Video;wcscpy_s(packet.detail,L"Smart View receiving · audio uses Windows desktop output.");
        }catch(...){notify(kagami::smart::State::Failed,L"Smart View frame processing failed.",E_FAIL);}
    }
};
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int) {
    try{init_apartment(apartment_type::single_threaded);CoreApplication::Run(make<ReceiverApp>());return 0;}catch(...){return 2;}
}
