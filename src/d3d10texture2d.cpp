#include "d3d10texture2d.h"
#include "dxgiswapchain.h"
#include "d3d10resource_impl.h"
#include "log.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10Texture2D, ## __VA_ARGS__)

class MyID3D10Texture2D::Impl {
    friend class MyID3D10Texture2D;

    IUNKNOWN_PRIV(ID3D10Texture2D)
    ID3D10RESOURCE_PRIV
    D3D10_TEXTURE2D_DESC desc = {};
    UINT orig_width = 0;
    UINT orig_height = 0;
    std::unordered_set<MyID3D10RenderTargetView *> rtvs;
    std::unordered_set<MyID3D10ShaderResourceView *> srvs;
    std::unordered_set<MyID3D10DepthStencilView *> dsvs;
    MyIDXGISwapChain *sc = NULL;

    Impl(
        ID3D10Texture2D **inner,
        const D3D10_TEXTURE2D_DESC *pDesc,
        UINT64 id,
        MyIDXGISwapChain *sc
    ) :
        IUNKNOWN_INIT(*inner),
        ID3D10RESOURCE_INIT(id),
        desc(*pDesc),
        orig_width(pDesc->Width),
        orig_height(pDesc->Height),
        sc(sc)
    {}

    ~Impl() {}
};

ID3D10RESOURCE_IMPL(MyID3D10Texture2D, ID3D10Texture2D, D3D10_RESOURCE_DIMENSION_TEXTURE2D)

D3D10_TEXTURE2D_DESC &MyID3D10Texture2D::get_desc() {
    return impl->desc;
}

const D3D10_TEXTURE2D_DESC &MyID3D10Texture2D::get_desc() const {
    return impl->desc;
}

UINT &MyID3D10Texture2D::get_orig_width() {
    return impl->orig_width;
}

UINT &MyID3D10Texture2D::get_orig_height() {
    return impl->orig_height;
}

std::unordered_set<MyID3D10RenderTargetView *> &MyID3D10Texture2D::get_rtvs() {
    return impl->rtvs;
}

std::unordered_set<MyID3D10ShaderResourceView *> &MyID3D10Texture2D::get_srvs() {
    return impl->srvs;
}

std::unordered_set<MyID3D10DepthStencilView *> &MyID3D10Texture2D::get_dsvs() {
    return impl->dsvs;
}

MyIDXGISwapChain *&MyID3D10Texture2D::get_sc() {
    return impl->sc;
}

MyID3D10Texture2D::MyID3D10Texture2D(
    ID3D10Texture2D **inner,
    const D3D10_TEXTURE2D_DESC *pDesc,
    UINT64 id,
    MyIDXGISwapChain *sc
) :
    impl(new Impl(inner, pDesc, id, sc))
{
    LOG_MFUN(_,
        LOG_ARG(*inner),
        LOG_ARG_TYPE(id, NumHexLogger)
    );
    *inner = this;
}

MyID3D10Texture2D::~MyID3D10Texture2D() {
    LOG_MFUN();
    if (auto sc = impl->sc) { sc->get_bbs().erase(this); }
    delete impl;
}

HRESULT STDMETHODCALLTYPE MyID3D10Texture2D::Map(
    UINT Subresource,
    D3D10_MAP MapType,
    UINT MapFlags,
    D3D10_MAPPED_TEXTURE2D *pMappedTex2D
) {
    LOG_MFUN();
    return impl->inner->Map(Subresource, MapType, MapFlags, pMappedTex2D);
}

void STDMETHODCALLTYPE MyID3D10Texture2D::Unmap(
    UINT Subresource
) {
    LOG_MFUN();
    impl->inner->Unmap(Subresource);
}

void STDMETHODCALLTYPE MyID3D10Texture2D::GetDesc(
    D3D10_TEXTURE2D_DESC *pDesc
) {
    LOG_MFUN();
    if (pDesc) {
        *pDesc = impl->desc;
        pDesc->Width = impl->orig_width;
        pDesc->Height = impl->orig_height;
    }
}
