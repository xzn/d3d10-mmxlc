#ifndef D3D10DEPTHSTENCILSTATE_H
#define D3D10DEPTHSTENCILSTATE_H

#include "main.h"
#include "d3d10devicechild.h"

class MyID3D10DepthStencilState : public ID3D10DepthStencilState {
    class Impl;
    Impl *impl;

public:
    ID3D10DEVICECHILD_DECL(ID3D10DepthStencilState);

    MyID3D10DepthStencilState(
        ID3D10DepthStencilState **inner,
        const D3D10_DEPTH_STENCIL_DESC *pDesc
    );

    virtual ~MyID3D10DepthStencilState();
    D3D10_DEPTH_STENCIL_DESC &get_desc();
    const D3D10_DEPTH_STENCIL_DESC &get_desc() const;

    virtual void STDMETHODCALLTYPE GetDesc(
        D3D10_DEPTH_STENCIL_DESC *pDesc
    );
};

extern std::unordered_map<ID3D10DepthStencilState *, MyID3D10DepthStencilState *> cached_dsss_map;

#endif
