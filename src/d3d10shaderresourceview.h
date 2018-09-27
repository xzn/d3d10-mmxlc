#ifndef D3D10SHADERRESOURCEVIEW_H
#define D3D10SHADERRESOURCEVIEW_H

#include "main.h"
#include "unknown.h"
#include "d3d10view.h"

class MyID3D10ShaderResourceView : public ID3D10ShaderResourceView {
public:
    D3D10_SHADER_RESOURCE_VIEW_DESC desc;
    ID3D10Resource *const resource;

    MyID3D10ShaderResourceView(
        ID3D10ShaderResourceView **inner,
        const D3D10_SHADER_RESOURCE_VIEW_DESC *pDesc,
        ID3D10Resource *resource
    );

    virtual ~MyID3D10ShaderResourceView();

    IUNKNOWN_DECL(MyID3D10ShaderResourceView, ID3D10ShaderResourceView)

    virtual void STDMETHODCALLTYPE GetDesc(
        D3D10_SHADER_RESOURCE_VIEW_DESC *pDesc
    );

    ID3D10VIEW_DECL
};

extern std::unordered_map<ID3D10ShaderResourceView *, MyID3D10ShaderResourceView *> current_srvs_map;

#endif
