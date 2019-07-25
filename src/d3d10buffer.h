#ifndef D3D10BUFFER_H
#define D3D10BUFFER_H

#include "main.h"
#include "unknown.h"
#include "d3d10resource.h"

class MyID3D10Buffer : public ID3D10Buffer {
    D3D10_MAP MapType;
    char *unmap_data;
    void *mapped_data;

public:
    D3D10_BUFFER_DESC desc;

    MyID3D10Buffer(
        ID3D10Buffer **inner,
        const D3D10_BUFFER_DESC *pDesc,
        UINT64 id
    );

    virtual ~MyID3D10Buffer();

    IUNKNOWN_DECL(MyID3D10Buffer, ID3D10Buffer)

    virtual HRESULT STDMETHODCALLTYPE Map(
        D3D10_MAP MapType,
        UINT MapFlags,
        void **ppData
    );

    virtual void STDMETHODCALLTYPE Unmap(
    );

    virtual void STDMETHODCALLTYPE GetDesc(
        D3D10_BUFFER_DESC *pDesc
    );

    ID3D10RESOURCE_DECL
};

extern std::unordered_map<ID3D10Buffer *, MyID3D10Buffer *> cached_bs_map;

#endif
