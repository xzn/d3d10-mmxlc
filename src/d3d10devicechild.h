#ifndef D3D10DEVICECHILD_H
#define D3D10DEVICECHILD_H

#include "unknown.h"

#define ID3D10DEVICECHILD_DECL(b) \
    virtual void STDMETHODCALLTYPE GetDevice( \
        ID3D10Device **ppDevice \
    ); \
 \
    virtual HRESULT STDMETHODCALLTYPE GetPrivateData( \
        REFGUID guid, \
        UINT *pDataSize, \
        void *pData \
    ); \
 \
    virtual HRESULT STDMETHODCALLTYPE SetPrivateData( \
        REFGUID guid, \
        UINT DataSize, \
        const void *pData \
    ); \
 \
    virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface( \
        REFGUID guid, \
        const IUnknown *pData \
    ); \
 \
    IUNKNOWN_DECL(b)

#endif
