#ifndef D3D10BUFFER_H
#define D3D10BUFFER_H

#include "main.h"
#include "d3d10resource.h"

class MyID3D10Buffer : public ID3D10Buffer {
    class Impl;
    Impl *impl;

public:
    MyID3D10Buffer(
        ID3D10Buffer **inner,
        const D3D10_BUFFER_DESC *pDesc,
        UINT64 id,
        const D3D10_SUBRESOURCE_DATA *pInitialData
    );

    virtual ~MyID3D10Buffer();

    ID3D10RESOURCE_DECL(ID3D10Buffer)
    D3D10_BUFFER_DESC &get_desc();
    const D3D10_BUFFER_DESC &get_desc() const;
    bool &get_cached_state();
    bool get_cached_state() const;
    char *&get_cached();
    char *get_cached() const;

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
};

extern std::unordered_map<ID3D10Buffer *, MyID3D10Buffer *> cached_bs_map;

#endif
