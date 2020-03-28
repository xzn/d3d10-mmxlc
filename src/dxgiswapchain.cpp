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
#include "unknown_impl.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyIDXGISwapChain, ## __VA_ARGS__)

extern UINT64 xorshift128p();

class MyIDXGISwapChain::Impl {
    friend class MyIDXGISwapChain;

    IUNKNOWN_PRIV(IDXGISwapChain)

    UINT cached_width = 0;
    UINT cached_height = 0;
    UINT cached_flags = 0;
    UINT cached_buffer_count = 0;
    DXGI_FORMAT cached_format = DXGI_FORMAT_UNKNOWN;
    bool is_config_display = false;
    UINT render_3d_width = 0;
    UINT render_3d_height = 0;
    UINT display_width = 0;
    UINT display_height = 0;
    UINT display_flags = 0;
    UINT display_buffer_count = 0;
    DXGI_FORMAT display_format = DXGI_FORMAT_UNKNOWN;
    bool render_3d_updated = false;
    bool display_updated = false;
    OverlayPtr overlay = {NULL};
    Config *config = NULL;
    MyID3D10Device *device = NULL;
    std::unordered_set<MyID3D10Texture2D *> bbs;
    DXGI_SWAP_CHAIN_DESC desc = {};

    HRESULT my_resize_buffers(
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
            inner->GetDesc(&desc);
            device->resize_buffers(width, height);
        }
        return ret;
    }

    Impl(
        DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
        IDXGISwapChain **inner,
        ID3D10Device **device
    ) :
        IUNKNOWN_INIT(*inner),
        cached_width(pSwapChainDesc->BufferDesc.Width),
        cached_height(pSwapChainDesc->BufferDesc.Height),
        cached_flags(pSwapChainDesc->Flags),
        cached_buffer_count(pSwapChainDesc->BufferCount),
        cached_format(pSwapChainDesc->BufferDesc.Format),
        device(new MyID3D10Device(
            device,
            cached_width,
            cached_height
        )),
        desc(*pSwapChainDesc)
    {
        this->device->AddRef();
    }

    ~Impl() {
        for (auto b : bbs) { b->get_sc() = NULL; }
        if (overlay) overlay->set_display(NULL, NULL, NULL);
        if (device) device->Release();
    }

    void update_config() {
if constexpr (ENABLE_CUSTOM_RESOLUTION) {
        if (config && config->render_display_updated) {
            config->begin_config();
            if (
                render_3d_width != config->render_3d_width ||
                render_3d_height != config->render_3d_height
            ) {
                render_3d_width = config->render_3d_width;
                render_3d_height = config->render_3d_height;
                render_3d_updated = true;
            }
            if (
                display_width != config->display_width ||
                display_height != config->display_height
            ) {
                display_width = config->display_width;
                display_height = config->display_height;
                display_updated = true;
            }
            config->render_display_updated = false;
            config->end_config();
        }

        if (
            (display_updated || is_config_display) &&
            (
                display_flags != cached_flags ||
                display_buffer_count != cached_buffer_count ||
                display_format != cached_format
            )
        ) {
            display_flags = cached_flags;
            display_buffer_count = cached_buffer_count;
            display_format = cached_format;
            display_updated = true;
        }

        if (display_updated) {
            device->get_inner()->OMSetRenderTargets(0, NULL, NULL);

            for (auto bb : bbs) {
                for (MyID3D10RenderTargetView *rtv : bb->get_rtvs()) {
                    if (auto &v = rtv->get_inner()) {
                        cached_rtvs_map.erase(v);
                        v->Release();
                        v = NULL;
                    }
                }
                for (MyID3D10ShaderResourceView *srv : bb->get_srvs()) {
                    if (auto &v = srv->get_inner()) {
                        cached_srvs_map.erase(v);
                        v->Release();
                        v = NULL;
                    }
                }
                for (MyID3D10DepthStencilView *dsv : bb->get_dsvs()) {
                    if (auto &v = dsv->get_inner()) {
                        cached_dsvs_map.erase(v);
                        v->Release();
                        v = NULL;
                    }
                }
                if (auto &b = bb->get_inner()) {
                    b->Release();
                    b = NULL;
                }
            }

            if (display_width && display_height) {
                HRESULT ret = my_resize_buffers(
                    display_width,
                    display_height,
                    display_flags,
                    display_buffer_count,
                    display_format
                );
                if (ret == S_OK) {
                    is_config_display = true;
                    overlay(
                        "Display resolution set to ",
                        std::to_string(display_width),
                        "x",
                        std::to_string(display_height)
                    );
                } else {
                    overlay(
                        "Failed to set display resolution to ",
                        std::to_string(display_width),
                        "x",
                        std::to_string(display_height)
                    );
                }
            } else {
                my_resize_buffers(
                    cached_width,
                    cached_height,
                    cached_flags,
                    cached_buffer_count,
                    cached_format
                );
                overlay(
                    "Restoring display resolution to ",
                    std::to_string(cached_width),
                    "x",
                    std::to_string(cached_height)
                );
                is_config_display = false;
            }

            for (auto bb : bbs) {
                auto &b = bb->get_inner();
                auto d = device->get_inner();
                inner->GetBuffer(
                    0,
                    IID_ID3D10Texture2D,
                    (void **)&b
                );
                for (MyID3D10RenderTargetView *rtv : bb->get_rtvs()) {
                    auto &v = rtv->get_inner();
                    d->CreateRenderTargetView(
                        b,
                        &rtv->get_desc(),
                        &v
                    );
                    cached_rtvs_map.emplace(v, rtv);
                }
                for (MyID3D10ShaderResourceView *srv : bb->get_srvs()) {
                    auto &v = srv->get_inner();
                    d->CreateShaderResourceView(
                        b,
                        &srv->get_desc(),
                        &v
                    );
                    cached_srvs_map.emplace(v, srv);
                }
                for (MyID3D10DepthStencilView *dsv : bb->get_dsvs()) {
                    auto &v = dsv->get_inner();
                    d->CreateDepthStencilView(
                        b,
                        &dsv->get_desc(),
                        &v
                    );
                    cached_dsvs_map.emplace(v, dsv);
                }
                b->GetDesc(&bb->get_desc());
            }

            display_updated = false;
        }

        if (render_3d_updated) {
            device->resize_render_3d(render_3d_width, render_3d_height);
            render_3d_updated = false;
        }
}
    }

    void set_overlay(MyIDXGISwapChain *sc, Overlay *overlay) {
        this->overlay = {overlay};
        if (device) device->set_overlay(overlay);
        if (overlay) {
            if (display_width && display_height) {
                auto desc = this->desc;
                desc.BufferDesc.Width = display_width;
                desc.BufferDesc.Height = display_height;
                overlay->set_display(&desc, sc, device);
            } else {
                overlay->set_display(&desc, sc, device);
            }
        }
    }

    void set_config(Config *config) {
        this->config = config;
        update_config();
        if (device) device->set_config(config);
    }
};

