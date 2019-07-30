#include "d3d10shaderresourceview.h"
#include "d3d10texture2d.h"
#include "d3d10view_impl.h"
#include "log.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10ShaderResourceView, ## __VA_ARGS__)

class MyID3D10ShaderResourceView::Impl {
    friend class MyID3D10ShaderResourceView;

    IUNKNOWN_PRIV(ID3D10ShaderResourceView)
    ID3D10VIEW_PRIV
    D3D10_SHADER_RESOURCE_VIEW_DESC desc;

    Impl(
        ID3D10ShaderResourceView **inner,
        const D3D10_SHADER_RESOURCE_VIEW_DESC *pDesc,
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

ID3D10VIEW_IMPL(MyID3D10ShaderResourceView, ID3D10ShaderResourceView)

D3D10_SHADER_RESOURCE_VIEW_DESC &MyID3D10ShaderResourceView::get_desc() {
    return impl->desc;
}

const D3D10_SHADER_RESOURCE_VIEW_DESC &MyID3D10ShaderResourceView::get_desc() const {
    return impl->desc;
}

MyID3D10ShaderResourceView::MyID3D10ShaderResourceView(
    ID3D10ShaderResourceView **inner,
    const D3D10_SHADER_RESOURCE_VIEW_DESC *pDesc,
    ID3D10Resource *resource
) :
    impl(new Impl(inner, pDesc, resource))
{
    LOG_MFUN(_,
        LOG_ARG(*inner)
    );
    cached_srvs_map.emplace(*inner, this);
    *inner = this;
}

MyID3D10ShaderResourceView::~MyID3D10ShaderResourceView() {
    LOG_MFUN();
    cached_srvs_map.erase(impl->inner);
    if (impl->desc.ViewDimension == D3D10_SRV_DIMENSION_TEXTURE2D) {
        MyID3D10Texture2D *tex = (MyID3D10Texture2D *)impl->resource;
        tex->get_srvs().erase(this);
    }
    delete impl;
}

void STDMETHODCALLTYPE MyID3D10ShaderResourceView::GetDesc(
    D3D10_SHADER_RESOURCE_VIEW_DESC *pDesc
) {
    LOG_MFUN();
    if (pDesc) *pDesc = impl->desc;
}

std::unordered_map<ID3D10ShaderResourceView *, MyID3D10ShaderResourceView *> cached_srvs_map;
