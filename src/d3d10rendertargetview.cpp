#include "d3d10rendertargetview.h"
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
    );current_rtvs_map.emplace(*inner, this);
    *inner = this;
}

MyID3D10RenderTargetView::~MyID3D10RenderTargetView() {
    LOG_MFUN();
    current_rtvs_map.erase(inner);
}

void STDMETHODCALLTYPE MyID3D10RenderTargetView::GetDesc(
    D3D10_RENDER_TARGET_VIEW_DESC *pDesc
) {
    LOG_MFUN();
    inner->GetDesc(pDesc);
}

ID3D10VIEW_IMPL(MyID3D10RenderTargetView)

std::unordered_map<ID3D10RenderTargetView *, MyID3D10RenderTargetView *> current_rtvs_map;
