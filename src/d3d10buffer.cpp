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
    char *buffer = NULL;
    void *mapped = NULL;
    D3D10_BUFFER_DESC desc = {};

    Impl(
        ID3D10Buffer **inner,
        const D3D10_BUFFER_DESC *pDesc,
        UINT64 id
    ) :
        IUNKNOWN_INIT(*inner),
        ID3D10RESOURCE_INIT(id),
        desc(*pDesc)
    {
        buffer =
            desc.BindFlags == D3D10_BIND_CONSTANT_BUFFER ?
                new char[desc.ByteWidth] :
                NULL;
    }

    ~Impl() {
        if (buffer) delete buffer;
    }

    HRESULT Map(
        D3D10_MAP MapType,
        UINT MapFlags,
        void **ppData
    ) {
        HRESULT ret = inner->Map(MapType, MapFlags, &mapped);
        if (ret == S_OK) {
            if (!LOG_STARTED || !buffer) {
                *ppData = mapped;
                goto e_ret;
            }

            map_type = MapType;
            bool read = false;
            switch (map_type) {
                case D3D10_MAP_WRITE_DISCARD:
                    memset(buffer, 0, desc.ByteWidth);
                    *ppData = buffer;
                    break;

                case D3D10_MAP_READ_WRITE:
                case D3D10_MAP_READ:
                    memcpy(buffer, mapped, desc.ByteWidth);
                    read = true;
                    *ppData = buffer;
                    break;

                case D3D10_MAP_WRITE:
                case D3D10_MAP_WRITE_NO_OVERWRITE:
                default:
                    *ppData = mapped;
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
            map_type = D3D10_MAP_WRITE_NO_OVERWRITE;
        }
        return ret;
    }

    void Unmap(
    ) {
        switch (map_type) {
            case D3D10_MAP_WRITE_DISCARD:
            case D3D10_MAP_READ_WRITE:
                if (buffer) {
                    memcpy(mapped, buffer, desc.ByteWidth);
                    LOG_MFUN(_,
                        LOG_ARG_TYPE(buffer, ByteArrayLogger, desc.ByteWidth)
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
};

MyID3D10Buffer::MyID3D10Buffer(
    ID3D10Buffer **inner,
    const D3D10_BUFFER_DESC *pDesc,
    UINT64 id
) :
    impl(new Impl(inner, pDesc, id))
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
