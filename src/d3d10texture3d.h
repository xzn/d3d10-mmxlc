#ifndef D3D10TEXTURE3D_H
#define D3D10TEXTURE3D_H

#include "main.h"
#include "unknown.h"
#include "d3d10resource.h"

class MyID3D10Texture3D : public ID3D10Texture3D {
public:
    D3D10_TEXTURE3D_DESC desc;

    MyID3D10Texture3D(
        ID3D10Texture3D **inner,
        const D3D10_TEXTURE3D_DESC *pDesc,
        UINT64 id
    );

    virtual ~MyID3D10Texture3D();

    IUNKNOWN_DECL(MyID3D10Texture3D, ID3D10Texture3D)

    virtual HRESULT STDMETHODCALLTYPE Map(
        UINT Subresource,
        D3D10_MAP MapType,
        UINT MapFlags,
        D3D10_MAPPED_TEXTURE3D *pMappedTex3D
    );

    virtual void STDMETHODCALLTYPE Unmap(
        UINT Subresource
    );

    virtual void STDMETHODCALLTYPE GetDesc(
        D3D10_TEXTURE3D_DESC *pDesc
    );

    ID3D10RESOURCE_DECL
};

#endif
