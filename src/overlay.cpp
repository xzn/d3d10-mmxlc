#include "overlay.h"
#include "dxgiswapchain.h"
#include "d3d10device.h"

#include "../imgui/imgui.h"
#include "../imgui/examples/imgui_impl_dx10.h"

#define TEXT_DURATION 2.5

namespace {

cs_wrapper gui_cs;

}

class Overlay::Impl {
    friend class Overlay;

    struct Text {
        std::string text;
        UINT64 time = 0;
    };
    std::deque<Text> texts;

    void push_text_base(std::string &&s) {
        std::cerr << s << std::endl;
        texts.emplace_back(Text{std::move(s)});
    }
    cs_wrapper texts_cs;

    void begin_text() {
        texts_cs.begin_cs();
    }

    void end_text() {
        texts_cs.end_cs();
    }

    void reset_texts_timings() {
        for (Text &text : texts) text.time = 0;
    }

    HWND hwnd = NULL;
    MyID3D10Device *pDevice = NULL;
    MyIDXGISwapChain *pSwapChain = NULL;
    ID3D10RenderTargetView *rtv = NULL;
    ImGuiContext *imgui_context = NULL;
    ImGuiIO *io = NULL;
    UINT64 time = 0;
    UINT64 ticks_per_second = 0;
    ImVec2 display_size = {};

    void create_render_target() {
        if (!rtv) {
            ID3D10Texture2D* pBackBuffer = NULL;
            pSwapChain->get_inner()->GetBuffer(0, __uuidof(ID3D10Texture2D), (LPVOID*)&pBackBuffer);
            if (pBackBuffer) {
                pDevice->get_inner()->CreateRenderTargetView(pBackBuffer, NULL, &rtv);
                pBackBuffer->Release();
            }
        }
        ImGui_ImplDX10_CreateDeviceObjects();
    }

    void cleanup_render_target() {
        ImGui_ImplDX10_InvalidateDeviceObjects();
        if (rtv) {
            rtv->Release();
            rtv = NULL;
        }
    }

    void set_display_size(ImVec2 size) {
        display_size = size;
    }

    void set_display(
        DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
        MyIDXGISwapChain *pSwapChain,
        MyID3D10Device *pDevice
    ) {
        reset_display();
        if (!(
            pSwapChainDesc &&
            pSwapChain &&
            pDevice
        )) return;

        hwnd = pSwapChainDesc->OutputWindow;
        this->pDevice = pDevice;
        this->pSwapChain = pSwapChain;

        pDevice->AddRef();
        pSwapChain->AddRef();

        imgui_context = ImGui::CreateContext();
        io = &ImGui::GetIO();
        io->IniFilename = NULL;
        ImGui_ImplDX10_Init(pDevice->get_inner());
        ImGui::StyleColorsClassic();
        ImGuiStyle *style = &ImGui::GetStyle();
        style->WindowBorderSize = 0;
        set_display_size(ImVec2(
            pSwapChainDesc->BufferDesc.Width,
            pSwapChainDesc->BufferDesc.Height
        ));

        create_render_target();
    }

    void reset_display() {
        cleanup_render_target();

        set_display_size({});
        ImGui_ImplDX10_Shutdown();
        io = NULL;
        if (imgui_context) {
            ImGui::DestroyContext(imgui_context);
            imgui_context = NULL;
        }

        if (pDevice) {
            pDevice->set_overlay(NULL);
            pDevice->Release();
            pDevice = NULL;
        }
        if (pSwapChain) {
            pSwapChain->set_overlay(NULL);
            pSwapChain->Release();
            pSwapChain = NULL;
        }
        hwnd = NULL;
    }

    Impl() {
        QueryPerformanceFrequency((LARGE_INTEGER *)&ticks_per_second);
    }

    ~Impl() {
        reset_display();
    }

    HRESULT resize_buffers(
        UINT buffer_count,
        UINT width,
        UINT height,
        DXGI_FORMAT format,
        UINT flags
    ) {
        reset_texts_timings();
        cleanup_render_target();
        HRESULT ret = pSwapChain->get_inner()->ResizeBuffers(
            buffer_count,
            width,
            height,
            format,
            flags
        );
        if (ret == S_OK) {
            set_display_size(ImVec2(width, height));
            create_render_target();
        }
        return ret;
    }

    void present(
        UINT SyncInterval,
        UINT Flags
    ) {
        if (!(
            texts.size() &&
            rtv &&
            gui_cs.try_begin_cs()
        )) {
            time = 0;
            return ;
        }

        ImGui::SetCurrentContext(imgui_context);
        ImGui_ImplDX10_NewFrame();

        if (!time || hwnd != GetForegroundWindow()) {
            reset_texts_timings();
            QueryPerformanceCounter((LARGE_INTEGER *)&time);
            io->DeltaTime = 1.0f/60.0f;
        } else {
            UINT64 current_time;
            QueryPerformanceCounter((LARGE_INTEGER *)&current_time);
            io->DeltaTime = (float)(current_time - time) / ticks_per_second;
            time = current_time;
        }
        io->DisplaySize = display_size;

        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(0, 0));
        ImGui::Begin(
            "Overlay",
            NULL,
            ImGuiWindowFlags_NoTitleBar
        );
        while (
            texts.size() &&
            texts.front().time &&
            time - texts.front().time > ticks_per_second * TEXT_DURATION
        ) {
            texts.pop_front();
        }
        for (Text &text : texts) {
            if (!text.time) text.time = time;
            ImGui::TextUnformatted(text.text.c_str());
        }
        ImGui::End();
        ImGui::Render();
        pDevice->get_inner()->OMSetRenderTargets(1, &rtv, NULL);
        ImGui_ImplDX10_RenderDrawData(ImGui::GetDrawData());

        gui_cs.end_cs();
    }
};

Overlay::Overlay() : impl(new Impl()) {}

Overlay::~Overlay() {
    delete impl;
}

void Overlay::set_display(
    DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
    MyIDXGISwapChain *pSwapChain,
    MyID3D10Device *pDevice
) {
    impl->begin_text();
    impl->set_display(
        pSwapChainDesc,
        pSwapChain,
        pDevice
    );
    impl->end_text();
}

HRESULT Overlay::present(
    UINT SyncInterval,
    UINT Flags
) {
    impl->begin_text();
    impl->present(SyncInterval, Flags);
    impl->end_text();
    return impl->pSwapChain->get_inner()->Present(SyncInterval, Flags);
}

HRESULT Overlay::resize_buffers(
    UINT buffer_count,
    UINT width,
    UINT height,
    DXGI_FORMAT format,
    UINT flags
) {
    impl->begin_text();
    HRESULT ret = impl->resize_buffers(
        buffer_count,
        width,
        height,
        format,
        flags
    );
    impl->end_text();
    return ret;
}

void Overlay::push_text_base(std::string &&s) {
    impl->begin_text();
    impl->push_text_base(std::move(s));
    impl->end_text();
}

OverlayPtr default_overlay;
