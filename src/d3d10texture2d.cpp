#include "d3d10texture2d.h"
#include "dxgiswapchain.h"
#include "log.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10Texture2D, ## __VA_ARGS__)

IUNKNOWN_IMPL(MyID3D10Texture2D)

MyID3D10Texture2D::MyID3D10Texture2D(
    ID3D10Texture2D **inner,
    const D3D10_TEXTURE2D_DESC *pDesc,
    MyIDXGISwapChain *sc
) :
    desc(*pDesc),
    orig_width(pDesc->Width),
    orig_height(pDesc->Height),
    sc(sc),
    IUNKNOWN_INIT(*inner)
{
    LOG_MFUN(_,
        LOG_ARG(*inner)
    );
    *inner = this;
}

MyID3D10Texture2D::~MyID3D10Texture2D() {
    LOG_MFUN();
    if (sc) sc->bbs.erase(this);
}

HRESULT STDMETHODCALLTYPE MyID3D10Texture2D::Map(
    UINT Subresource,
    D3D10_MAP MapType,
    UINT MapFlags,
    D3D10_MAPPED_TEXTURE2D *pMappedTex2D
) {
    LOG_MFUN();
    return inner->Map(Subresource, MapType, MapFlags, pMappedTex2D);
}

void STDMETHODCALLTYPE MyID3D10Texture2D::Unmap(
    UINT Subresource
) {
    LOG_MFUN();
    inner->Unmap(Subresource);
}

void STDMETHODCALLTYPE MyID3D10Texture2D::GetDesc(
    D3D10_TEXTURE2D_DESC *pDesc
) {
    LOG_MFUN();
    inner->GetDesc(pDesc);
}

ID3D10RESOURCE_IMPL(MyID3D10Texture2D, D3D10_RESOURCE_DIMENSION_TEXTURE2D)
