#ifndef D3D10SAMPLERSTATE_H
#define D3D10SAMPLERSTATE_H

#include "main.h"
#include "unknown.h"
#include "d3d10devicechild.h"

class MyID3D10SamplerState : public ID3D10SamplerState {
public:
    D3D10_SAMPLER_DESC desc;
    ID3D10SamplerState *linear;

    MyID3D10SamplerState(
        ID3D10SamplerState **inner,
        const D3D10_SAMPLER_DESC *pDesc,
        ID3D10SamplerState *linear
    );

    virtual ~MyID3D10SamplerState();

    IUNKNOWN_DECL(MyID3D10SamplerState, ID3D10SamplerState)

    virtual void STDMETHODCALLTYPE GetDesc(
        D3D10_SAMPLER_DESC *pDesc
    );

    ID3D10DEVICECHILD_DECL
};

extern std::unordered_map<ID3D10SamplerState *, MyID3D10SamplerState *> cached_sss_map;

#endif
