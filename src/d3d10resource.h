#ifndef D3D10RESOURCE_H
#define D3D10RESOURCE_H

#include "d3d10devicechild.h"

#define ID3D10RESOURCE_DECL \
    UINT64 id; \
 \
    virtual void STDMETHODCALLTYPE GetType( \
        D3D10_RESOURCE_DIMENSION *rType \
    ); \
 \
    virtual void STDMETHODCALLTYPE SetEvictionPriority( \
        UINT EvictionPriority \
    ); \
 \
    virtual UINT STDMETHODCALLTYPE GetEvictionPriority(); \
 \
    ID3D10DEVICECHILD_DECL

#define ID3D10RESOURCE_IMPL(n, t) \
    void STDMETHODCALLTYPE n::GetType( \
        D3D10_RESOURCE_DIMENSION *rType \
    ) { \
        LOG_MFUN(); \
        if (rType) *rType = t; \
    } \
 \
    void STDMETHODCALLTYPE n::SetEvictionPriority( \
        UINT EvictionPriority \
    ) { \
        LOG_MFUN(); \
        inner->SetEvictionPriority(EvictionPriority); \
    } \
 \
    UINT STDMETHODCALLTYPE n::GetEvictionPriority() { \
        LOG_MFUN(); \
        return inner->GetEvictionPriority(); \
    } \
 \
    ID3D10DEVICECHILD_IMPL(n)

#endif
