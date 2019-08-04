#ifndef UNKNOWN_IMPL_H
#define UNKNOWN_IMPL_H

#define IUNKNOWN_PRIV(b) \
    b *inner = NULL;

#define IUNKNOWN_INIT(n) \
    inner(n)

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
        ULONG ret = impl->inner->AddRef(); \
        LOG_MFUN(_, ret); \
        return ret; \
    } \
 \
    ULONG STDMETHODCALLTYPE d::Release() { \
        ULONG ret = impl->inner->Release(); \
        LOG_MFUN(_, ret); \
        if (!ret) { \
            delete this; \
        } \
        return ret; \
    }

#endif
