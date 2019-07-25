#include "d3d10texture3d.h"
#include "log.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10Texture3D, ## __VA_ARGS__)

IUNKNOWN_IMPL(MyID3D10Texture3D)

MyID3D10Texture3D::MyID3D10Texture3D(
    ID3D10Texture3D **inner,
    const D3D10_TEXTURE3D_DESC *pDesc,
    UINT64 id
) :
    desc(*pDesc),
    IUNKNOWN_INIT(*inner),
    id(id)
{
    LOG_MFUN(_,
        LOG_ARG(*inner),
        LOG_ARG_TYPE(id, NumHexLogger)
    );
    *inner = this;
}

MyID3D10Texture3D::~MyID3D10Texture3D() {
    LOG_MFUN();
}

HRESULT STDMETHODCALLTYPE MyID3D10Texture3D::Map(
    UINT Subresource,
    D3D10_MAP MapType,
    UINT MapFlags,
    D3D10_MAPPED_TEXTURE3D *pMappedTex3D
) {
    LOG_MFUN();
    return inner->Map(Subresource, MapType, MapFlags, pMappedTex3D);
}

void STDMETHODCALLTYPE MyID3D10Texture3D::Unmap(
    UINT Subresource
) {
    LOG_MFUN();
    inner->Unmap(Subresource);
}

void STDMETHODCALLTYPE MyID3D10Texture3D::GetDesc(
    D3D10_TEXTURE3D_DESC *pDesc
) {
    LOG_MFUN();
    inner->GetDesc(pDesc);
}

ID3D10RESOURCE_IMPL(MyID3D10Texture3D, D3D10_RESOURCE_DIMENSION_TEXTURE3D)
