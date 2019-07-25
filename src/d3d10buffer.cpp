#include "d3d10buffer.h"
#include "log.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10Buffer, ## __VA_ARGS__)

IUNKNOWN_IMPL(MyID3D10Buffer)

MyID3D10Buffer::MyID3D10Buffer(
    ID3D10Buffer **inner,
    const D3D10_BUFFER_DESC *pDesc,
    UINT64 id
) :
    desc(*pDesc),
    IUNKNOWN_INIT(*inner),
    id(id)
{
    LOG_MFUN(_,
        LOG_ARG(*inner),
        LOG_ARG_TYPE(id, NumHexLogger)
    );
    cached_bs_map.emplace(*inner, this);
    *inner = this;
    unmap_data = desc.BindFlags == D3D10_BIND_CONSTANT_BUFFER ?
        new char[desc.ByteWidth] :
        NULL;
}

MyID3D10Buffer::~MyID3D10Buffer() {
    LOG_MFUN();
    cached_bs_map.erase(inner);
    if (unmap_data) delete unmap_data;
}

HRESULT STDMETHODCALLTYPE MyID3D10Buffer::Map(
    D3D10_MAP MapType,
    UINT MapFlags,
    void **ppData
) {
    HRESULT ret = inner->Map(MapType, MapFlags, &mapped_data);
    if (ret == S_OK) {
        if (!LOG_STARTED || !unmap_data) {
            *ppData = mapped_data;
            goto e_ret;
        }

        this->MapType = MapType;
        bool read = false;
        switch (MapType) {
            case D3D10_MAP_WRITE_DISCARD:
                memset(unmap_data, 0, desc.ByteWidth);
                *ppData = unmap_data;
                break;

            case D3D10_MAP_READ_WRITE:
            case D3D10_MAP_READ:
                memcpy(unmap_data, mapped_data, desc.ByteWidth);
                read = true;
                *ppData = unmap_data;
                break;

            case D3D10_MAP_WRITE:
            case D3D10_MAP_WRITE_NO_OVERWRITE:
            default:
                *ppData = mapped_data;
                break;
        }
        if (!read) goto e_ret;
        LOG_MFUN(_,
            LOG_ARG(MapType),
            LOG_ARG_TYPE(MapFlags, D3D10_MAP_FLAG),
            LOG_ARG_TYPE(*ppData, ByteArrayLogger, desc.ByteWidth),
            ret
        );
    } else {
e_ret:
        LOG_MFUN(_,
            LOG_ARG(MapType),
            LOG_ARG_TYPE(MapFlags, D3D10_MAP_FLAG),
            ret
        );
        this->MapType = D3D10_MAP_WRITE_NO_OVERWRITE;
    }
    return ret;
}

void STDMETHODCALLTYPE MyID3D10Buffer::Unmap(
) {
    switch (MapType) {
        case D3D10_MAP_WRITE_DISCARD:
        case D3D10_MAP_READ_WRITE:
            if (unmap_data) {
                memcpy(mapped_data, unmap_data, desc.ByteWidth);
                LOG_MFUN(_,
                    LOG_ARG_TYPE(unmap_data, ByteArrayLogger, desc.ByteWidth)
                );

                break;
            }
            /* fall-through */

        case D3D10_MAP_READ:
        case D3D10_MAP_WRITE:
        case D3D10_MAP_WRITE_NO_OVERWRITE:
        default:
            LOG_MFUN();
            break;
    }
    inner->Unmap();
}

void STDMETHODCALLTYPE MyID3D10Buffer::GetDesc(
    D3D10_BUFFER_DESC *pDesc
) {
    LOG_MFUN();
    inner->GetDesc(pDesc);
}

ID3D10RESOURCE_IMPL(MyID3D10Buffer, D3D10_RESOURCE_DIMENSION_BUFFER)

std::unordered_map<ID3D10Buffer *, MyID3D10Buffer *> cached_bs_map;
