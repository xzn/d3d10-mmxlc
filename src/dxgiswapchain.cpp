#include "dxgiswapchain.h"
#include "d3d10device.h"
#include "overlay.h"
#include "d3d10device.h"
#include "d3d10texture2d.h"
#include "log.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyIDXGISwapChain, ## __VA_ARGS__)

IUNKNOWN_IMPL(MyIDXGISwapChain)

MyIDXGISwapChain::MyIDXGISwapChain(
    DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
    IDXGISwapChain **inner,
    ID3D10Device **device
) :
    current_width(pSwapChainDesc->BufferDesc.Width),
    current_height(pSwapChainDesc->BufferDesc.Height),
    IUNKNOWN_INIT(*inner)
{
    LOG_MFUN(_,
        LOG_ARG(*inner),
        LOG_ARG(*device)
    );
    overlay = new Overlay(pSwapChainDesc, *inner, *device);
    default_overlays->insert(overlay);
    my_device = new MyID3D10Device(
        device,
        current_width,
        current_height
    );
    *inner = this;
}

MyIDXGISwapChain::~MyIDXGISwapChain() {
    LOG_MFUN();
    default_overlays->erase(overlay);
    if (overlay) delete overlay;
    if (my_device) my_device->Release();
}

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::Present(
    UINT sync_interval,
    UINT flags
) {
    HRESULT ret = 0;
    my_device->present();
    if (overlay) {
        ret = overlay->present(
            sync_interval,
            flags
        );
    } else {
        ret = inner->Present(
            sync_interval,
            flags
        );
    }
    LOG_MFUN(_, ret);
    default_logger->next_frame();
    return ret;
}

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::GetBuffer(
    UINT buffer_idx,
    REFIID riid,
    void **surface
) {
    if (riid != IID_ID3D10Texture2D) return E_NOTIMPL;
    HRESULT ret = inner->GetBuffer(
        buffer_idx,
        riid,
        surface
    );
    if (ret == S_OK) {
        LOG_MFUN(_,
            LOG_ARG(buffer_idx),
            LOG_ARG(&riid),
            LOG_ARG(*surface),
            ret
        );
        D3D10_TEXTURE2D_DESC desc;
        ((ID3D10Texture2D *)*surface)->GetDesc(&desc);
        new MyID3D10Texture2D((ID3D10Texture2D **)surface, &desc);
    } else {
        LOG_MFUN(_,
            LOG_ARG(buffer_idx),
            LOG_ARG(&riid),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::SetFullscreenState(
    WINBOOL fullscreen,
    IDXGIOutput *target
) {
    LOG_MFUN();
    return inner->SetFullscreenState(
        fullscreen,
        target
    );
}

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::GetFullscreenState(
    WINBOOL *fullscreen,
    IDXGIOutput **target
) {
    LOG_MFUN();
    return inner->GetFullscreenState(
        fullscreen,
        target
    );
}

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::GetDesc(
    DXGI_SWAP_CHAIN_DESC *desc
) {
    LOG_MFUN();
    return inner->GetDesc(desc);
}

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::ResizeBuffers(
    UINT buffer_count,
    UINT width,
    UINT height,
    DXGI_FORMAT format,
    UINT flags
) {
    LOG_MFUN();
    my_device->resize_buffers(width, height);
    HRESULT ret;
    if (overlay) {
         ret = overlay->resize_buffers(
            buffer_count,
            width,
            height,
            format,
            flags
        );
    } else {
        ret = inner->ResizeBuffers(
            buffer_count,
            width,
            height,
            format,
            flags
        );
    }
    if (ret == S_OK) {
        current_width = width;
        current_height = height;
    } else {
        current_width = current_height = 0;
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::ResizeTarget(
    const DXGI_MODE_DESC *target_mode_desc
) {
    LOG_MFUN();
    return inner->ResizeTarget(target_mode_desc);
}

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::GetContainingOutput(
    IDXGIOutput **output
) {
    LOG_MFUN();
    return inner->GetContainingOutput(output);
}

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::GetFrameStatistics(
    DXGI_FRAME_STATISTICS *stats
) {
    LOG_MFUN();
    return inner->GetFrameStatistics(stats);
}

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::GetLastPresentCount(
    UINT *last_present_count
) {
    LOG_MFUN();
    return inner->GetLastPresentCount(last_present_count);
}

// IDXGIDeviceSubObject

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::GetDevice(
    REFIID riid,
    void **device
) {
    LOG_MFUN();
    return inner->GetDevice(
        riid,
        device
    );
}

// IDXGIObject

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::SetPrivateData(
    REFGUID guid,
    UINT data_size,
    const void *data
) {
    LOG_MFUN();
    return inner->SetPrivateData(
        guid,
        data_size,
        data
    );
}

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::SetPrivateDataInterface(
    REFGUID guid,
    const IUnknown *object
) {
    LOG_MFUN();
    return inner->SetPrivateDataInterface(
        guid,
        object
    );
}

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::GetPrivateData(
    REFGUID guid,
    UINT *data_size,
    void *data
) {
    LOG_MFUN();
    return inner->GetPrivateData(
        guid,
        data_size,
        data
    );
}

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::GetParent(
    REFIID riid,
    void **parent
) {
    LOG_MFUN();
    return inner->GetParent(
        riid,
        parent
    );
}
