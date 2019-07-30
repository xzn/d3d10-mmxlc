#ifndef D3D10VIEW_H
#define D3D10VIEW_H

#include "d3d10devicechild.h"

#define ID3D10VIEW_DECL(b) \
    ID3D10Resource *&get_resource();  \
    ID3D10Resource *get_resource() const;  \
 \
    virtual void STDMETHODCALLTYPE GetResource( \
        ID3D10Resource **ppResource \
    ); \
 \
    ID3D10DEVICECHILD_DECL(b)

#endif
