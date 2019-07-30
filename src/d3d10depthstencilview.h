#ifndef D3D10DEPTHSTENCILVIEW_H
#define D3D10DEPTHSTENCILVIEW_H

#include "main.h"
#include "d3d10view.h"

class MyID3D10DepthStencilView : public ID3D10DepthStencilView {
    class Impl;
    Impl *impl;

public:
    MyID3D10DepthStencilView(
        ID3D10DepthStencilView **inner,
        const D3D10_DEPTH_STENCIL_VIEW_DESC *pDesc,
        ID3D10Resource *resource
    );

    virtual ~MyID3D10DepthStencilView();

    ID3D10VIEW_DECL(ID3D10DepthStencilView)
    D3D10_DEPTH_STENCIL_VIEW_DESC &get_desc();
    const D3D10_DEPTH_STENCIL_VIEW_DESC &get_desc() const;

    virtual void STDMETHODCALLTYPE GetDesc(
        D3D10_DEPTH_STENCIL_VIEW_DESC *pDesc
    );
};

extern std::unordered_map<ID3D10DepthStencilView *, MyID3D10DepthStencilView *> cached_dsvs_map;

#endif
