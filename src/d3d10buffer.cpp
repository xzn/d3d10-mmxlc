#include "d3d10buffer.h"
#include "log.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10Buffer, ## __VA_ARGS__)

IUNKNOWN_IMPL(MyID3D10Buffer)

MyID3D10Buffer::MyID3D10Buffer(
    ID3D10Buffer **inner,
    const D3D10_BUFFER_DESC *pDesc
) :
    desc(*pDesc),
    IUNKNOWN_INIT(*inner)
{
    LOG_MFUN(_,
        LOG_ARG(*inner)
    );
    current_bs_map.emplace(*inner, this);
    *inner = this;
}

MyID3D10Buffer::~MyID3D10Buffer() {
    LOG_MFUN();
    current_bs_map.erase(inner);
}

HRESULT STDMETHODCALLTYPE MyID3D10Buffer::Map(
    D3D10_MAP MapType,
    UINT MapFlags,
    void **ppData
) {
    LOG_MFUN();
    return inner->Map(MapType, MapFlags, ppData);
}

void STDMETHODCALLTYPE MyID3D10Buffer::Unmap(
) {
    LOG_MFUN();
    inner->Unmap();
}

void STDMETHODCALLTYPE MyID3D10Buffer::GetDesc(
    D3D10_BUFFER_DESC *pDesc
) {
    LOG_MFUN();
    inner->GetDesc(pDesc);
}

ID3D10RESOURCE_IMPL(MyID3D10Buffer, D3D10_RESOURCE_DIMENSION_BUFFER)

std::unordered_map<ID3D10Buffer *, MyID3D10Buffer *> current_bs_map;
