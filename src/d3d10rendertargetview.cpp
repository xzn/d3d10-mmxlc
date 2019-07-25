#include "d3d10rendertargetview.h"
#include "d3d10texture2d.h"
#include "log.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10RenderTargetView, ## __VA_ARGS__)

IUNKNOWN_IMPL(MyID3D10RenderTargetView)

MyID3D10RenderTargetView::MyID3D10RenderTargetView(
    ID3D10RenderTargetView **inner,
    const D3D10_RENDER_TARGET_VIEW_DESC *pDesc,
    ID3D10Resource *resource
) :
    desc(*pDesc),
    resource(resource),
    IUNKNOWN_INIT(*inner)
{
    LOG_MFUN(_,
        LOG_ARG(*inner)
    );
    cached_rtvs_map.emplace(*inner, this);
    *inner = this;
    resource->AddRef();
}

MyID3D10RenderTargetView::~MyID3D10RenderTargetView() {
    LOG_MFUN();
    cached_rtvs_map.erase(inner);
    if (desc.ViewDimension == D3D10_RTV_DIMENSION_TEXTURE2D) {
        MyID3D10Texture2D *texture_2d = (MyID3D10Texture2D *)resource;
        texture_2d->rtvs.erase(this);
    }
    resource->Release();
}

void STDMETHODCALLTYPE MyID3D10RenderTargetView::GetDesc(
    D3D10_RENDER_TARGET_VIEW_DESC *pDesc
) {
    LOG_MFUN();
    inner->GetDesc(pDesc);
}

ID3D10VIEW_IMPL(MyID3D10RenderTargetView)

std::unordered_map<ID3D10RenderTargetView *, MyID3D10RenderTargetView *> cached_rtvs_map;
