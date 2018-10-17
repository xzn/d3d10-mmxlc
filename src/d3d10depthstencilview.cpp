#include "d3d10depthstencilview.h"
#include "d3d10texture2d.h"
#include "log.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10DepthStencilView, ## __VA_ARGS__)

IUNKNOWN_IMPL(MyID3D10DepthStencilView)

MyID3D10DepthStencilView::MyID3D10DepthStencilView(
    ID3D10DepthStencilView **inner,
    const D3D10_DEPTH_STENCIL_VIEW_DESC *pDesc,
    ID3D10Resource *resource
) :
    desc(*pDesc),
    resource(resource),
    IUNKNOWN_INIT(*inner)
{
    LOG_MFUN(_,
        LOG_ARG(*inner)
    );
    current_dsvs_map.emplace(*inner, this);
    *inner = this;
    resource->AddRef();
}

MyID3D10DepthStencilView::~MyID3D10DepthStencilView() {
    LOG_MFUN();
    current_dsvs_map.erase(inner);
    if (desc.ViewDimension == D3D10_DSV_DIMENSION_TEXTURE2D) {
        MyID3D10Texture2D *texture_2d = (MyID3D10Texture2D *)resource;
        texture_2d->dsvs.erase(this);
    }
    resource->Release();
}

void STDMETHODCALLTYPE MyID3D10DepthStencilView::GetDesc(
    D3D10_DEPTH_STENCIL_VIEW_DESC *pDesc
) {
    LOG_MFUN();
    inner->GetDesc(pDesc);
}

ID3D10VIEW_IMPL(MyID3D10DepthStencilView)

std::unordered_map<ID3D10DepthStencilView *, MyID3D10DepthStencilView *> current_dsvs_map;
