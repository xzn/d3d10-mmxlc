#ifndef D3D10RENDERTARGETVIEW_H
#define D3D10RENDERTARGETVIEW_H

#include "main.h"
#include "unknown.h"
#include "d3d10view.h"

class MyID3D10RenderTargetView : public ID3D10RenderTargetView {
public:
    D3D10_RENDER_TARGET_VIEW_DESC desc;
    ID3D10Resource *const resource;

    MyID3D10RenderTargetView(
        ID3D10RenderTargetView **inner,
        const D3D10_RENDER_TARGET_VIEW_DESC *pDesc,
        ID3D10Resource *resource
    );

    virtual ~MyID3D10RenderTargetView();

    IUNKNOWN_DECL(MyID3D10RenderTargetView, ID3D10RenderTargetView)

    virtual void STDMETHODCALLTYPE GetDesc(
        D3D10_RENDER_TARGET_VIEW_DESC *pDesc
    );

    ID3D10VIEW_DECL
};

extern std::unordered_map<ID3D10RenderTargetView *, MyID3D10RenderTargetView *> cached_rtvs_map;

#endif
