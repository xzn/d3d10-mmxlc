#ifndef OVERLAY_H
#define OVERLAY_H

#include "main.h"

#include "../imgui/imgui.h"

class Overlay {
    HWND hwnd;
    ID3D10Device *pDevice;
    IDXGISwapChain *pSwapChain;
    ID3D10RenderTargetView *rtv;
    ImGuiContext *imgui_context;
    ImGuiIO *io;
    UINT64 Time;
    UINT64 TicksPerSecond;
    ImVec2 DisplaySize;

    bool log_toggle_hotkey_active;
    bool log_frame_hotkey_active;
    bool log_frame_active;

    struct Text {
        std::string text;
        UINT64 time;
    };
    static std::deque<Text> texts;
    static CRITICAL_SECTION texts_cs;
    friend BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID);

    void reset_texts_timings();

    bool hotkey_active(const std::vector<BYTE> &vks);

    void create_render_target();
    void cleanup_render_target();

    void set_display_size(ImVec2 size);

    static void push_text_base(std::string &s);
    template<class T, class... Ts>
    static std::enable_if_t<std::is_convertible_v<T, std::string>> push_text_base(std::string &s, T a, Ts... as) {
        s += std::string(a);
        push_text_base(s, as...);
    }
    template<class T, class... Ts>
    static std::enable_if_t<std::is_convertible_v<T, std::wstring>> push_text_base(std::string &s, T a, Ts... as) {
        push_text_base(s, std::wstring_convert<std::codecvt_utf8<wchar_t>>{}.to_bytes(std::wstring(a)), as...);
    }

public:
    Overlay(
        DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
        IDXGISwapChain *pSwapChain,
        ID3D10Device *pDevice
    );

    ~Overlay();

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
    static void push_text(Ts... as) {
        EnterCriticalSection(&texts_cs);

        std::string s{};
        push_text_base(s, as...);

        std::cerr << s << std::endl;

        LeaveCriticalSection(&texts_cs);
    }
};
extern std::unordered_set<Overlay *> *default_overlays;

#endif
