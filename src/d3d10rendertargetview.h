#ifndef D3D10RENDERTARGETVIEW_H
#define D3D10RENDERTARGETVIEW_H

#include "main.h"
#include "unknown.h"
#include "d3d10view.h"

class MyID3D10RenderTargetView : public ID3D10RenderTargetView {
    class Impl;
    Impl *impl;

public:
    MyID3D10RenderTargetView(
        ID3D10RenderTargetView **inner,
        const D3D10_RENDER_TARGET_VIEW_DESC *pDesc,
        ID3D10Resource *resource
    );

    virtual ~MyID3D10RenderTargetView();

    ID3D10VIEW_DECL(ID3D10RenderTargetView)
    D3D10_RENDER_TARGET_VIEW_DESC &get_desc();
    const D3D10_RENDER_TARGET_VIEW_DESC &get_desc() const;

    virtual void STDMETHODCALLTYPE GetDesc(
        D3D10_RENDER_TARGET_VIEW_DESC *pDesc
    );
};

extern std::unordered_map<ID3D10RenderTargetView *, MyID3D10RenderTargetView *> cached_rtvs_map;

#endif
