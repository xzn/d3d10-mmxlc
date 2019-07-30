#ifndef D3D10DEVICECHILD_IMPL_H
#define D3D10DEVICECHILD_IMPL_H

#include "unknown_impl.h"

#define ID3D10DEVICECHILD_IMPL(d, b) \
    void STDMETHODCALLTYPE d::GetDevice( \
        ID3D10Device **ppDevice \
    ) { \
        LOG_MFUN(); \
        impl->inner->GetDevice(ppDevice); \
    } \
 \
    HRESULT STDMETHODCALLTYPE d::GetPrivateData( \
        REFGUID guid, \
        UINT *pDataSize, \
        void *pData \
    ) { \
        LOG_MFUN(); \
        return impl->inner->GetPrivateData(guid, pDataSize, pData); \
    } \
 \
    HRESULT STDMETHODCALLTYPE d::SetPrivateData( \
        REFGUID guid, \
        UINT DataSize, \
        const void *pData \
    ) { \
        LOG_MFUN(); \
        return impl->inner->SetPrivateData(guid, DataSize, pData); \
    } \
 \
    HRESULT STDMETHODCALLTYPE d::SetPrivateDataInterface( \
        REFGUID guid, \
        const IUnknown *pData \
    ) { \
        LOG_MFUN(); \
        return impl->inner->SetPrivateDataInterface(guid, pData); \
    } \
 \
    IUNKNOWN_IMPL(d, b)

#endif
