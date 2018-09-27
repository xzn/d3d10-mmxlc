#ifndef D3D10DEVICECHILD_H
#define D3D10DEVICECHILD_H

#define ID3D10DEVICECHILD_DECL \
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
    );

#define ID3D10DEVICECHILD_IMPL(n) \
    void STDMETHODCALLTYPE n::GetDevice( \
        ID3D10Device **ppDevice \
    ) { \
        LOG_MFUN(); \
        inner->GetDevice(ppDevice); \
    } \
 \
    HRESULT STDMETHODCALLTYPE n::GetPrivateData( \
        REFGUID guid, \
        UINT *pDataSize, \
        void *pData \
    ) { \
        LOG_MFUN(); \
        return inner->GetPrivateData(guid, pDataSize, pData); \
    } \
 \
    HRESULT STDMETHODCALLTYPE n::SetPrivateData( \
        REFGUID guid, \
        UINT DataSize, \
        const void *pData \
    ) { \
        LOG_MFUN(); \
        return inner->SetPrivateData(guid, DataSize, pData); \
    } \
 \
    HRESULT STDMETHODCALLTYPE n::SetPrivateDataInterface( \
        REFGUID guid, \
        const IUnknown *pData \
    ) { \
        LOG_MFUN(); \
        return inner->SetPrivateDataInterface(guid, pData); \
    }

#endif
