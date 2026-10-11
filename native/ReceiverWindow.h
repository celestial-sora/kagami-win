#pragma once
#include <d3d11.h>
#include <dxgi1_2.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include "Framing.h"
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"windowsapp.lib")
namespace kagami {
// Capture only a window owned by the explicitly started receiver process.
// No monitor capture, saved frames, remote IPC or physical camera access.
class ReceiverWindow {
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item{nullptr};
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool pool{nullptr};
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession session{nullptr};
    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice runtimeDevice{nullptr};
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> staging;
    winrt::Windows::Graphics::SizeInt32 size{};
public:
    void stop() noexcept {
        try {if(session)session.Close();if(pool)pool.Close();}catch(...){}
        session=nullptr;pool=nullptr;item=nullptr;runtimeDevice=nullptr;
        staging.Reset();context.Reset();device.Reset();size={};
    }
    ~ReceiverWindow(){stop();}
    void start(HWND window,DWORD expectedPid) {
        stop();DWORD pid{};GetWindowThreadProcessId(window,&pid);
        if(pid!=expectedPid||!IsWindowVisible(window))throw winrt::hresult_error(E_ACCESSDENIED);
        if(!winrt::Windows::Graphics::Capture::GraphicsCaptureSession::IsSupported())throw winrt::hresult_error(E_NOTIMPL);
        auto interop=winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem,IGraphicsCaptureItemInterop>();
        winrt::check_hresult(interop->CreateForWindow(window,winrt::guid_of<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>(),winrt::put_abi(item)));
        winrt::check_hresult(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context));
        Microsoft::WRL::ComPtr<IDXGIDevice> dxgi;winrt::check_hresult(device.As(&dxgi));
        winrt::com_ptr<IInspectable> inspectable;
        winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgi.Get(),inspectable.put()));
        runtimeDevice=inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
        size=item.Size();
        if(size.Width<1||size.Height<1||size.Width>8192||size.Height>8192)throw winrt::hresult_error(E_INVALIDARG);
        pool=winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::CreateFreeThreaded(runtimeDevice,winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,1,size);
        session=pool.CreateCaptureSession(item);session.IsCursorCaptureEnabled(false);session.StartCapture();
    }
    std::vector<BYTE> take() {
        if(!pool)return {};
        auto frame=pool.TryGetNextFrame();if(!frame)return {};
        auto actual=frame.ContentSize();
        if(actual.Width<=0||actual.Height<=0||actual.Width>8192||actual.Height>8192){frame.Close();return {};}
        if(actual.Width!=size.Width||actual.Height!=size.Height){frame.Close();size=actual;staging.Reset();pool.Recreate(runtimeDevice,winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,1,size);return {};}
        auto access=frame.Surface().as<IDirect3DDxgiInterfaceAccess>();
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;winrt::check_hresult(access->GetInterface(IID_PPV_ARGS(&texture)));
        D3D11_TEXTURE2D_DESC desc{};texture->GetDesc(&desc);
        if(desc.Width<unsigned(size.Width)||desc.Height<unsigned(size.Height)){frame.Close();return {};}
        if(!staging){desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;desc.MiscFlags=0;winrt::check_hresult(device->CreateTexture2D(&desc,nullptr,&staging));}
        context->CopyResource(staging.Get(),texture.Get());D3D11_MAPPED_SUBRESOURCE map{};
        std::vector<BYTE> pixels(size_t(size.Width)*size.Height*4);
        winrt::check_hresult(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&map));
        for(int y=0;y<size.Height;++y)memcpy(pixels.data()+size_t(y)*size.Width*4,static_cast<BYTE*>(map.pData)+size_t(y)*map.RowPitch,size_t(size.Width)*4);
        context->Unmap(staging.Get(),0);frame.Close();
        return reframe(pixels,size.Width,size.Height,Framing{});
    }
};
}
