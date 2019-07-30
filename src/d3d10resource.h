#ifndef D3D10RESOURCE_H
#define D3D10RESOURCE_H

#include "d3d10devicechild.h"

#define ID3D10RESOURCE_DECL(b) \
    UINT64 get_id() const; \
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
    ID3D10DEVICECHILD_DECL(b)

#endif
