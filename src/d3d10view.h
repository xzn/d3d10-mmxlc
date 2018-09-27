#ifndef D3D10VIEW_H
#define D3D10VIEW_H

#include "d3d10devicechild.h"

#define ID3D10VIEW_DECL \
    virtual void STDMETHODCALLTYPE GetResource( \
        ID3D10Resource **ppResource \
    ); \
 \
    ID3D10DEVICECHILD_DECL

#define ID3D10VIEW_IMPL(n) \
    void STDMETHODCALLTYPE n::GetResource( \
        ID3D10Resource **ppResource \
    ) { \
        LOG_MFUN(); \
        inner->GetResource(ppResource); \
    } \
 \
    ID3D10DEVICECHILD_IMPL(n)

#endif
