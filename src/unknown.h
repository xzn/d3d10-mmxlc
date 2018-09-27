#ifndef UNKNOWN_H
#define UNKNOWN_H

#define IUNKNOWN_DECL(n, b) \
    b *const inner; \
    DWORD rc; \
 \
    HRESULT STDMETHODCALLTYPE QueryInterface( \
        REFIID riid, \
        void   **ppvObject \
    ); \
 \
    ULONG STDMETHODCALLTYPE AddRef(); \
 \
    ULONG STDMETHODCALLTYPE Release();

#define IUNKNOWN_IMPL(n) \
    HRESULT STDMETHODCALLTYPE n::QueryInterface( \
        REFIID riid, \
        void   **ppvObject \
    ) { \
        HRESULT ret = inner->QueryInterface(riid, ppvObject); \
        LOG_MFUN(_, \
            LOG_ARG(&riid), \
            LOG_ARG(*ppvObject), \
            ret \
        ); \
        return ret; \
    } \
 \
    ULONG STDMETHODCALLTYPE n::AddRef() { \
        unsigned ret = InterlockedIncrement(&rc); \
        LOG_MFUN(_, ret); \
        return ret; \
    } \
 \
    ULONG STDMETHODCALLTYPE n::Release() { \
        unsigned ret = InterlockedDecrement(&rc); \
        if (rc == 0) { \
            inner->Release(); \
            delete this; \
        } \
        LOG_MFUN(_, ret); \
        return ret; \
    }

#define IUNKNOWN_INIT(n) \
    inner(n), \
    rc(1)

#endif
