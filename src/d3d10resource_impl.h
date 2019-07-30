#ifndef D3D10RESOURCE_IMPL_H
#define D3D10RESOURCE_IMPL_H

#include "d3d10devicechild_impl.h"

#define ID3D10RESOURCE_PRIV \
    UINT64 id = 0;

#define ID3D10RESOURCE_INIT(n) \
    id(n)

#define ID3D10RESOURCE_IMPL(d, b, t) \
    UINT64 d::get_id() const { \
        return impl->id; \
    } \
 \
    void STDMETHODCALLTYPE d::GetType( \
        D3D10_RESOURCE_DIMENSION *rType \
    ) { \
        LOG_MFUN(); \
        if (rType) *rType = t; \
    } \
 \
    void STDMETHODCALLTYPE d::SetEvictionPriority( \
        UINT EvictionPriority \
    ) { \
        LOG_MFUN(); \
        impl->inner->SetEvictionPriority(EvictionPriority); \
    } \
 \
    UINT STDMETHODCALLTYPE d::GetEvictionPriority() { \
        LOG_MFUN(); \
        return impl->inner->GetEvictionPriority(); \
    } \
 \
    ID3D10DEVICECHILD_IMPL(d, b)

#endif
