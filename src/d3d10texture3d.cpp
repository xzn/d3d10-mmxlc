#include "d3d10texture3d.h"
#include "d3d10resource_impl.h"
#include "log.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10Texture3D, ## __VA_ARGS__)

class MyID3D10Texture3D::Impl {
    friend class MyID3D10Texture3D;

    IUNKNOWN_PRIV(ID3D10Texture3D)
    ID3D10RESOURCE_PRIV
    D3D10_TEXTURE3D_DESC desc = {};

    Impl(
        ID3D10Texture3D **inner,
        const D3D10_TEXTURE3D_DESC *pDesc,
        UINT64 id
    ) :
        IUNKNOWN_INIT(*inner),
        ID3D10RESOURCE_INIT(id),
        desc(*pDesc)
    {}

    ~Impl() {}
};

MyID3D10Texture3D::MyID3D10Texture3D(
    ID3D10Texture3D **inner,
    const D3D10_TEXTURE3D_DESC *pDesc,
    UINT64 id
) :
    impl(new Impl(inner, pDesc, id))
{
    LOG_MFUN(_,
        LOG_ARG(*inner),
        LOG_ARG_TYPE(id, NumHexLogger)
    );
    *inner = this;
}

MyID3D10Texture3D::~MyID3D10Texture3D() {
    LOG_MFUN();
    delete impl;
}

ID3D10RESOURCE_IMPL(MyID3D10Texture3D, ID3D10Texture3D, D3D10_RESOURCE_DIMENSION_TEXTURE3D)

D3D10_TEXTURE3D_DESC &MyID3D10Texture3D::get_desc() {
    return impl->desc;
}

const D3D10_TEXTURE3D_DESC &MyID3D10Texture3D::get_desc() const {
    return impl->desc;
}

HRESULT STDMETHODCALLTYPE MyID3D10Texture3D::Map(
    UINT Subresource,
    D3D10_MAP MapType,
    UINT MapFlags,
    D3D10_MAPPED_TEXTURE3D *pMappedTex3D
) {
    LOG_MFUN();
    return impl->inner->Map(Subresource, MapType, MapFlags, pMappedTex3D);
}

void STDMETHODCALLTYPE MyID3D10Texture3D::Unmap(
    UINT Subresource
) {
    LOG_MFUN();
    impl->inner->Unmap(Subresource);
}

void STDMETHODCALLTYPE MyID3D10Texture3D::GetDesc(
    D3D10_TEXTURE3D_DESC *pDesc
) {
    LOG_MFUN();
    if (pDesc) *pDesc = impl->desc;
}
