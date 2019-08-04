#include "d3d10depthstencilstate.h"
#include "d3d10devicechild_impl.h"
#include "log.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10DepthStencilState, ## __VA_ARGS__)

class MyID3D10DepthStencilState::Impl {
    friend class MyID3D10DepthStencilState;

    IUNKNOWN_PRIV(ID3D10DepthStencilState)

    D3D10_DEPTH_STENCIL_DESC desc = {};

    Impl(
        ID3D10DepthStencilState **inner,
        const D3D10_DEPTH_STENCIL_DESC *pDesc
    ) :
        IUNKNOWN_INIT(*inner),
        desc(*pDesc)
    {}

    ~Impl() {}
};

ID3D10DEVICECHILD_IMPL(MyID3D10DepthStencilState, ID3D10DepthStencilState)

D3D10_DEPTH_STENCIL_DESC &MyID3D10DepthStencilState::get_desc() {
    return impl->desc;
}

const D3D10_DEPTH_STENCIL_DESC &MyID3D10DepthStencilState::get_desc() const {
    return impl->desc;
}

MyID3D10DepthStencilState::MyID3D10DepthStencilState(
    ID3D10DepthStencilState **inner,
    const D3D10_DEPTH_STENCIL_DESC *pDesc
) :
    impl(new Impl(inner, pDesc))
{
    LOG_MFUN(_,
        LOG_ARG(*inner)
    );
    cached_dsss_map.emplace(*inner, this);
    *inner = this;
}

MyID3D10DepthStencilState::~MyID3D10DepthStencilState() {
    LOG_MFUN();
    cached_dsss_map.erase(impl->inner);
    delete impl;
}

void STDMETHODCALLTYPE MyID3D10DepthStencilState::GetDesc(
    D3D10_DEPTH_STENCIL_DESC *pDesc
) {
    LOG_MFUN();
    if (pDesc) *pDesc = impl->desc;
}

std::unordered_map<ID3D10DepthStencilState *, MyID3D10DepthStencilState *> cached_dsss_map;
