#include "dxgiswapchain.h"
#include "d3d10device.h"
#include "overlay.h"
#include "d3d10device.h"
#include "d3d10texture2d.h"
#include "d3d10rendertargetview.h"
#include "d3d10shaderresourceview.h"
#include "d3d10depthstencilview.h"
#include "conf.h"
#include "log.h"
#include "tex.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyIDXGISwapChain, ## __VA_ARGS__)

extern UINT64 xorshift128p();

IUNKNOWN_IMPL(MyIDXGISwapChain)

void MyIDXGISwapChain::update_config() {
if constexpr (ENABLE_CUSTOM_RESOLUTION) {
    if (default_config->render_display_updated) {
        EnterCriticalSection(&default_config->cs);

        if (config.render_3d_width != default_config->render_3d_width || config.render_3d_height != default_config->render_3d_height) {
            config.render_3d_width = default_config->render_3d_width;
            config.render_3d_height = default_config->render_3d_height;
            config.render_3d_updated = true;
        }

        if (config.display_width != default_config->display_width || config.display_height != default_config->display_height) {
            config.display_width = default_config->display_width;
            config.display_height = default_config->display_height;
            config.display_updated = true;
        }

        default_config->render_display_updated = false;

        LeaveCriticalSection(&default_config->cs);
    }

    if ((config.display_updated || is_config_display) && (config.display_flags != cached_flags || config.display_buffer_count != cached_buffer_count || config.display_format != cached_format)) {
        config.display_flags = cached_flags;
        config.display_buffer_count = cached_buffer_count;
        config.display_format = cached_format;
        config.display_updated = true;
    }

    if (config.display_updated) {
        my_device->inner->OMSetRenderTargets(0, NULL, NULL);

        for (MyID3D10Texture2D *bb : bbs) {
            for (MyID3D10RenderTargetView *rtv : bb->rtvs) {
                if (rtv->inner) {
                    cached_rtvs_map.erase(rtv->inner);
                    rtv->inner->Release();
                    rtv->inner = NULL;
                }
            }
            for (MyID3D10ShaderResourceView *srv : bb->srvs) {
                if (srv->inner) {
                    cached_srvs_map.erase(srv->inner);
                    srv->inner->Release();
                    srv->inner = NULL;
                }
            }
            for (MyID3D10DepthStencilView *dsv : bb->dsvs) {
                if (dsv->inner) {
                    cached_dsvs_map.erase(dsv->inner);
                    dsv->inner->Release();
                    dsv->inner = NULL;
                }
            }
            if (bb->inner) {
                bb->inner->Release();
                bb->inner = NULL;
            }
        }

        if (config.display_width && config.display_height) {
            HRESULT ret = my_resize_buffers(config.display_width, config.display_height, config.display_flags, config.display_buffer_count, config.display_format);
            if (ret == S_OK) {
                is_config_display = true;
                Overlay::push_text("Display resolution set to ", std::to_string(config.display_width), "x", std::to_string(config.display_height));
            } else {
                Overlay::push_text("Failed to set display resolution to ", std::to_string(config.display_width), "x", std::to_string(config.display_height));
            }
        } else {
            my_resize_buffers(cached_width, cached_height, cached_flags, cached_buffer_count, cached_format);
            Overlay::push_text("Restoring display resolution to ", std::to_string(cached_width), "x", std::to_string(cached_height));
            is_config_display = false;
        }

        for (MyID3D10Texture2D *bb : bbs) {
            inner->GetBuffer(
                0,
                IID_ID3D10Texture2D,
                (void **)&bb->inner
            );
            for (MyID3D10RenderTargetView *rtv : bb->rtvs) {
                my_device->inner->CreateRenderTargetView(bb->inner, &rtv->desc, &rtv->inner);
                cached_rtvs_map.emplace(rtv->inner, rtv);
            }
            for (MyID3D10ShaderResourceView *srv : bb->srvs) {
                my_device->inner->CreateShaderResourceView(bb->inner, &srv->desc, &srv->inner);
                cached_srvs_map.emplace(srv->inner, srv);
            }
            for (MyID3D10DepthStencilView *dsv : bb->dsvs) {
                my_device->inner->CreateDepthStencilView(bb->inner, &dsv->desc, &dsv->inner);
                cached_dsvs_map.emplace(dsv->inner, dsv);
            }
            bb->inner->GetDesc(&bb->desc);
        }

        config.display_updated = false;
    }

    if (config.render_3d_updated) {
        my_device->resize_render_3d(config.render_3d_width, config.render_3d_height);
        config.render_3d_updated = false;
    }
}
}

MyIDXGISwapChain::MyIDXGISwapChain(
    DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
    IDXGISwapChain **inner,
    ID3D10Device **device
) :
    cached_width(pSwapChainDesc->BufferDesc.Width),
    cached_height(pSwapChainDesc->BufferDesc.Height),
    cached_flags(pSwapChainDesc->Flags),
    cached_buffer_count(pSwapChainDesc->BufferCount),
    cached_format(pSwapChainDesc->BufferDesc.Format),
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
        cached_width,
        cached_height
    );
    my_device->AddRef();
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
    update_config();
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
        D3D10_TEXTURE2D_DESC desc_orig = {
            .Width = cached_width,
            .Height = cached_height
        };
        MyID3D10Texture2D *bb = new MyID3D10Texture2D((ID3D10Texture2D **)surface, &desc_orig, xorshift128p(), this);
        bb->desc = desc;
        bbs.insert(bb);
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

HRESULT MyIDXGISwapChain::my_resize_buffers(
    UINT width,
    UINT height,
    UINT flags,
    UINT buffer_count,
    DXGI_FORMAT format
) {
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
        my_device->resize_buffers(width, height);
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::ResizeBuffers(
    UINT buffer_count,
    UINT width,
    UINT height,
    DXGI_FORMAT format,
    UINT flags
) {
    LOG_MFUN();

    cached_width = width;
    cached_height = height;
    cached_flags = flags;
    cached_buffer_count = buffer_count;
    cached_format = format;
    update_config();
    HRESULT ret = is_config_display ? S_OK : my_resize_buffers(width, height, flags, buffer_count, format);
    if (ret == S_OK) my_device->resize_orig_buffers(width, height);

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
