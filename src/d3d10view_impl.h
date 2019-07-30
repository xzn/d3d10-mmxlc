#ifndef D3D10VIEW_IMPL_H
#define D3D10VIEW_IMPL_H

#include "d3d10devicechild_impl.h"

#define ID3D10VIEW_PRIV \
    ID3D10Resource *resource = NULL; \

#define ID3D10VIEW_INIT(n) \
    resource(n)

#define ID3D10VIEW_IMPL(d, b) \
    ID3D10Resource *&d::get_resource() { \
        return impl->resource; \
    } \
 \
    ID3D10Resource *d::get_resource() const { \
        return impl->resource; \
    } \
 \
    void STDMETHODCALLTYPE d::GetResource( \
        ID3D10Resource **ppResource \
    ) { \
        LOG_MFUN(); \
        impl->inner->GetResource(ppResource); \
    } \
 \
    ID3D10DEVICECHILD_IMPL(d, b)

#endif
