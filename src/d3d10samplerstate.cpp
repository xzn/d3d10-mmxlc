#include "d3d10samplerstate.h"
#include "log.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10SamplerState, ## __VA_ARGS__)

IUNKNOWN_IMPL(MyID3D10SamplerState)

MyID3D10SamplerState::MyID3D10SamplerState(
    ID3D10SamplerState **inner,
    const D3D10_SAMPLER_DESC *pDesc
) :
    desc(*pDesc),
    IUNKNOWN_INIT(*inner)
{
    LOG_MFUN(_,
        LOG_ARG(*inner)
    );
    current_sss_map.emplace(*inner, this);
    *inner = this;
}

MyID3D10SamplerState::~MyID3D10SamplerState() {
    LOG_MFUN();
    current_sss_map.erase(inner);
}

void STDMETHODCALLTYPE MyID3D10SamplerState::GetDesc(
    D3D10_SAMPLER_DESC *pDesc
) {
    LOG_MFUN();
    inner->GetDesc(pDesc);
}

ID3D10DEVICECHILD_IMPL(MyID3D10SamplerState)

std::unordered_map<ID3D10SamplerState *, MyID3D10SamplerState *> current_sss_map;
