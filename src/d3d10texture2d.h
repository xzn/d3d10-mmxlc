#ifndef D3D10TEXTURE2D_H
#define D3D10TEXTURE2D_H

#include "main.h"
#include "d3d10resource.h"

class MyID3D10RenderTargetView;
class MyID3D10ShaderResourceView;
class MyID3D10DepthStencilView;
class MyIDXGISwapChain;

class MyID3D10Texture2D : public ID3D10Texture2D {
    class Impl;
    Impl *impl;

public:
    UINT &get_orig_width();
    UINT &get_orig_height();
    std::unordered_set<MyID3D10RenderTargetView *> &get_rtvs();
    std::unordered_set<MyID3D10ShaderResourceView *> &get_srvs();
    std::unordered_set<MyID3D10DepthStencilView *> &get_dsvs();
    MyIDXGISwapChain *&get_sc();

    MyID3D10Texture2D(
        ID3D10Texture2D **inner,
        const D3D10_TEXTURE2D_DESC *pDesc,
        UINT64 id,
        MyIDXGISwapChain *sc = NULL
    );

    virtual ~MyID3D10Texture2D();
    D3D10_TEXTURE2D_DESC &get_desc();
    const D3D10_TEXTURE2D_DESC &get_desc() const;

    ID3D10RESOURCE_DECL(ID3D10Texture2D)

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
};

#endif
