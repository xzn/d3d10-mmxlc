#ifndef UNKNOWN_H
#define UNKNOWN_H

#define IUNKNOWN_DECL(b) \
    b *&get_inner(); \
 \
    HRESULT STDMETHODCALLTYPE QueryInterface( \
        REFIID riid, \
        void   **ppvObject \
    ); \
 \
    ULONG STDMETHODCALLTYPE AddRef(); \
 \
    ULONG STDMETHODCALLTYPE Release();

#endif
