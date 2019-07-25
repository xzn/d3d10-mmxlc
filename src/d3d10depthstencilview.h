#ifndef D3D10DEPTHSTENCILVIEW_H
#define D3D10DEPTHSTENCILVIEW_H

#include "main.h"
#include "unknown.h"
#include "d3d10view.h"

class MyID3D10DepthStencilView : public ID3D10DepthStencilView {
public:
    D3D10_DEPTH_STENCIL_VIEW_DESC desc;
    ID3D10Resource *const resource;

    MyID3D10DepthStencilView(
        ID3D10DepthStencilView **inner,
        const D3D10_DEPTH_STENCIL_VIEW_DESC *pDesc,
        ID3D10Resource *resource
    );

    virtual ~MyID3D10DepthStencilView();

    IUNKNOWN_DECL(MyID3D10DepthStencilView, ID3D10DepthStencilView)

    virtual void STDMETHODCALLTYPE GetDesc(
        D3D10_DEPTH_STENCIL_VIEW_DESC *pDesc
    );

    ID3D10VIEW_DECL
};

extern std::unordered_map<ID3D10DepthStencilView *, MyID3D10DepthStencilView *> cached_dsvs_map;

#endif
