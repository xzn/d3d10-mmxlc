#ifndef D3D10SHADERRESOURCEVIEW_H
#define D3D10SHADERRESOURCEVIEW_H

#include "main.h"
#include "unknown.h"
#include "d3d10view.h"

class MyID3D10ShaderResourceView : public ID3D10ShaderResourceView {
    class Impl;
    Impl *impl;

public:
    MyID3D10ShaderResourceView(
        ID3D10ShaderResourceView **inner,
        const D3D10_SHADER_RESOURCE_VIEW_DESC *pDesc,
        ID3D10Resource *resource
    );

    virtual ~MyID3D10ShaderResourceView();

    ID3D10VIEW_DECL(ID3D10ShaderResourceView)
    D3D10_SHADER_RESOURCE_VIEW_DESC &get_desc();
    const D3D10_SHADER_RESOURCE_VIEW_DESC &get_desc() const;

    virtual void STDMETHODCALLTYPE GetDesc(
        D3D10_SHADER_RESOURCE_VIEW_DESC *pDesc
    );
};

extern std::unordered_map<ID3D10ShaderResourceView *, MyID3D10ShaderResourceView *> cached_srvs_map;

#endif
