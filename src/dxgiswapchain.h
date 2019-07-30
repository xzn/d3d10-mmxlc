#ifndef DXGISWAPCHAIN_H
#define DXGISWAPCHAIN_H

#include "main.h"
#include "unknown.h"

class MyID3D10Texture2D;
class Overlay;
class Config;

class MyIDXGISwapChain : public IDXGISwapChain {
    class Impl;
    Impl *impl;

public:
    MyIDXGISwapChain(
        DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
        IDXGISwapChain **inner,
        ID3D10Device **device
    );

    virtual ~MyIDXGISwapChain();

    void set_overlay(Overlay *overlay);
    void set_config(Config *config);

    std::unordered_set<MyID3D10Texture2D *> &get_bbs();

    IUNKNOWN_DECL(IDXGISwapChain)

    // IDXGISwapChain

    virtual HRESULT STDMETHODCALLTYPE Present(
        UINT sync_interval,
        UINT flags
    );

    virtual HRESULT STDMETHODCALLTYPE GetBuffer(
        UINT buffer_idx,
        REFIID riid,
        void **surface
    );

    virtual HRESULT STDMETHODCALLTYPE SetFullscreenState(
        WINBOOL fullscreen,
        IDXGIOutput *target
    );

    virtual HRESULT STDMETHODCALLTYPE GetFullscreenState(
        WINBOOL *fullscreen,
        IDXGIOutput **target
    );

    virtual HRESULT STDMETHODCALLTYPE GetDesc(
        DXGI_SWAP_CHAIN_DESC *desc
    );

    virtual HRESULT STDMETHODCALLTYPE ResizeBuffers(
        UINT buffer_count,
        UINT width,
        UINT height,
        DXGI_FORMAT format,
        UINT flags
    );

    virtual HRESULT STDMETHODCALLTYPE ResizeTarget(
        const DXGI_MODE_DESC *target_mode_desc
    );

    virtual HRESULT STDMETHODCALLTYPE GetContainingOutput(
        IDXGIOutput **output
    );

    virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics(
        DXGI_FRAME_STATISTICS *stats
    );

    virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount(
        UINT *last_present_count
    );

    // IDXGIDeviceSubObject

    virtual HRESULT STDMETHODCALLTYPE GetDevice(
        REFIID riid,
        void **device
    );

    // IDXGIObject

    virtual HRESULT STDMETHODCALLTYPE SetPrivateData(
        REFGUID guid,
        UINT data_size,
        const void *data
    );

    virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(
        REFGUID guid,
        const IUnknown *object
    );

    virtual HRESULT STDMETHODCALLTYPE GetPrivateData(
        REFGUID guid,
        UINT *data_size,
        void *data
    );

    virtual HRESULT STDMETHODCALLTYPE GetParent(
        REFIID riid,
        void **parent
    );
};

#endif
