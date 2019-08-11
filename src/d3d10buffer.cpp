#include "d3d10buffer.h"
#include "d3d10resource_impl.h"
#include "log.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10Buffer, ## __VA_ARGS__)

class MyID3D10Buffer::Impl {
    friend class MyID3D10Buffer;

    IUNKNOWN_PRIV(ID3D10Buffer)
    ID3D10RESOURCE_PRIV

    D3D10_MAP map_type = (D3D10_MAP)0;
    void *mapped = NULL; // result of map
    char *cached = NULL; // local cache
    bool cached_state = false;
    D3D10_BUFFER_DESC desc = {};

    Impl(
        ID3D10Buffer **inner,
        const D3D10_BUFFER_DESC *pDesc,
        UINT64 id,
        const D3D10_SUBRESOURCE_DATA *pInitialData
    ) :
        IUNKNOWN_INIT(*inner),
        ID3D10RESOURCE_INIT(id),
        desc(*pDesc)
    {
        switch (desc.BindFlags) {
            case D3D10_BIND_CONSTANT_BUFFER:
            case D3D10_BIND_INDEX_BUFFER:
            case D3D10_BIND_VERTEX_BUFFER:
                cached = new char[desc.ByteWidth]{};
                if (pInitialData && pInitialData->pSysMem)
                    memcpy(
                        cached,
                        pInitialData->pSysMem,
                        desc.ByteWidth
                    );
                cached_state = true;
                break;

            default:
                break;
        }
    }

    ~Impl() {
        if (cached) delete[] cached;
    }

    HRESULT Map(
        D3D10_MAP MapType,
        UINT MapFlags,
        void **ppData
    ) {
        HRESULT ret = inner->Map(MapType, MapFlags, &mapped);
        if (ret == S_OK) {
            map_type = MapType;
            if (!LOG_STARTED || !cached) {
                *ppData = mapped;
                mapped = NULL;
            } else {
                switch (map_type) {
                    case D3D10_MAP_WRITE_DISCARD:
                        memset(cached, 0, desc.ByteWidth);
                        *ppData = cached;
                        break;

                    case D3D10_MAP_READ_WRITE:
                    case D3D10_MAP_READ:
                        memcpy(cached, mapped, desc.ByteWidth);
                        *ppData = cached;
                        break;

                    case D3D10_MAP_WRITE:
                    case D3D10_MAP_WRITE_NO_OVERWRITE:
                    default:
                        *ppData = mapped;
                        cached_state = false;
                        mapped = NULL;
                        break;
                }
            }
        }
        LOG_MFUN(_,
            LOG_ARG(MapType),
            LOG_ARG_TYPE(MapFlags, D3D10_MAP_FLAG),
            ret
        );
        return ret;
    }

    void Unmap(
    ) {
        LOG_MFUN();
        switch (map_type) {
            case D3D10_MAP_WRITE_DISCARD:
            case D3D10_MAP_READ_WRITE:
                if (cached && mapped) {
                    memcpy(mapped, cached, desc.ByteWidth);
                    cached_state = true;
                    break;
                }
                /* fall-through */

            case D3D10_MAP_READ:
            case D3D10_MAP_WRITE:
            case D3D10_MAP_WRITE_NO_OVERWRITE:
            default:
                mapped = NULL;
                break;
        }
        inner->Unmap();
    }
};

MyID3D10Buffer::MyID3D10Buffer(
    ID3D10Buffer **inner,
    const D3D10_BUFFER_DESC *pDesc,
    UINT64 id,
    const D3D10_SUBRESOURCE_DATA *pInitialData
) :
    impl(new Impl(inner, pDesc, id, pInitialData))
{
    LOG_MFUN(_,
        LOG_ARG(*inner),
        LOG_ARG_TYPE(id, NumHexLogger)
    );
    cached_bs_map.emplace(*inner, this);
    *inner = this;
}

MyID3D10Buffer::~MyID3D10Buffer() {
    LOG_MFUN();
    cached_bs_map.erase(impl->inner);
    delete impl;
}

ID3D10RESOURCE_IMPL(MyID3D10Buffer, ID3D10Buffer, D3D10_RESOURCE_DIMENSION_BUFFER)

D3D10_BUFFER_DESC &MyID3D10Buffer::get_desc() {
    return impl->desc;
}

const D3D10_BUFFER_DESC &MyID3D10Buffer::get_desc() const {
    return impl->desc;
}

char *&MyID3D10Buffer::get_cached() {
    return impl->cached;
}

char *MyID3D10Buffer::get_cached() const {
    return impl->cached;
}

bool &MyID3D10Buffer::get_cached_state() {
    return impl->cached_state;
}

bool MyID3D10Buffer::get_cached_state() const {
    return impl->cached_state;
}

HRESULT STDMETHODCALLTYPE MyID3D10Buffer::Map(
    D3D10_MAP MapType,
    UINT MapFlags,
    void **ppData
) {
    return impl->Map(MapType, MapFlags, ppData);
}

void STDMETHODCALLTYPE MyID3D10Buffer::Unmap(
) {
    impl->Unmap();
}

void STDMETHODCALLTYPE MyID3D10Buffer::GetDesc(
    D3D10_BUFFER_DESC *pDesc
) {
    LOG_MFUN();
    if (pDesc) *pDesc = impl->desc;
}

std::unordered_map<ID3D10Buffer *, MyID3D10Buffer *> cached_bs_map;
