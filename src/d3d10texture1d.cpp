#include "d3d10texture1d.h"
#include "d3d10resource_impl.h"
#include "log.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10Texture1D, ## __VA_ARGS__)

class MyID3D10Texture1D::Impl {
    friend class MyID3D10Texture1D;

    IUNKNOWN_PRIV(ID3D10Texture1D)
    ID3D10RESOURCE_PRIV
    D3D10_TEXTURE1D_DESC desc = {};

    Impl(
        ID3D10Texture1D **inner,
        const D3D10_TEXTURE1D_DESC *pDesc,
        UINT64 id
    ) :
        IUNKNOWN_INIT(*inner),
        ID3D10RESOURCE_INIT(id),
        desc(*pDesc)
    {}
};

ID3D10RESOURCE_IMPL(MyID3D10Texture1D, ID3D10Texture1D, D3D10_RESOURCE_DIMENSION_TEXTURE1D)

D3D10_TEXTURE1D_DESC &MyID3D10Texture1D::get_desc() {
    return impl->desc;
}

const D3D10_TEXTURE1D_DESC &MyID3D10Texture1D::get_desc() const {
    return impl->desc;
}

MyID3D10Texture1D::MyID3D10Texture1D(
    ID3D10Texture1D **inner,
    const D3D10_TEXTURE1D_DESC *pDesc,
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

MyID3D10Texture1D::~MyID3D10Texture1D() {
    LOG_MFUN();
    delete impl;
}

HRESULT STDMETHODCALLTYPE MyID3D10Texture1D::Map(
    UINT Subresource,
    D3D10_MAP MapType,
    UINT MapFlags,
    void **ppData
) {
    LOG_MFUN();
    return impl->inner->Map(Subresource, MapType, MapFlags, ppData);
}

void STDMETHODCALLTYPE MyID3D10Texture1D::Unmap(
    UINT Subresource
) {
    LOG_MFUN();
    impl->inner->Unmap(Subresource);
}

void STDMETHODCALLTYPE MyID3D10Texture1D::GetDesc(
    D3D10_TEXTURE1D_DESC *pDesc
) {
    LOG_MFUN();
    if (pDesc) *pDesc = impl->desc;
}
