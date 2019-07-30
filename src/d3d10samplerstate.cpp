#include "d3d10samplerstate.h"
#include "d3d10devicechild_impl.h"
#include "log.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10SamplerState, ## __VA_ARGS__)

class MyID3D10SamplerState::Impl {
    friend class MyID3D10SamplerState;

    IUNKNOWN_PRIV(ID3D10SamplerState)

    D3D10_SAMPLER_DESC desc = {};
    ID3D10SamplerState *linear = NULL;

    Impl(
        ID3D10SamplerState **inner,
        const D3D10_SAMPLER_DESC *pDesc,
        ID3D10SamplerState *linear
    ) :
        IUNKNOWN_INIT(*inner),
        desc(*pDesc),
        linear(linear)
    {}

    ~Impl() { if (linear) linear->Release(); }
};

ID3D10DEVICECHILD_IMPL(MyID3D10SamplerState, ID3D10SamplerState)

D3D10_SAMPLER_DESC &MyID3D10SamplerState::get_desc() {
    return impl->desc;
}

const D3D10_SAMPLER_DESC &MyID3D10SamplerState::get_desc() const {
    return impl->desc;
}

MyID3D10SamplerState::MyID3D10SamplerState(
    ID3D10SamplerState **inner,
    const D3D10_SAMPLER_DESC *pDesc,
    ID3D10SamplerState *linear
) :
    impl(new Impl(inner, pDesc, linear))
{
    LOG_MFUN(_,
        LOG_ARG(*inner)
    );
    cached_sss_map.emplace(*inner, this);
    *inner = this;
}

MyID3D10SamplerState::~MyID3D10SamplerState() {
    LOG_MFUN();
    cached_sss_map.erase(impl->inner);
    delete impl;
}

ID3D10SamplerState *MyID3D10SamplerState::get_linear() {
    return impl->linear;
}

void STDMETHODCALLTYPE MyID3D10SamplerState::GetDesc(
    D3D10_SAMPLER_DESC *pDesc
) {
    LOG_MFUN();
    if (pDesc) *pDesc = impl->desc;
}

std::unordered_map<ID3D10SamplerState *, MyID3D10SamplerState *> cached_sss_map;
