#include "d3d10depthstencilview.h"
#include "d3d10texture2d.h"
#include "d3d10view_impl.h"
#include "log.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10DepthStencilView, ## __VA_ARGS__)

class MyID3D10DepthStencilView::Impl {
    friend class MyID3D10DepthStencilView;

    IUNKNOWN_PRIV(ID3D10DepthStencilView)
    ID3D10VIEW_PRIV
    D3D10_DEPTH_STENCIL_VIEW_DESC desc = {};

    Impl(
        ID3D10DepthStencilView **inner,
        const D3D10_DEPTH_STENCIL_VIEW_DESC *pDesc,
        ID3D10Resource *resource
    ) :
        IUNKNOWN_INIT(*inner),
        ID3D10VIEW_INIT(resource),
        desc(*pDesc)
    {
        resource->AddRef();
    }

    ~Impl() {
        resource->Release();
    }
};

ID3D10VIEW_IMPL(MyID3D10DepthStencilView, ID3D10DepthStencilView)

D3D10_DEPTH_STENCIL_VIEW_DESC &MyID3D10DepthStencilView::get_desc() {
    return impl->desc;
}

const D3D10_DEPTH_STENCIL_VIEW_DESC &MyID3D10DepthStencilView::get_desc() const {
    return impl->desc;
}

MyID3D10DepthStencilView::MyID3D10DepthStencilView(
    ID3D10DepthStencilView **inner,
    const D3D10_DEPTH_STENCIL_VIEW_DESC *pDesc,
    ID3D10Resource *resource
) :
    impl(new Impl(inner, pDesc, resource))
{
    LOG_MFUN(_,
        LOG_ARG(*inner)
    );
    cached_dsvs_map.emplace(*inner, this);
    *inner = this;
}

MyID3D10DepthStencilView::~MyID3D10DepthStencilView() {
    LOG_MFUN();
    cached_dsvs_map.erase(impl->inner);
    if (impl->desc.ViewDimension == D3D10_DSV_DIMENSION_TEXTURE2D) {
        MyID3D10Texture2D *tex = (MyID3D10Texture2D *)impl->resource;
        tex->get_dsvs().erase(this);
    }
    delete impl;
}

void STDMETHODCALLTYPE MyID3D10DepthStencilView::GetDesc(
    D3D10_DEPTH_STENCIL_VIEW_DESC *pDesc
) {
    LOG_MFUN();
    if (pDesc) *pDesc = impl->desc;
}

std::unordered_map<ID3D10DepthStencilView *, MyID3D10DepthStencilView *> cached_dsvs_map;