IUNKNOWN_IMPL(MyIDXGISwapChain, IDXGISwapChain)

void MyIDXGISwapChain::set_overlay(Overlay *overlay) {
    impl->set_overlay(this, overlay);
}

void MyIDXGISwapChain::set_config(Config *config) {
    impl->set_config(config);
}

std::unordered_set<MyID3D10Texture2D *> &MyIDXGISwapChain::get_bbs() {
    return impl->bbs;
}

MyIDXGISwapChain::MyIDXGISwapChain(
    DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
    IDXGISwapChain **inner,
    ID3D10Device **device
) :
    impl(new Impl(pSwapChainDesc, inner, device))
{
    LOG_MFUN(_,
        LOG_ARG(*inner),
        LOG_ARG(*device)
    );
    *inner = this;
}

MyIDXGISwapChain::~MyIDXGISwapChain() {
    LOG_MFUN();
    delete impl;
}

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::Present(
    UINT sync_interval,
    UINT flags
) {
    HRESULT ret = 0;
    impl->device->present();
    if (impl->overlay) {
        ret = impl->overlay->present(
            sync_interval,
            flags
        );
    } else {
        ret = impl->inner->Present(
            sync_interval,
            flags
        );
    }
    LOG_MFUN(_, ret);
    LOGGER->next_frame();
    impl->update_config();
    return ret;
}

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::GetBuffer(
    UINT buffer_idx,
    REFIID riid,
    void **surface
) {
    if (riid != IID_ID3D10Texture2D) return E_NOTIMPL;
    HRESULT ret = impl->inner->GetBuffer(
        buffer_idx,
        riid,
        surface
    );
    if (ret == S_OK) {
        LOG_MFUN(_,
            LOG_ARG(buffer_idx),
            LOG_ARG(riid),
            LOG_ARG(*surface),
            ret
        );
        D3D10_TEXTURE2D_DESC desc;
        ((ID3D10Texture2D *)*surface)->GetDesc(&desc);
        D3D10_TEXTURE2D_DESC desc_orig = {
            .Width = impl->cached_width,
            .Height = impl->cached_height
        };
        auto bb = new MyID3D10Texture2D(
            (ID3D10Texture2D **)surface,
            &desc_orig,
            xorshift128p(),
            this
        );
        bb->get_desc() = desc;
        impl->bbs.insert(bb);
    } else {
        LOG_MFUN(_,
            LOG_ARG(buffer_idx),
            LOG_ARG(riid),
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
    return impl->inner->SetFullscreenState(
        fullscreen,
        target
    );
}

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::GetFullscreenState(
    WINBOOL *fullscreen,
    IDXGIOutput **target
) {
    LOG_MFUN();
    return impl->inner->GetFullscreenState(
        fullscreen,
        target
    );
}

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::GetDesc(
    DXGI_SWAP_CHAIN_DESC *desc
) {
    LOG_MFUN();
    if (desc) {
        *desc = impl->desc;
        desc->BufferDesc.Width = impl->cached_width;
        desc->BufferDesc.Height = impl->cached_height;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::ResizeBuffers(
    UINT buffer_count,
    UINT width,
    UINT height,
    DXGI_FORMAT format,
    UINT flags
) {
    impl->cached_width = width;
    impl->cached_height = height;
    impl->cached_flags = flags;
    impl->cached_buffer_count = buffer_count;
    impl->cached_format = format;
    impl->update_config();
    HRESULT ret = impl->is_config_display ?
        S_OK :
        impl->my_resize_buffers(width, height, flags, buffer_count, format);
    if (ret == S_OK) { impl->device->resize_orig_buffers(width, height); }
    LOG_MFUN(_,
        LOG_ARG(buffer_count),
        LOG_ARG(width),
        LOG_ARG(height),
        LOG_ARG(format),
        LOG_ARG(flags),
        ret
    );

    return ret;
}

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::ResizeTarget(
    const DXGI_MODE_DESC *target_mode_desc
) {
    LOG_MFUN();
    return impl->inner->ResizeTarget(target_mode_desc);
}

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::GetContainingOutput(
    IDXGIOutput **output
) {
    LOG_MFUN();
    return impl->inner->GetContainingOutput(output);
}

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::GetFrameStatistics(
    DXGI_FRAME_STATISTICS *stats
) {
    LOG_MFUN();
    return impl->inner->GetFrameStatistics(stats);
}

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::GetLastPresentCount(
    UINT *last_present_count
) {
    LOG_MFUN();
    return impl->inner->GetLastPresentCount(last_present_count);
}

// IDXGIDeviceSubObject

HRESULT STDMETHODCALLTYPE MyIDXGISwapChain::GetDevice(
    REFIID riid,
    void **device
) {
    LOG_MFUN();
    return impl->inner->GetDevice(
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
    return impl->inner->SetPrivateData(
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
    return impl->inner->SetPrivateDataInterface(
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
    return impl->inner->GetPrivateData(
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
    return impl->inner->GetParent(
        riid,
        parent
    );
}
