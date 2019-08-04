#ifndef D3D10SAMPLERSTATE_H
#define D3D10SAMPLERSTATE_H

#include "main.h"
#include "d3d10devicechild.h"

class MyID3D10SamplerState : public ID3D10SamplerState {
    class Impl;
    Impl *impl;

public:
    ID3D10DEVICECHILD_DECL(ID3D10SamplerState);

    ID3D10SamplerState *&get_linear();
    ID3D10SamplerState *get_linear() const;

    MyID3D10SamplerState(
        ID3D10SamplerState **inner,
        const D3D10_SAMPLER_DESC *pDesc,
        ID3D10SamplerState *linear = NULL
    );

    virtual ~MyID3D10SamplerState();
    D3D10_SAMPLER_DESC &get_desc();
    const D3D10_SAMPLER_DESC &get_desc() const;

    virtual void STDMETHODCALLTYPE GetDesc(
        D3D10_SAMPLER_DESC *pDesc
    );
};

extern std::unordered_map<ID3D10SamplerState *, MyID3D10SamplerState *> cached_sss_map;

#endif
