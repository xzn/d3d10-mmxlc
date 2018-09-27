#include "d3d10texture1d.h"
#include "log.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10Texture1D, ## __VA_ARGS__)

IUNKNOWN_IMPL(MyID3D10Texture1D)

MyID3D10Texture1D::MyID3D10Texture1D(
    ID3D10Texture1D **inner,
    const D3D10_TEXTURE1D_DESC *pDesc
) :
    desc(*pDesc),
    IUNKNOWN_INIT(*inner)
{
    LOG_MFUN(_,
        LOG_ARG(*inner)
    );
    *inner = this;
}

MyID3D10Texture1D::~MyID3D10Texture1D() {
    LOG_MFUN();
}

HRESULT STDMETHODCALLTYPE MyID3D10Texture1D::Map(
    UINT Subresource,
    D3D10_MAP MapType,
    UINT MapFlags,
    void **ppData
) {
    LOG_MFUN();
    return inner->Map(Subresource, MapType, MapFlags, ppData);
}

void STDMETHODCALLTYPE MyID3D10Texture1D::Unmap(
    UINT Subresource
) {
    LOG_MFUN();
    inner->Unmap(Subresource);
}

void STDMETHODCALLTYPE MyID3D10Texture1D::GetDesc(
    D3D10_TEXTURE1D_DESC *pDesc
) {
    LOG_MFUN();
    inner->GetDesc(pDesc);
}

ID3D10RESOURCE_IMPL(MyID3D10Texture1D, D3D10_RESOURCE_DIMENSION_TEXTURE1D)
