#ifndef UNKNOWN_IMPL_H
#define UNKNOWN_IMPL_H

#define IUNKNOWN_PRIV(b) \
    b *inner = NULL; \
    DWORD rc = 0;

#define IUNKNOWN_INIT(n) \
    inner(n), \
    rc(1)

#define IUNKNOWN_IMPL(d, b) \
    b *&d::get_inner() { \
        return impl->inner; \
    } \
 \
    HRESULT STDMETHODCALLTYPE d::QueryInterface( \
        REFIID riid, \
        void   **ppvObject \
    ) { \
        HRESULT ret = impl->inner->QueryInterface(riid, ppvObject); \
        if (ret == S_OK) { \
            LOG_MFUN(_, \
                LOG_ARG(riid), \
                LOG_ARG(*ppvObject), \
                ret \
            ); \
        } else { \
            LOG_MFUN(_, \
                LOG_ARG(riid), \
                ret \
            ); \
        } \
        return ret; \
    } \
 \
    ULONG STDMETHODCALLTYPE d::AddRef() { \
        unsigned ret = InterlockedIncrement(&impl->rc); \
        LOG_MFUN(_, ret); \
        return ret; \
    } \
 \
    ULONG STDMETHODCALLTYPE d::Release() { \
        unsigned ret = InterlockedDecrement(&impl->rc); \
        LOG_MFUN(_, ret); \
        if (!ret) { \
            impl->inner->Release(); \
            delete this; \
        } \
        return ret; \
    }

#endif
