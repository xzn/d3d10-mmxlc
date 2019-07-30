#include "d3d10rendertargetview.h"
#include "d3d10texture2d.h"
#include "d3d10view_impl.h"
#include "log.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10RenderTargetView, ## __VA_ARGS__)

class MyID3D10RenderTargetView::Impl {
    friend class MyID3D10RenderTargetView;

    IUNKNOWN_PRIV(ID3D10RenderTargetView)
    ID3D10VIEW_PRIV
    D3D10_RENDER_TARGET_VIEW_DESC desc;

    Impl(
        ID3D10RenderTargetView **inner,
        const D3D10_RENDER_TARGET_VIEW_DESC *pDesc,
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

ID3D10VIEW_IMPL(MyID3D10RenderTargetView, ID3D10RenderTargetView)

D3D10_RENDER_TARGET_VIEW_DESC &MyID3D10RenderTargetView::get_desc() {
    return impl->desc;
}

const D3D10_RENDER_TARGET_VIEW_DESC &MyID3D10RenderTargetView::get_desc() const {
    return impl->desc;
}

MyID3D10RenderTargetView::MyID3D10RenderTargetView(
    ID3D10RenderTargetView **inner,
    const D3D10_RENDER_TARGET_VIEW_DESC *pDesc,
    ID3D10Resource *resource
) :
    impl(new Impl(inner, pDesc, resource))
{
    LOG_MFUN(_,
        LOG_ARG(*inner)
    );
    cached_rtvs_map.emplace(*inner, this);
    *inner = this;
}

MyID3D10RenderTargetView::~MyID3D10RenderTargetView() {
    LOG_MFUN();
    cached_rtvs_map.erase(impl->inner);
    if (impl->desc.ViewDimension == D3D10_RTV_DIMENSION_TEXTURE2D) {
        MyID3D10Texture2D *tex = (MyID3D10Texture2D *)impl->resource;
        tex->get_rtvs().erase(this);
    }
    delete impl;
}

void STDMETHODCALLTYPE MyID3D10RenderTargetView::GetDesc(
    D3D10_RENDER_TARGET_VIEW_DESC *pDesc
) {
    LOG_MFUN();
    if (pDesc) *pDesc = impl->desc;
}

std::unordered_map<ID3D10RenderTargetView *, MyID3D10RenderTargetView *> cached_rtvs_map;
