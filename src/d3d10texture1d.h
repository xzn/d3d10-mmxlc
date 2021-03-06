#ifndef D3D10TEXTURE1D_H
#define D3D10TEXTURE1D_H

#include "main.h"
#include "d3d10resource.h"

class MyID3D10Texture1D : public ID3D10Texture1D {
    class Impl;
    Impl *impl;

public:
    MyID3D10Texture1D(
        ID3D10Texture1D **inner,
        const D3D10_TEXTURE1D_DESC *pDesc,
        UINT64 id
    );

    virtual ~MyID3D10Texture1D();

    ID3D10RESOURCE_DECL(ID3D10Texture1D)
    D3D10_TEXTURE1D_DESC &get_desc();
    const D3D10_TEXTURE1D_DESC &get_desc() const;

    virtual HRESULT STDMETHODCALLTYPE Map(
        UINT Subresource,
        D3D10_MAP MapType,
        UINT MapFlags,
        void **ppData
    );

    virtual void STDMETHODCALLTYPE Unmap(
        UINT Subresource
    );

    virtual void STDMETHODCALLTYPE GetDesc(
        D3D10_TEXTURE1D_DESC *pDesc
    );
};

#endif
