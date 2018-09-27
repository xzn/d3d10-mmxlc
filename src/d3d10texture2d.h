#ifndef D3D10TEXTURE2D_H
#define D3D10TEXTURE2D_H

#include "main.h"
#include "unknown.h"
#include "d3d10resource.h"

class MyID3D10Texture2D : public ID3D10Texture2D {
public:
    D3D10_TEXTURE2D_DESC desc;

    MyID3D10Texture2D(
        ID3D10Texture2D **inner,
        const D3D10_TEXTURE2D_DESC *pDesc
    );

    virtual ~MyID3D10Texture2D();

    IUNKNOWN_DECL(MyID3D10Texture2D, ID3D10Texture2D)

    virtual HRESULT STDMETHODCALLTYPE Map(
        UINT Subresource,
        D3D10_MAP MapType,
        UINT MapFlags,
        D3D10_MAPPED_TEXTURE2D *pMappedTex2D
    );

    virtual void STDMETHODCALLTYPE Unmap(
        UINT Subresource
    );

    virtual void STDMETHODCALLTYPE GetDesc(
        D3D10_TEXTURE2D_DESC *pDesc
    );

    ID3D10RESOURCE_DECL
};

#endif
