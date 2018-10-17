#include "d3d10shaderresourceview.h"
#include "d3d10texture2d.h"
#include "log.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10ShaderResourceView, ## __VA_ARGS__)

IUNKNOWN_IMPL(MyID3D10ShaderResourceView)

MyID3D10ShaderResourceView::MyID3D10ShaderResourceView(
    ID3D10ShaderResourceView **inner,
    const D3D10_SHADER_RESOURCE_VIEW_DESC *pDesc,
    ID3D10Resource *resource
) :
    desc(*pDesc),
    resource(resource),
    IUNKNOWN_INIT(*inner)
{
    LOG_MFUN(_,
        LOG_ARG(*inner)
    );
    current_srvs_map.emplace(*inner, this);
    *inner = this;
    resource->AddRef();
}

MyID3D10ShaderResourceView::~MyID3D10ShaderResourceView() {
    LOG_MFUN();
    current_srvs_map.erase(inner);
    if (desc.ViewDimension == D3D10_SRV_DIMENSION_TEXTURE2D) {
        MyID3D10Texture2D *texture_2d = (MyID3D10Texture2D *)resource;
        texture_2d->srvs.erase(this);
    }
    resource->Release();
}

void STDMETHODCALLTYPE MyID3D10ShaderResourceView::GetDesc(
    D3D10_SHADER_RESOURCE_VIEW_DESC *pDesc
) {
    LOG_MFUN();
    inner->GetDesc(pDesc);
}

ID3D10VIEW_IMPL(MyID3D10ShaderResourceView)

std::unordered_map<ID3D10ShaderResourceView *, MyID3D10ShaderResourceView *> current_srvs_map;
