#include "overlay.h"
#include "log.h"
#include "conf.h"

#include "../imgui/examples/imgui_impl_dx10.h"

#define TEXT_DURATION 2.5

void Overlay::reset_texts_timings() {
    for (Text &text : texts) text.time = 0;
}

bool Overlay::hotkey_active(const std::vector<BYTE> &vks) {
    if (!vks.size()) return false;
    for (BYTE vk : vks) {
        if (!GetAsyncKeyState(vk)) return false;
    }
    return true;
}

Overlay::Overlay(
    DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
    IDXGISwapChain *pSwapChain,
    ID3D10Device *pDevice
) :
    hwnd(pSwapChainDesc->OutputWindow),
    pDevice(pDevice),
    pSwapChain(pSwapChain),
    rtv(NULL),
    imgui_context(NULL),
    io(NULL),
    Time(0),
    TicksPerSecond(0),
    DisplaySize{},
    log_toggle_hotkey_active(false),
    log_frame_hotkey_active(false),
    log_frame_active(false)
{
    Config *config = default_config;
    if (config) {
        log_toggle_hotkey = config->log_toggle_hotkey;
        log_frame_hotkey = config->log_frame_hotkey;
    }

    pDevice->AddRef();
    pSwapChain->AddRef();

    assert(QueryPerformanceFrequency((LARGE_INTEGER *)&TicksPerSecond));

    imgui_context = ImGui::CreateContext();
    io = &ImGui::GetIO();
    io->IniFilename = NULL;
    ImGui_ImplDX10_Init(pDevice);
    ImGui::StyleColorsClassic();
    ImGuiStyle *style = &ImGui::GetStyle();
    style->WindowBorderSize = 0;
    set_display_size(ImVec2(
        pSwapChainDesc->BufferDesc.Width,
        pSwapChainDesc->BufferDesc.Height
    ));

    create_render_target();
}

Overlay::~Overlay() {
    cleanup_render_target();

    ImGui_ImplDX10_Shutdown();
    if (imgui_context) ImGui::DestroyContext(imgui_context);

    pDevice->Release();
    pSwapChain->Release();
}

HRESULT Overlay::present(
    UINT SyncInterval,
    UINT Flags
) {
    EnterCriticalSection(&texts_cs);

    Logger *logger = default_logger;
    if (log_frame_active) {
        log_frame_active = false;
        if (logger) logger->stop();
    }

    if (hwnd == GetForegroundWindow()) {
        if (hotkey_active(log_toggle_hotkey)) {
            if (!log_toggle_hotkey_active) {
                log_toggle_hotkey_active = true;
                if (logger) {
                    if (!logger->get_started()) {
                        if (logger->start()) {
                            Overlay::push_text("Logging to ", logger->get_file_name(), " enabled");
                        }
                    } else {
                        logger->stop();
                        Overlay::push_text("Logging to ", logger->get_file_name(), " disabled");
                    }
                }
            }
        } else {
            log_toggle_hotkey_active = false;
        }
        if (hotkey_active(log_frame_hotkey)) {
            if (!log_frame_hotkey_active) {
                log_frame_hotkey_active = true;
                if (logger) {
                    if (logger->start()) {
                        Overlay::push_text("Logging to ", logger->get_file_name(), " for one frame");
                        log_frame_active = true;
                    }
                }
            }
        } else {
            log_frame_hotkey_active = false;
        }
    } else {
        reset_texts_timings();
    }

    if (texts.size()) {
        ImGui::SetCurrentContext(imgui_context);
        ImGui_ImplDX10_NewFrame();

        if (!Time) {
            assert(QueryPerformanceCounter((LARGE_INTEGER *)&Time));
            io->DeltaTime = 1.0f/60.0f;
        } else {
            UINT64 current_time;
            assert(QueryPerformanceCounter((LARGE_INTEGER *)&current_time));
            io->DeltaTime = (float)(current_time - Time) / TicksPerSecond;
            Time = current_time;
        }
        io->DisplaySize = DisplaySize;

        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(0, 0));
        ImGui::Begin("", NULL, ImGuiWindowFlags_NoTitleBar);
        while (
            texts.size() &&
            texts.front().time &&
            Time - texts.front().time > TicksPerSecond * TEXT_DURATION
        ) {
            texts.pop_front();
        }
        for (Text &text : texts) {
            if (!text.time) text.time = Time;
            ImGui::TextUnformatted(text.text.c_str());
        }
        ImGui::End();
        ImGui::Render();
        pDevice->OMSetRenderTargets(1, &rtv, NULL);
        ImGui_ImplDX10_RenderDrawData(ImGui::GetDrawData());
    } else {
        Time = 0;
    }

    LeaveCriticalSection(&texts_cs);
    return pSwapChain->Present(SyncInterval, Flags);
}

void Overlay::create_render_target() {
    if (!rtv) {
        ID3D10Texture2D* pBackBuffer = NULL;
        pSwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (LPVOID*)&pBackBuffer);
        if (pBackBuffer) {
            pDevice->CreateRenderTargetView(pBackBuffer, NULL, &rtv);
            pBackBuffer->Release();
        }
    }

    ImGui_ImplDX10_CreateDeviceObjects();
}

void Overlay::cleanup_render_target() {
    ImGui_ImplDX10_InvalidateDeviceObjects();

    if (rtv) {
        rtv->Release();
        rtv = NULL;
    }
}

void Overlay::set_display_size(ImVec2 size) {
    DisplaySize = size;
}

HRESULT Overlay::resize_buffers(
    UINT buffer_count,
    UINT width,
    UINT height,
    DXGI_FORMAT format,
    UINT flags
) {
    EnterCriticalSection(&texts_cs);

    reset_texts_timings();
    cleanup_render_target();
    HRESULT ret = pSwapChain->ResizeBuffers(
        buffer_count,
        width,
        height,
        format,
        flags
    );
    set_display_size(ImVec2(width, height));
    create_render_target();

    LeaveCriticalSection(&texts_cs);
    return ret;
}

void Overlay::push_text_base(std::string &s) {
    texts.push_back({s, 0});
}

std::deque<Overlay::Text> Overlay::texts;
CRITICAL_SECTION Overlay::texts_cs;

std::unordered_set<Overlay *> *default_overlays;
