#ifndef OVERLAY_H
#define OVERLAY_H

#include "main.h"

class MyIDXGISwapChain;
class MyID3D10Device;
class Overlay {
    class Impl;
    Impl *impl;

    void push_text_base(std::string &&s);
    template<class T, class... Ts>
    std::enable_if_t<std::is_convertible_v<T, std::string>> push_text_base(std::string &&s, const T &a, const Ts &... as) {
        s += std::string(a);
        push_text_base(std::move(s), as...);
    }
    template<class T, class... Ts>
    std::enable_if_t<std::is_convertible_v<T, std::wstring>> push_text_base(std::string &&s, const T &a, const Ts &... as) {
        push_text_base(std::move(s), std::wstring_convert<std::codecvt_utf8<wchar_t>>{}.to_bytes(std::wstring(a)), as...);
    }

public:
    Overlay();
    ~Overlay();

    void set_display(
        DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
        MyIDXGISwapChain *pSwapChain,
        MyID3D10Device *pDevice
    );

    HRESULT present(
        UINT SyncInterval,
        UINT Flags
    );

    HRESULT resize_buffers(
        UINT buffer_count,
        UINT width,
        UINT height,
        DXGI_FORMAT format,
        UINT flags
    );

    template<class... Ts>
    void push_text(Ts &... as) {
        std::string s;
        push_text_base(std::move(s), as...);
    }
};

extern struct OverlayPtr {
    Overlay *overlay;
    OverlayPtr(Overlay *overlay = NULL) : overlay(overlay) {}
    template<class... As>
    void operator()(const As &... as) const {
        if (overlay) overlay->push_text(as...);
    }
    OverlayPtr& operator=(Overlay *overlay) {
        this->overlay = overlay;
        return *this;
    }
    Overlay *operator->() const { return *this; }
    operator Overlay *() const { return overlay; }
} default_overlay;

#endif
