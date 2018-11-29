#include "d3d10device.h"
#include "d3d10pixelshader.h"
#include "d3d10buffer.h"
#include "d3d10texture1d.h"
#include "d3d10texture2d.h"
#include "d3d10texture3d.h"
#include "d3d10samplerstate.h"
#include "d3d10rendertargetview.h"
#include "d3d10shaderresourceview.h"
#include "d3d10depthstencilview.h"
#include "conf.h"
#include "log.h"
#include "overlay.h"
#include "tex.h"
#include "../smhasher/src/MurmurHash3.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10Device, ## __VA_ARGS__)
#define LOG_MFUN_BEGIN(_, ...) LOG_MFUN_BEGIN_DEF(MyID3D10Device, ## __VA_ARGS__)

#define BYTECODE_LENGTH_THRESHOLD 1000

#define X1_WIDTH 256
#define X1_HEIGHT 224
#define X4_WIDTH 320
#define X4_HEIGHT 240
#define X1_WIDTH_FILTERED (X1_WIDTH * 2)
#define X1_HEIGHT_FILTERED (X1_HEIGHT * 2)
#define X4_WIDTH_FILTERED (X4_WIDTH * 2)
#define X4_HEIGHT_FILTERED (X4_HEIGHT * 2)

void MyID3D10Device::clear_filter() {
    filter = false;
    if (filter_state.srv) {
        filter_state.srv->Release();
        filter_state.srv = NULL;
    }
    if (filter_state.rtv_tex) {
        filter_state.rtv_tex->Release();
        filter_state.rtv_tex = NULL;
    }
    if (filter_state.ps) {
        filter_state.ps->Release();
        filter_state.ps = NULL;
    }
    filter_state.t1 = false;
    if (filter_state.psss) {
        filter_state.psss->Release();
        filter_state.psss = NULL;
    }
    filter_state.x4 = false;
    filter_state.start_vertex_location = 0;
}

void MyID3D10Device::update_config() {
    config.interp = default_config->interp;
    config.enhanced = default_config->enhanced;

if constexpr (ENABLE_SLANG_SHADER) {
    if (default_config->slang_shader_updated || default_config->slang_shader_3d_updated) {
        EnterCriticalSection(&default_config->cs);

        if (default_config->slang_shader_updated) {
            if (!d3d10) {
                if (!(d3d10 = my_d3d10_gfx_init(inner, TEX_FORMAT))) {
                    Overlay::push_text("Failed to initialize slang shader");
                }
            }
            if (d3d10) {
                if (!default_config->slang_shader.size()) {
                    my_d3d10_gfx_set_shader(d3d10, NULL);
                    Overlay::push_text("Slang shader disabled");
                } else if (my_d3d10_gfx_set_shader(d3d10, default_config->slang_shader.c_str())) {
                    Overlay::push_text("Slang shader set to ", default_config->slang_shader);
                } else {
                    Overlay::push_text("Failed to set slang shader to ", default_config->slang_shader);
                }
            }
            default_config->slang_shader_updated = false;
        }

        if (default_config->slang_shader_3d_updated) {
            if (!d3d10_3d) {
                if (!(d3d10_3d = my_d3d10_gfx_init(inner, TEX_FORMAT))) {
                    Overlay::push_text("Failed to initialize slang shader 3d");
                }
            }
            if (d3d10_3d) {
                if (!default_config->slang_shader_3d.size()) {
                    my_d3d10_gfx_set_shader(d3d10_3d, NULL);
                    Overlay::push_text("Slang shader 3d disabled");
                } else if (my_d3d10_gfx_set_shader(d3d10_3d, default_config->slang_shader_3d.c_str())) {
                    Overlay::push_text("Slang shader 3d set to ", default_config->slang_shader_3d);
                } else {
                    Overlay::push_text("Failed to set slang shader 3d to ", default_config->slang_shader_3d);
                }
            }
            default_config->slang_shader_3d_updated = false;
        }

        LeaveCriticalSection(&default_config->cs);
    }
}
}

void MyID3D10Device::present() {
    clear_filter();
    update_config();
    ++frame_count;
}

void MyID3D10Device::Size::resize(UINT width, UINT height) {
    sc_width = width;
    sc_height = height;
    render_width = sc_height / 3 * 4;
    render_height = sc_height;
    render_3d_width = render_width - 1;
    render_3d_height = render_height;
}

void MyID3D10Device::resize_render_3d(UINT width, UINT height) {
    render_3d_width = width;
    render_3d_height = height;
    if (render_3d_width && render_3d_height) {
        Overlay::push_text("3D render resolution set to ", std::to_string(render_3d_width), "x", std::to_string(render_3d_height));
    } else {
        render_3d_width = current_size.render_3d_width;
        render_3d_height = current_size.render_3d_height;
        Overlay::push_text("Restoring 3D render resolution to ", std::to_string(render_3d_width), "x", std::to_string(render_3d_height));
    }
}

void MyID3D10Device::resize_buffers(UINT width, UINT height) {
    current_size.resize(width, height);
    if (!render_3d_width || !render_3d_height) {
        render_3d_width = current_size.render_3d_width;
        render_3d_height = current_size.render_3d_height;
    }
    clear_filter();
    update_config();
    filter_temp_init();
    frame_count = 0;
}

void MyID3D10Device::resize_orig_buffers(UINT width, UINT height) {
    orig_size.resize(width, height);
}

void MyID3D10Device::create_sampler(
    D3D10_FILTER filter,
    ID3D10SamplerState *&sampler
) {
    D3D10_SAMPLER_DESC desc = {
        .Filter = filter,
        .AddressU = D3D10_TEXTURE_ADDRESS_CLAMP,
        .AddressV = D3D10_TEXTURE_ADDRESS_CLAMP,
        .AddressW = D3D10_TEXTURE_ADDRESS_CLAMP,
        .MipLODBias = 0,
        .MaxAnisotropy = 16,
        .ComparisonFunc = D3D10_COMPARISON_NEVER,
        .BorderColor = {},
        .MinLOD = 0,
        .MaxLOD = D3D10_FLOAT32_MAX
    };
    inner->CreateSamplerState(&desc, &sampler);
}

void get_resolution_mul(
    UINT &width,
    UINT &height,
    UINT orig_width,
    UINT orig_height
) {
    UINT width_quo = width / orig_width;
    UINT width_rem = width % orig_width;
    if (width_rem) ++width_quo;
    UINT height_quo = height / orig_height;
    UINT height_rem = height % orig_height;
    if (height_rem) ++height_quo;
    UINT mul = std::max(width_quo, height_quo);
    width = orig_width * mul;
    height = orig_height * mul;
}

const DXGI_FORMAT MyID3D10Device::TEX_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;

void MyID3D10Device::create_texture(
    UINT width,
    UINT height,
    ID3D10Texture2D *&texture,
    DXGI_FORMAT format
) {
    D3D10_TEXTURE2D_DESC desc = {
        .Width = width,
        .Height = height,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = format,
        .SampleDesc = {.Count = 1, .Quality = 0},
        .Usage = D3D10_USAGE_DEFAULT,
        .BindFlags = D3D10_BIND_SHADER_RESOURCE | D3D10_BIND_RENDER_TARGET,
        .CPUAccessFlags = 0,
        .MiscFlags = 0
    };
    inner->CreateTexture2D(&desc, NULL, &texture);
}

void MyID3D10Device::create_texture_mul(
    UINT &orig_width,
    UINT &orig_height,
    ID3D10Texture2D *&texture
) {
    UINT width = current_size.render_width;
    UINT height = current_size.render_height;
    get_resolution_mul(width, height, orig_width, orig_height);
    create_texture(width, height, texture);
    orig_width = width;
    orig_height = height;
}

void MyID3D10Device::create_rtv(
    ID3D10Texture2D *tex,
    ID3D10RenderTargetView *&rtv,
    DXGI_FORMAT format
) {
    D3D10_RENDER_TARGET_VIEW_DESC desc = {
        .Format = format,
        .ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D,
    };
    desc.Texture2D.MipSlice = 0;
    inner->CreateRenderTargetView(tex, &desc, &rtv);
}

void MyID3D10Device::create_srv(
    ID3D10Texture2D *tex,
    ID3D10ShaderResourceView *&srv,
    DXGI_FORMAT format
) {
    D3D10_SHADER_RESOURCE_VIEW_DESC desc = {
        .Format = format,
        .ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D,
    };
    D3D10_TEXTURE2D_DESC tex_desc;
    tex->GetDesc(&tex_desc);
    desc.Texture2D.MostDetailedMip = 0;
    desc.Texture2D.MipLevels = tex_desc.MipLevels;
    inner->CreateShaderResourceView(tex, &desc, &srv);
}

void MyID3D10Device::create_dsv(
    ID3D10Texture2D *tex,
    ID3D10DepthStencilView *&dsv,
    DXGI_FORMAT format
) {
    D3D10_DEPTH_STENCIL_VIEW_DESC desc = {
        .Format = format,
        .ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D,
    };
    desc.Texture2D.MipSlice = 0;
    inner->CreateDepthStencilView(tex, &desc, &dsv);
}

void MyID3D10Device::create_tex_and_views_nn(
    TextureAndViews *tex,
    UINT orig_width,
    UINT orig_height
) {
    create_texture_mul(
        orig_width,
        orig_height,
        tex->tex
    );
    create_srv(
        tex->tex,
        tex->srv
    );
    create_rtv(
        tex->tex,
        tex->rtv
    );
    tex->width = orig_width;
    tex->height = orig_height;
}

void MyID3D10Device::create_tex_and_view_1(
    TextureViewsAndBuffer *tex,
    UINT width,
    UINT height,
    UINT orig_width,
    UINT orig_height
) {
    create_texture(width, height, tex->tex);
    create_srv(
        tex->tex,
        tex->srv
    );
    create_rtv(
        tex->tex,
        tex->rtv
    );
    tex->width = width;
    tex->height = height;
    float ps_cb_data[4] = {
        (float)orig_width,
        (float)orig_height,
        (float)(1.0 / orig_width),
        (float)(1.0 / orig_height)
    };
    D3D10_BUFFER_DESC desc = {
        .ByteWidth = sizeof(ps_cb_data),
        .Usage = D3D10_USAGE_IMMUTABLE,
        .BindFlags = D3D10_BIND_CONSTANT_BUFFER,
        .CPUAccessFlags = 0,
        .MiscFlags = 0
    };
    D3D10_SUBRESOURCE_DATA data = {
        .pSysMem = ps_cb_data,
        .SysMemPitch = 0,
        .SysMemSlicePitch = 0
    };
    inner->CreateBuffer(&desc, &data, &tex->ps_cb);
}

void MyID3D10Device::create_tex_and_view_1_v(
    std::vector<TextureViewsAndBuffer *> &tex_v,
    UINT orig_width,
    UINT orig_height
) {
    bool last = false;
    do {
        UINT orig_width_next = orig_width * 2;
        UINT orig_height_next = orig_height * 2;
        last = orig_width_next >= current_size.render_width && orig_height_next >= current_size.render_height;
        TextureViewsAndBuffer *tex = new TextureViewsAndBuffer{};
        create_tex_and_view_1(
            tex,
            orig_width_next,
            orig_height_next,
            orig_width,
            orig_height
        );
        tex_v.push_back(tex);
        orig_width = orig_width_next;
        orig_height = orig_height_next;
    } while (!last);
}

void MyID3D10Device::filter_temp_init() {
    filter_temp_shutdown();
    create_sampler(
        D3D10_FILTER_MIN_MAG_MIP_POINT,
        filter_temp.sampler_nn
    );
    create_sampler(
        D3D10_FILTER_MIN_MAG_MIP_LINEAR,
        filter_temp.sampler_linear
    );
    filter_temp.tex_nn_x1 = new TextureAndViews{};
    create_tex_and_views_nn(
        filter_temp.tex_nn_x1,
        X1_WIDTH,
        X1_HEIGHT
    );
    filter_temp.tex_nn_x4 = new TextureAndViews{};
    create_tex_and_views_nn(
        filter_temp.tex_nn_x4,
        X4_WIDTH,
        X4_HEIGHT
    );
    create_tex_and_view_1_v(
        filter_temp.tex_1_x1,
        X1_WIDTH_FILTERED,
        X1_HEIGHT_FILTERED
    );
    create_tex_and_view_1_v(
        filter_temp.tex_1_x4,
        X4_WIDTH_FILTERED,
        X4_HEIGHT_FILTERED
    );
}

void MyID3D10Device::filter_temp_shutdown() {
    if (filter_temp.sampler_nn) {
        filter_temp.sampler_nn->Release();
        filter_temp.sampler_nn = NULL;
    }
    if (filter_temp.sampler_linear) {
        filter_temp.sampler_linear->Release();
        filter_temp.sampler_linear = NULL;
    }
    if (filter_temp.tex_nn_x1) {
        delete filter_temp.tex_nn_x1;
        filter_temp.tex_nn_x1 = NULL;
    }
    if (filter_temp.tex_nn_x4) {
        delete filter_temp.tex_nn_x4;
        filter_temp.tex_nn_x4 = NULL;
    }
    for (TextureAndViews *tex : filter_temp.tex_1_x1) {
        delete tex;
    }
    filter_temp.tex_1_x1.clear();
    for (TextureAndViews *tex : filter_temp.tex_1_x4) {
        delete tex;
    }
    filter_temp.tex_1_x4.clear();
}

IUNKNOWN_IMPL(MyID3D10Device)

MyID3D10Device::MyID3D10Device(
    ID3D10Device **inner,
    UINT width,
    UINT height
) :
    IUNKNOWN_INIT(*inner)
{
    LOG_MFUN();
    resize_buffers(width, height);
    *inner = this;
}

MyID3D10Device::~MyID3D10Device() {
    LOG_MFUN();
    filter_temp_shutdown();
    clear_filter();
    if (d3d10) my_d3d10_gfx_free(d3d10);
    if (d3d10_3d) my_d3d10_gfx_free(d3d10_3d);
}

void STDMETHODCALLTYPE MyID3D10Device::VSSetConstantBuffers(
    UINT StartSlot,
    UINT NumBuffers,
    ID3D10Buffer *const *ppConstantBuffers
) {
    LOG_MFUN();
    ID3D10Buffer *buffers[NumBuffers];
    for (UINT i = 0; i < NumBuffers; ++i) {
        buffers[i] = ppConstantBuffers[i] ? ((MyID3D10Buffer *)ppConstantBuffers[i])->inner : NULL;
    }
    if (NumBuffers) {
        memcpy(current_vscbs + StartSlot, buffers, sizeof(buffers[0]) * NumBuffers);
    }
    inner->VSSetConstantBuffers(
        StartSlot,
        NumBuffers,
        NumBuffers ? buffers : ppConstantBuffers
    );
}

void STDMETHODCALLTYPE MyID3D10Device::PSSetShaderResources(
    UINT StartSlot,
    UINT NumViews,
    ID3D10ShaderResourceView *const *ppShaderResourceViews
) {
    LOG_MFUN(_,
        LOG_ARG(StartSlot),
        LOG_ARG(NumViews),
        LOG_ARG_TYPE(ppShaderResourceViews, ArrayLoggerDeref, NumViews)
    );
    if (NumViews) current_pssrv = ppShaderResourceViews ? (MyID3D10ShaderResourceView *)*ppShaderResourceViews : NULL;
    ID3D10ShaderResourceView *srvs[NumViews];
    for (UINT i = 0; i < NumViews; ++i) {
        MyID3D10ShaderResourceView *my_srv = (MyID3D10ShaderResourceView *)ppShaderResourceViews[i];
        srvs[i] = my_srv ? my_srv->inner : NULL;
    }
    if (NumViews) {
        memcpy(current_pssrvs + StartSlot, srvs, sizeof(srvs[0]) * NumViews);
    }
    inner->PSSetShaderResources(
        StartSlot,
        NumViews,
        NumViews ? srvs : ppShaderResourceViews
    );
}

void STDMETHODCALLTYPE MyID3D10Device::PSSetShader(
    ID3D10PixelShader *pPixelShader
) {
    LOG_MFUN(_,
        LOG_ARG(pPixelShader)
    );
    current_ps = (MyID3D10PixelShader *)pPixelShader;
    inner->PSSetShader(current_ps->inner);
}

void STDMETHODCALLTYPE MyID3D10Device::PSSetSamplers(
    UINT StartSlot,
    UINT NumSamplers,
    ID3D10SamplerState *const *ppSamplers
) {
    LOG_MFUN(_,
        LOG_ARG(StartSlot),
        LOG_ARG(NumSamplers),
        LOG_ARG_TYPE(ppSamplers, ArrayLoggerDeref, NumSamplers)
    );
    if (NumSamplers) current_psss = ppSamplers ? (MyID3D10SamplerState *)*ppSamplers : NULL;
    ID3D10SamplerState *sss[NumSamplers];
    for (UINT i = 0; i < NumSamplers; ++i) {
        sss[i] = ppSamplers[i] ? ((MyID3D10SamplerState *)ppSamplers[i])->inner : NULL;
    }
    if (NumSamplers) {
        memcpy(current_pssss + StartSlot, sss, sizeof(sss[0]) * NumSamplers);
    }
    inner->PSSetSamplers(
        StartSlot,
        NumSamplers,
        NumSamplers ? sss : ppSamplers
    );
}

void STDMETHODCALLTYPE MyID3D10Device::VSSetShader(
    ID3D10VertexShader *pVertexShader
) {
    LOG_MFUN(_,
        LOG_ARG(pVertexShader)
    );
    current_vs = pVertexShader;
    inner->VSSetShader(pVertexShader);
}

void STDMETHODCALLTYPE MyID3D10Device::DrawIndexed(
    UINT IndexCount,
    UINT StartIndexLocation,
    INT BaseVertexLocation
) {
    LOG_MFUN();
    set_render_vp();
    inner->DrawIndexed(
        IndexCount,
        StartIndexLocation,
        BaseVertexLocation
    );
}

void STDMETHODCALLTYPE MyID3D10Device::Draw(
    UINT VertexCount,
    UINT StartVertexLocation
) {
    LOG_MFUN(_,
        LOG_ARG(VertexCount),
        LOG_ARG(StartVertexLocation)
    );
    set_render_vp();
    bool filter_next = false;
    bool filter_ss = false;
    bool filter_ss_3d = false;
if constexpr (ENABLE_SLANG_SHADER) {
    filter_ss = d3d10 && d3d10->shader_preset;
    filter_ss_3d = d3d10_3d && d3d10_3d->shader_preset;
}
    bool x8 = filter_ss_3d || render_3d_width != current_size.render_3d_width || render_3d_height != current_size.render_3d_height;
    if (
        !config.interp &&
        !filter_ss &&
        !filter_ss_3d &&
        !x8
    ) goto end;

    if (VertexCount != 4) goto end;
{
    MyID3D10SamplerState *psss = current_psss;
    if (!psss) goto end;
    filter_next = filter && psss == filter_state.psss;

    D3D10_SAMPLER_DESC &sampler_desc = psss->desc;

    if (
        sampler_desc.Filter != D3D10_FILTER_MIN_MAG_MIP_POINT ||
        sampler_desc.AddressU != D3D10_TEXTURE_ADDRESS_CLAMP ||
        sampler_desc.AddressV != D3D10_TEXTURE_ADDRESS_CLAMP ||
        sampler_desc.AddressW != D3D10_TEXTURE_ADDRESS_CLAMP ||
        sampler_desc.ComparisonFunc != D3D10_COMPARISON_NEVER
    ) goto end;

    MyID3D10ShaderResourceView *srv = current_pssrv;
    if (!srv) goto end;

    D3D10_SHADER_RESOURCE_VIEW_DESC &srv_desc = srv->desc;
    if (srv_desc.ViewDimension != D3D10_SRV_DIMENSION_TEXTURE2D) goto end;

    MyID3D10Texture2D *srv_tex = (MyID3D10Texture2D *)srv->resource;

    filter_next = filter_next && srv_tex == filter_state.rtv_tex;
    bool x4 = false;
    D3D10_TEXTURE2D_DESC &srv_tex_desc = srv_tex->desc;
    if (!filter_next) {
        if (
            srv_tex_desc.Width == X1_WIDTH &&
            srv_tex_desc.Height == X1_HEIGHT
        ) { // SNES
        } else if (
            srv_tex_desc.Width == X4_WIDTH &&
            srv_tex_desc.Height == X4_HEIGHT
        ) { // PlayStation
            x4 = true;
        } else if (
            x8 &&
            srv_tex->orig_width == orig_size.render_3d_width &&
            srv_tex->orig_height == orig_size.render_3d_height
        ) { // PlayStation 2
        } else {
             goto end;
        }
    }

    MyID3D10RenderTargetView *rtv = current_rtv;
    if (!rtv) goto end;

    D3D10_RENDER_TARGET_VIEW_DESC &rtv_desc = rtv->desc;
    if (rtv_desc.ViewDimension != D3D10_RTV_DIMENSION_TEXTURE2D) goto end;

    MyID3D10Texture2D *rtv_tex = (MyID3D10Texture2D *)rtv->resource;

    D3D10_TEXTURE2D_DESC &rtv_tex_desc = rtv_tex->desc;

    if (filter_next || x8) {
        if (
            rtv_tex->orig_width != orig_size.sc_width ||
            rtv_tex->orig_height != orig_size.sc_height
        ) goto end;

        auto set_filter_state_ps = [&] {
            inner->PSSetShader(filter_state.ps->inner);
        };
        auto restore_ps = [&] {
            inner->PSSetShader(current_ps->inner);
        };
        auto restore_vps = [&] {
            inner->RSSetViewports(1, &current_vp);
        };
        auto restore_pscbs = [&] {
            inner->PSSetConstantBuffers(0, 1, current_pscbs);
        };
        auto restore_rtvs = [&] {
            inner->OMSetRenderTargets(
                1,
                &rtv->inner,
                current_dsv->inner
            );
        };
        auto restore_pssrvs = [&] {
            inner->PSSetShaderResources(
                0,
                MAX_SHADER_RESOURCES,
                current_pssrvs
            );
        };
        auto restore_pssss = [&] {
            inner->PSSetSamplers(0, MAX_SAMPLERS, current_pssss);
        };

        auto set_viewport = [&](UINT width, UINT height) {
            D3D10_VIEWPORT viewport = {
                .TopLeftX = 0,
                .TopLeftY = 0,
                .Width = width,
                .Height = height,
                .MinDepth = 0,
                .MaxDepth = 1,
            };
            inner->RSSetViewports(1, &viewport);
        };
        auto set_srv = [&](ID3D10ShaderResourceView *srv) {
            inner->PSSetShaderResources(
                0,
                1,
                &srv
            );
        };
        auto set_rtv = [&](ID3D10RenderTargetView *rtv) {
            inner->OMSetRenderTargets(
                1,
                &rtv,
                NULL
            );
        };
        auto set_psss = [&](ID3D10SamplerState *psss) {
            inner->PSSetSamplers(0, 1, &psss);
        };

        auto draw_enhanced = [&](std::vector<TextureViewsAndBuffer *> &v_v) {
            auto v_it = v_v.begin();
            if (v_it != v_v.end()) {
                set_filter_state_ps();
                auto set_it_viewport = [&] {
                    set_viewport((*v_it)->width, (*v_it)->height);
                };
                set_it_viewport();
                auto set_it_rtv = [&] {
                    set_rtv((*v_it)->rtv);
                };
                set_it_rtv();
                auto set_it_pscbs = [&] {
                    inner->PSSetConstantBuffers(0, 1, &(*v_it)->ps_cb);
                };
                set_it_pscbs();
                inner->Draw(VertexCount, filter_state.start_vertex_location);
                auto v_it_prev = v_it;
                auto set_it_prev_srv = [&] {
                    set_srv((*v_it_prev)->srv);
                };
                while (++v_it != v_v.end()) {
                    set_it_rtv();
                    set_it_prev_srv();
                    set_it_viewport();
                    set_it_pscbs();
                    inner->Draw(VertexCount, filter_state.start_vertex_location);
                    v_it_prev = v_it;
                }
                restore_ps();
                restore_vps();
                restore_rtvs();
                restore_pscbs();
                set_it_prev_srv();
                set_psss(filter_temp.sampler_linear);
                inner->Draw(VertexCount, StartVertexLocation);
                restore_pssrvs();
                restore_pssss();
            } else {
                set_psss(filter_temp.sampler_linear);
                inner->Draw(VertexCount, StartVertexLocation);
                restore_pssss();
            }
        };
        auto draw_nn = [&](TextureAndViews *v) {
            set_viewport(v->width, v->height);
            set_rtv(v->rtv);
            set_srv(filter_state.srv->inner);
            inner->Draw(VertexCount, filter_state.start_vertex_location);
            restore_vps();
            restore_rtvs();
            set_srv(v->srv);
            set_psss(filter_temp.sampler_linear);
            inner->Draw(VertexCount, StartVertexLocation);
            restore_pssrvs();
            restore_pssss();
        };
        auto draw_ss = [&](d3d10_video_t *d3d10, MyID3D10ShaderResourceView *srv, D3D10_VIEWPORT *current_vp, TextureAndViews *current_tex = NULL) {
            if (current_tex) {
                video_viewport_t vp = {
                    .x = 0,
                    .y = 0,
                    .width = current_tex->width,
                    .height = current_tex->height,
                    .full_width = current_tex->width,
                    .full_height = current_tex->height
                };
                my_d3d10_update_viewport(d3d10, current_tex->rtv, &vp);
            } else {
                video_viewport_t vp = {
                    .x = current_vp->TopLeftX,
                    .y = current_vp->TopLeftY,
                    .width = current_vp->Width,
                    .height = current_vp->Height,
                    .full_width = current_size.sc_width,
                    .full_height = current_size.sc_height
                };
                my_d3d10_update_viewport(d3d10, rtv->inner, &vp);
            }
            MyID3D10Texture2D *my_texture = (MyID3D10Texture2D *)srv->resource;
            d3d10_texture_t texture = {};
            texture.handle = my_texture->inner;
            texture.desc = my_texture->desc;
            my_d3d10_gfx_frame(d3d10, &texture, frame_count);

            inner->IASetPrimitiveTopology(current_pt);
            inner->IASetVertexBuffers(
                0,
                D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT,
                current_vbs.ppVertexBuffers,
                current_vbs.pStrides,
                current_vbs.pOffsets
            );
            inner->OMSetBlendState(
                current_bs.pBlendState,
                current_bs.BlendFactor,
                current_bs.SampleMask
            );
            inner->PSSetConstantBuffers(0, MAX_CONSTANT_BUFFERS, current_pscbs);
            inner->VSSetConstantBuffers(0, MAX_CONSTANT_BUFFERS, current_vscbs);
            inner->IASetInputLayout(current_il);
            restore_ps();
            inner->VSSetShader(current_vs);
            inner->GSSetShader(current_gs);
            restore_vps();
            restore_rtvs();

            if (current_tex) {
                set_psss(filter_temp.sampler_linear);
                set_srv(current_tex->srv);
                inner->Draw(VertexCount, StartVertexLocation);
            }

            restore_pssss();
            restore_pssrvs();
        };

        if (x8) {
            if (filter_ss_3d) {
                D3D10_VIEWPORT vp = {
                    .TopLeftX = (INT)(current_size.sc_width - current_size.render_3d_width) / 2,
                    .TopLeftY = 0,
                    .Width = current_size.render_3d_width,
                    .Height = current_size.render_3d_height
                };
                draw_ss(d3d10_3d, srv, &vp);
            } else {
                set_psss(filter_temp.sampler_linear);
                inner->Draw(VertexCount, StartVertexLocation);
                restore_pssss();
            }
            filter_next = true;
            goto end;
        } else if (filter_state.t1) {
            if (filter_ss) {
                draw_ss(d3d10, filter_state.srv, &current_vp, config.interp ? filter_state.x4 ? filter_temp.tex_nn_x4 : filter_temp.tex_nn_x1 : NULL);
            } else if (config.enhanced) {
                if (filter_state.x4) {
                    draw_enhanced(filter_temp.tex_1_x4);
                } else {
                    draw_enhanced(filter_temp.tex_1_x1);
                }
            } else {
                set_psss(filter_temp.sampler_linear);
                inner->Draw(VertexCount, StartVertexLocation);
                restore_pssss();
            }
        } else {
            if (filter_state.x4) {
                draw_nn(filter_temp.tex_nn_x4);
            } else {
                draw_nn(filter_temp.tex_nn_x1);
            }
        }
        clear_filter();
    } else {
        if (
            rtv_tex_desc.Width != srv_tex_desc.Width * 2 ||
            rtv_tex_desc.Height != srv_tex_desc.Height * 2
        ) goto end;

        clear_filter();
        filter = true;
        filter_state.rtv_tex = rtv_tex; rtv_tex->AddRef();
        if (current_ps) {
            filter_state.ps = current_ps; current_ps->AddRef();
            switch (current_ps->bytecode_hash) {
                case 0xa54c4b2: filter_state.t1 = true; break;
                case 0x1f4c05ac: filter_state.t1 = false; break;
                default: filter_state.t1 = current_ps->bytecode_length >= BYTECODE_LENGTH_THRESHOLD; break;
            }
        }
        if (filter_ss || !filter_state.t1) {
            filter_next = true;
            filter_state.srv = srv; srv->AddRef();
        }
        filter_state.psss = psss; psss->AddRef();
        filter_state.x4 = x4;
        filter_state.start_vertex_location = StartVertexLocation;
    }
}
end:

    if (!filter_next) inner->Draw(VertexCount, StartVertexLocation);
}

void STDMETHODCALLTYPE MyID3D10Device::PSSetConstantBuffers(
    UINT StartSlot,
    UINT NumBuffers,
    ID3D10Buffer *const *ppConstantBuffers
) {
    LOG_MFUN(_,
        LOG_ARG(StartSlot),
        LOG_ARG(NumBuffers),
        LOG_ARG_TYPE(ppConstantBuffers, ArrayLoggerDeref, NumBuffers)
    );
    ID3D10Buffer *buffers[NumBuffers];
    for (UINT i = 0; i < NumBuffers; ++i) {
        buffers[i] = ppConstantBuffers[i] ? ((MyID3D10Buffer *)ppConstantBuffers[i])->inner : NULL;
    }
    if (NumBuffers) {
        memcpy(current_pscbs + StartSlot, buffers, sizeof(buffers[0]) * NumBuffers);
    }
    inner->PSSetConstantBuffers(
        StartSlot,
        NumBuffers,
        NumBuffers ? buffers : ppConstantBuffers
    );
}

void STDMETHODCALLTYPE MyID3D10Device::IASetInputLayout(
    ID3D10InputLayout *pInputLayout
) {
    LOG_MFUN(_,
        LOG_ARG(pInputLayout)
    );
    current_il = pInputLayout;
    inner->IASetInputLayout(pInputLayout);
}

void STDMETHODCALLTYPE MyID3D10Device::IASetVertexBuffers(
    UINT StartSlot,
    UINT NumBuffers,
    ID3D10Buffer *const *ppVertexBuffers,
    const UINT *pStrides,
    const UINT *pOffsets
) {
    LOG_MFUN(_,
        LOG_ARG(StartSlot),
        LOG_ARG(NumBuffers),
        LOG_ARG_TYPE(ppVertexBuffers, ArrayLoggerDeref, NumBuffers),
        LOG_ARG_TYPE(pStrides, ArrayLoggerDeref, NumBuffers),
        LOG_ARG_TYPE(pOffsets, ArrayLoggerDeref, NumBuffers)
    );
    ID3D10Buffer *buffers[NumBuffers];
    for (UINT i = 0; i < NumBuffers; ++i) {
        buffers[i] = ppVertexBuffers[i] ? ((MyID3D10Buffer *)ppVertexBuffers[i])->inner : NULL;
    }
if constexpr (ENABLE_SLANG_SHADER) {
    if (NumBuffers) {
        memcpy(current_vbs.ppVertexBuffers + StartSlot, buffers, sizeof(buffers[0]) * NumBuffers);
        memcpy(current_vbs.pStrides + StartSlot, pStrides, sizeof(pStrides[0]) * NumBuffers);
        memcpy(current_vbs.pOffsets + StartSlot, pOffsets, sizeof(pOffsets[0]) * NumBuffers);
    }
}
    inner->IASetVertexBuffers(
        StartSlot,
        NumBuffers,
        NumBuffers ? buffers : ppVertexBuffers,
        pStrides,
        pOffsets
    );
}

void STDMETHODCALLTYPE MyID3D10Device::IASetIndexBuffer(
    ID3D10Buffer *pIndexBuffer,
    DXGI_FORMAT Format,
    UINT Offset
) {
    LOG_MFUN(_,
        LOG_ARG(pIndexBuffer),
        LOG_ARG(Format),
        LOG_ARG(Offset)
    );
    inner->IASetIndexBuffer(
        pIndexBuffer ? ((MyID3D10Buffer *)pIndexBuffer)->inner : NULL,
        Format,
        Offset
    );
}

void STDMETHODCALLTYPE MyID3D10Device::DrawIndexedInstanced(
    UINT IndexCountPerInstance,
    UINT InstanceCount,
    UINT StartIndexLocation,
    INT BaseVertexLocation,
    UINT StartInstanceLocation
) {
    LOG_MFUN();
    set_render_vp();
    inner->DrawIndexedInstanced(
        IndexCountPerInstance,
        InstanceCount,
        StartIndexLocation,
        BaseVertexLocation,
        StartInstanceLocation
    );
}

void STDMETHODCALLTYPE MyID3D10Device::DrawInstanced(
    UINT VertexCountPerInstance,
    UINT InstanceCount,
    UINT StartVertexLocation,
    UINT StartInstanceLocation
) {
    LOG_MFUN();
    set_render_vp();
    inner->DrawInstanced(
        VertexCountPerInstance,
        InstanceCount,
        StartVertexLocation,
        StartInstanceLocation
    );
}

void STDMETHODCALLTYPE MyID3D10Device::GSSetConstantBuffers(
    UINT StartSlot,
    UINT NumBuffers,
    ID3D10Buffer *const *ppConstantBuffers
) {
    LOG_MFUN();
    ID3D10Buffer *buffers[NumBuffers];
    for (UINT i = 0; i < NumBuffers; ++i) {
        buffers[i] = ppConstantBuffers[i] ? ((MyID3D10Buffer *)ppConstantBuffers[i])->inner : NULL;
    }
    inner->GSSetConstantBuffers(
        StartSlot,
        NumBuffers,
        NumBuffers ? buffers : ppConstantBuffers
    );
}

void STDMETHODCALLTYPE MyID3D10Device::GSSetShader(
    ID3D10GeometryShader *pShader
) {
    LOG_MFUN(_,
        LOG_ARG(pShader)
    );
    current_gs = pShader;
    inner->GSSetShader(pShader);
}

void STDMETHODCALLTYPE MyID3D10Device::IASetPrimitiveTopology(
    D3D10_PRIMITIVE_TOPOLOGY Topology
) {
    LOG_MFUN(_,
        LOG_ARG(Topology)
    );
    inner->IASetPrimitiveTopology(Topology);
if constexpr (ENABLE_SLANG_SHADER) {
    current_pt = Topology;
}
}

void STDMETHODCALLTYPE MyID3D10Device::VSSetShaderResources(
    UINT StartSlot,
    UINT NumViews,
    ID3D10ShaderResourceView *const *ppShaderResourceViews
) {
    LOG_MFUN(_,
        LOG_ARG(StartSlot),
        LOG_ARG(NumViews),
        LOG_ARG_TYPE(ppShaderResourceViews, ArrayLoggerDeref, NumViews)
    );
    ID3D10ShaderResourceView *srvs[NumViews];
    for (UINT i = 0; i < NumViews; ++i) {
        MyID3D10ShaderResourceView *my_srv = (MyID3D10ShaderResourceView *)ppShaderResourceViews[i];
        srvs[i] = my_srv ? my_srv->inner : NULL;
    }
    inner->VSSetShaderResources(
        StartSlot,
        NumViews,
        NumViews ? srvs : ppShaderResourceViews
    );
}

void STDMETHODCALLTYPE MyID3D10Device::VSSetSamplers(
    UINT StartSlot,
    UINT NumSamplers,
    ID3D10SamplerState *const *ppSamplers
) {
    LOG_MFUN();
    ID3D10SamplerState *sss[NumSamplers];
    for (UINT i = 0; i < NumSamplers; ++i) {
        sss[i] = ppSamplers[i] ? ((MyID3D10SamplerState *)ppSamplers[i])->inner : NULL;
    }
    inner->VSSetSamplers(
        StartSlot,
        NumSamplers,
        NumSamplers ? sss : ppSamplers
    );
}

void STDMETHODCALLTYPE MyID3D10Device::SetPredication(
    ID3D10Predicate *pPredicate,
    WINBOOL PredicateValue
) {
    LOG_MFUN();
    inner->SetPredication(
        pPredicate,
        PredicateValue
    );
}

void STDMETHODCALLTYPE MyID3D10Device::GSSetShaderResources(
    UINT StartSlot,
    UINT NumViews,
    ID3D10ShaderResourceView *const *ppShaderResourceViews
) {
    LOG_MFUN(_,
        LOG_ARG(StartSlot),
        LOG_ARG(NumViews),
        LOG_ARG_TYPE(ppShaderResourceViews, ArrayLoggerDeref, NumViews)
    );
    ID3D10ShaderResourceView *srvs[NumViews];
    for (UINT i = 0; i < NumViews; ++i) {
        MyID3D10ShaderResourceView *my_srv = (MyID3D10ShaderResourceView *)ppShaderResourceViews[i];
        srvs[i] = my_srv ? my_srv->inner : NULL;
    }
    inner->GSSetShaderResources(
        StartSlot,
        NumViews,
        NumViews ? srvs : ppShaderResourceViews
    );
}

void STDMETHODCALLTYPE MyID3D10Device::GSSetSamplers(
    UINT StartSlot,
    UINT NumSamplers,
    ID3D10SamplerState *const *ppSamplers
) {
    LOG_MFUN();
    ID3D10SamplerState *sss[NumSamplers];
    for (UINT i = 0; i < NumSamplers; ++i) {
        sss[i] = ppSamplers[i] ? ((MyID3D10SamplerState *)ppSamplers[i])->inner : NULL;
    }
    inner->GSSetSamplers(
        StartSlot,
        NumSamplers,
        NumSamplers ? sss : ppSamplers
    );
}

bool MyID3D10Device::get_render_tex_views(
    MyID3D10Texture2D *tex,
    UINT width,
    UINT height,
    UINT orig_width,
    UINT orig_height
) {
    bool ret = false;
if constexpr (ENABLE_CUSTOM_RESOLUTION) {
    D3D10_TEXTURE2D_DESC desc = tex->desc;
    if (tex->orig_width == orig_width && tex->orig_height == orig_height) {
        ret = true;
        if (desc.Width != width || desc.Height != height) {
            if (tex->sc) {
                ret = false;
            } else {
                desc.Width = width;
                desc.Height = height;
                tex->inner->Release();
                inner->CreateTexture2D(&desc, NULL, &tex->inner);

                for (MyID3D10RenderTargetView *rtv : tex->rtvs) {
                    current_rtvs_map.erase(rtv->inner);
                    rtv->inner->Release();
                    inner->CreateRenderTargetView(tex->inner, &rtv->desc, &rtv->inner);
                    current_rtvs_map.emplace(rtv->inner, rtv);
                }
                for (MyID3D10ShaderResourceView *srv : tex->srvs) {
                    current_srvs_map.erase(srv->inner);
                    srv->inner->Release();
                    inner->CreateShaderResourceView(tex->inner, &srv->desc, &srv->inner);
                    current_srvs_map.emplace(srv->inner, srv);
                }
                for (MyID3D10DepthStencilView *dsv : tex->dsvs) {
                    current_dsvs_map.erase(dsv->inner);
                    dsv->inner->Release();
                    inner->CreateDepthStencilView(tex->inner, &dsv->desc, &dsv->inner);
                    current_dsvs_map.emplace(dsv->inner, dsv);
                }

                tex->desc = desc;
            }
        }

        if (ret && (width != orig_width || height != orig_height)) {
            render_width = width;
            render_height = height;
            render_orig_width = orig_width;
            render_orig_height = orig_height;
            need_current_vp = true;
        }
    }
}
    return ret;
}

void MyID3D10Device::set_render_vp() {
if constexpr (ENABLE_CUSTOM_RESOLUTION) {
    if (need_current_vp && !is_current_vp) {
        if (current_vp.Width && current_vp.Height) {
            D3D10_VIEWPORT vp = {
                .TopLeftX = (INT)(current_vp.TopLeftX * render_width / render_orig_width),
                .TopLeftY = (INT)(current_vp.TopLeftY * render_height / render_orig_height),
                .Width = current_vp.Width * render_width / render_orig_width,
                .Height = current_vp.Height * render_height / render_orig_height,
                .MinDepth = current_vp.MinDepth,
                .MaxDepth = current_vp.MaxDepth,
            };
            orig_vp = current_vp;
            current_vp = vp;
            inner->RSSetViewports(1, &current_vp);
        }
        is_current_vp = true;
    }
}
}

void MyID3D10Device::reset_render_vp() {
if constexpr (ENABLE_CUSTOM_RESOLUTION) {
    if (need_current_vp && is_current_vp) {
        if (current_vp.Width && current_vp.Height) {
            current_vp = orig_vp;
            orig_vp = {};
            inner->RSSetViewports(1, &current_vp);
        }
        is_current_vp = false;
    }
}
}

#define GET_RENDER_TEX_VIEWS(v) do { \
    MyID3D10Texture2D *tex = (MyID3D10Texture2D *)v->resource; \
    get_render_tex_views( \
        tex, \
        current_size.sc_width, \
        current_size.sc_height, \
        orig_size.sc_width, \
        orig_size.sc_height \
    ) || \
    get_render_tex_views( \
        tex, \
        render_3d_width, \
        render_3d_height, \
        orig_size.render_3d_width, \
        orig_size.render_3d_height \
    ); \
} while (0)

void STDMETHODCALLTYPE MyID3D10Device::OMSetRenderTargets(
    UINT NumViews,
    ID3D10RenderTargetView *const *ppRenderTargetViews,
    ID3D10DepthStencilView *pDepthStencilView
) {
    LOG_MFUN(_,
        LOG_ARG(NumViews),
        LOG_ARG_TYPE(ppRenderTargetViews, ArrayLoggerDeref, NumViews),
        LOG_ARG(pDepthStencilView)
    );
    reset_render_vp();
    need_current_vp = false;
    current_rtv = NumViews && ppRenderTargetViews ? (MyID3D10RenderTargetView *)*ppRenderTargetViews : NULL;
    current_dsv = (MyID3D10DepthStencilView *)pDepthStencilView;
    ID3D10RenderTargetView *rtvs[NumViews];
    for (UINT i = 0; i < NumViews; ++i) {
        MyID3D10RenderTargetView *my_rtv = (MyID3D10RenderTargetView *)ppRenderTargetViews[i];
        if (my_rtv && my_rtv->desc.ViewDimension == D3D10_RTV_DIMENSION_TEXTURE2D) {
            GET_RENDER_TEX_VIEWS(my_rtv);
        }
        rtvs[i] = my_rtv ? my_rtv->inner : NULL;
    }
    if (current_dsv && current_dsv->desc.ViewDimension == D3D10_DSV_DIMENSION_TEXTURE2D) {
        GET_RENDER_TEX_VIEWS(current_dsv);
    }
    inner->OMSetRenderTargets(
        NumViews,
        NumViews ? rtvs : ppRenderTargetViews,
        current_dsv ? current_dsv->inner : NULL
    );
}

void STDMETHODCALLTYPE MyID3D10Device::OMSetBlendState(
    ID3D10BlendState *pBlendState,
    const FLOAT BlendFactor[4],
    UINT SampleMask
) {
    LOG_MFUN();
    inner->OMSetBlendState(
        pBlendState,
        BlendFactor,
        SampleMask
    );
if constexpr (ENABLE_SLANG_SHADER) {
    current_bs.pBlendState = pBlendState;
    current_bs.BlendFactor[0] = BlendFactor[0];
    current_bs.BlendFactor[1] = BlendFactor[1];
    current_bs.BlendFactor[2] = BlendFactor[2];
    current_bs.BlendFactor[3] = BlendFactor[3];
    current_bs.SampleMask = SampleMask;
}
}

void STDMETHODCALLTYPE MyID3D10Device::OMSetDepthStencilState(
    ID3D10DepthStencilState *pDepthStencilState,
    UINT StencilRef
) {
    LOG_MFUN();
    inner->OMSetDepthStencilState(
        pDepthStencilState,
        StencilRef
    );
}

void STDMETHODCALLTYPE MyID3D10Device::SOSetTargets(
    UINT NumBuffers,
    ID3D10Buffer *const *ppSOTargets,
    const UINT *pOffsets
) {
    LOG_MFUN();
    ID3D10Buffer *buffers[NumBuffers];
    for (UINT i = 0; i < NumBuffers; ++i) {
        buffers[i] = ppSOTargets[i] ? ((MyID3D10Buffer *)ppSOTargets[i])->inner : NULL;
    }
    inner->SOSetTargets(
        NumBuffers,
        NumBuffers ? buffers : ppSOTargets,
        pOffsets
    );
}

void STDMETHODCALLTYPE MyID3D10Device::DrawAuto(
) {
    LOG_MFUN();
    set_render_vp();
    inner->DrawAuto(
    );
}

void STDMETHODCALLTYPE MyID3D10Device::RSSetState(
    ID3D10RasterizerState *pRasterizerState
) {
    LOG_MFUN();
    inner->RSSetState(
        pRasterizerState
    );
}

void STDMETHODCALLTYPE MyID3D10Device::RSSetViewports(
    UINT NumViewports,
    const D3D10_VIEWPORT *pViewports
) {
    LOG_MFUN(_,
        LOG_ARG(NumViewports),
        LOG_ARG_TYPE(pViewports, ArrayLoggerRef, NumViewports)
    );
    if (NumViewports) {
        current_vp = *pViewports;
    } else {
        current_vp = {};
    }
    orig_vp = {};
    is_current_vp = false;
    inner->RSSetViewports(
        NumViewports,
        pViewports
    );
}

void STDMETHODCALLTYPE MyID3D10Device::RSSetScissorRects(
    UINT NumRects,
    const D3D10_RECT *pRects
) {
    LOG_MFUN();
    inner->RSSetScissorRects(
        NumRects,
        pRects
    );
}

void STDMETHODCALLTYPE MyID3D10Device::CopySubresourceRegion(
    ID3D10Resource *pDstResource,
    UINT DstSubresource,
    UINT DstX,
    UINT DstY,
    UINT DstZ,
    ID3D10Resource *pSrcResource,
    UINT SrcSubresource,
    const D3D10_BOX *pSrcBox
) {
    LOG_MFUN(_,
        LOG_ARG(pDstResource),
        LOG_ARG(DstSubresource),
        LOG_ARG(DstX),
        LOG_ARG(DstY),
        LOG_ARG(DstZ),
        LOG_ARG(pSrcResource),
        LOG_ARG(SrcSubresource),
        LOG_ARG(pSrcBox)
    );
    D3D10_RESOURCE_DIMENSION dstType;
    pDstResource->GetType(&dstType);
    D3D10_RESOURCE_DIMENSION srcType;
    pSrcResource->GetType(&srcType);
    if (dstType != srcType) return;
    switch (dstType) {
        case D3D10_RESOURCE_DIMENSION_BUFFER:
            pDstResource = ((MyID3D10Buffer *)pDstResource)->inner;
            pSrcResource = ((MyID3D10Buffer *)pSrcResource)->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
            pDstResource = ((MyID3D10Texture1D *)pDstResource)->inner;
            pSrcResource = ((MyID3D10Texture1D *)pSrcResource)->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
            pDstResource = ((MyID3D10Texture2D *)pDstResource)->inner;
            pSrcResource = ((MyID3D10Texture2D *)pSrcResource)->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
            pDstResource = ((MyID3D10Texture3D *)pDstResource)->inner;
            pSrcResource = ((MyID3D10Texture3D *)pSrcResource)->inner;
            break;

        default:
            break;
    }
    inner->CopySubresourceRegion(
        pDstResource,
        DstSubresource,
        DstX,
        DstY,
        DstZ,
        pSrcResource,
        SrcSubresource,
        pSrcBox
    );
}

void STDMETHODCALLTYPE MyID3D10Device::CopyResource(
    ID3D10Resource *pDstResource,
    ID3D10Resource *pSrcResource
) {
    LOG_MFUN(_,
        LOG_ARG(pDstResource),
        LOG_ARG(pSrcResource)
    );
    D3D10_RESOURCE_DIMENSION dstType;
    pDstResource->GetType(&dstType);
    D3D10_RESOURCE_DIMENSION srcType;
    pSrcResource->GetType(&srcType);
    if (dstType != srcType) return;
    switch (dstType) {
        case D3D10_RESOURCE_DIMENSION_BUFFER:
            pDstResource = ((MyID3D10Buffer *)pDstResource)->inner;
            pSrcResource = ((MyID3D10Buffer *)pSrcResource)->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
            pDstResource = ((MyID3D10Texture1D *)pDstResource)->inner;
            pSrcResource = ((MyID3D10Texture1D *)pSrcResource)->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
            pDstResource = ((MyID3D10Texture2D *)pDstResource)->inner;
            pSrcResource = ((MyID3D10Texture2D *)pSrcResource)->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
            pDstResource = ((MyID3D10Texture3D *)pDstResource)->inner;
            pSrcResource = ((MyID3D10Texture3D *)pSrcResource)->inner;
            break;

        default:
            break;
    }
    inner->CopyResource(
        pDstResource,
        pSrcResource
    );
}

void STDMETHODCALLTYPE MyID3D10Device::UpdateSubresource(
    ID3D10Resource *pDstResource,
    UINT DstSubresource,
    const D3D10_BOX *pDstBox,
    const void *pSrcData,
    UINT SrcRowPitch,
    UINT SrcDepthPitch
) {
    LOG_MFUN(_,
        LOG_ARG(pDstResource),
        LOG_ARG(DstSubresource)
    );
    D3D10_RESOURCE_DIMENSION dstType;
    pDstResource->GetType(&dstType);
    switch (dstType) {
        case D3D10_RESOURCE_DIMENSION_BUFFER:
            pDstResource = ((MyID3D10Buffer *)pDstResource)->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
            pDstResource = ((MyID3D10Texture1D *)pDstResource)->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
            pDstResource = ((MyID3D10Texture2D *)pDstResource)->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
            pDstResource = ((MyID3D10Texture3D *)pDstResource)->inner;
            break;

        default:
            break;
    }
    inner->UpdateSubresource(
        pDstResource,
        DstSubresource,
        pDstBox,
        pSrcData,
        SrcRowPitch,
        SrcDepthPitch
    );
}

void STDMETHODCALLTYPE MyID3D10Device::ClearRenderTargetView(
    ID3D10RenderTargetView *pRenderTargetView,
    const FLOAT ColorRGBA[4]
) {
    LOG_MFUN(_,
        LOG_ARG(pRenderTargetView)
    );
    inner->ClearRenderTargetView(
        ((MyID3D10RenderTargetView *)pRenderTargetView)->inner,
        ColorRGBA
    );
}

void STDMETHODCALLTYPE MyID3D10Device::ClearDepthStencilView(
    ID3D10DepthStencilView *pDepthStencilView,
    UINT ClearFlags,
    FLOAT Depth,
    UINT8 Stencil
) {
    if (LOG_MFUN_BEGIN(_,
        LOG_ARG(pDepthStencilView),
        LOG_ARG_TYPE(ClearFlags, D3D10_CLEAR_Logger)
    )) {
        if (ClearFlags & D3D10_CLEAR_DEPTH) {
            LOG_FUN_ARGS_NEXT(LOG_ARG(Depth));
        }
        if (ClearFlags & D3D10_CLEAR_STENCIL) {
            LOG_FUN_ARGS_NEXT(LOG_ARG(+Stencil));
        }
        LOG_FUN_END();
    }
    inner->ClearDepthStencilView(
        pDepthStencilView ? ((MyID3D10DepthStencilView *)pDepthStencilView)->inner : NULL,
        ClearFlags,
        Depth,
        Stencil
    );
}

void STDMETHODCALLTYPE MyID3D10Device::GenerateMips(
    ID3D10ShaderResourceView *pShaderResourceView
) {
    LOG_MFUN();
    inner->GenerateMips(
        ((MyID3D10ShaderResourceView *)pShaderResourceView)->inner
    );
}

void STDMETHODCALLTYPE MyID3D10Device::ResolveSubresource(
    ID3D10Resource *pDstResource,
    UINT DstSubresource,
    ID3D10Resource *pSrcResource,
    UINT SrcSubresource,
    DXGI_FORMAT Format
) {
    LOG_MFUN(_,
        LOG_ARG(pDstResource),
        LOG_ARG(DstSubresource),
        LOG_ARG(pSrcResource),
        LOG_ARG(SrcSubresource),
        LOG_ARG(Format)
    );
    D3D10_RESOURCE_DIMENSION dstType;
    pDstResource->GetType(&dstType);
    D3D10_RESOURCE_DIMENSION srcType;
    pSrcResource->GetType(&srcType);
    switch (dstType) {
        case D3D10_RESOURCE_DIMENSION_BUFFER:
            pDstResource = ((MyID3D10Buffer *)pDstResource)->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
            pDstResource = ((MyID3D10Texture1D *)pDstResource)->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
            pDstResource = ((MyID3D10Texture2D *)pDstResource)->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
            pDstResource = ((MyID3D10Texture3D *)pDstResource)->inner;
            break;

        default:
            break;
    }
    switch (srcType) {
        case D3D10_RESOURCE_DIMENSION_BUFFER:
            pSrcResource = ((MyID3D10Buffer *)pSrcResource)->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
            pSrcResource = ((MyID3D10Texture1D *)pSrcResource)->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
            pSrcResource = ((MyID3D10Texture2D *)pSrcResource)->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
            pSrcResource = ((MyID3D10Texture3D *)pSrcResource)->inner;
            break;

        default:
            break;
    }
    inner->ResolveSubresource(
        pDstResource,
        DstSubresource,
        pSrcResource,
        SrcSubresource,
        Format
    );
}

void STDMETHODCALLTYPE MyID3D10Device::VSGetConstantBuffers(
    UINT StartSlot,
    UINT NumBuffers,
    ID3D10Buffer **ppConstantBuffers
) {
    LOG_MFUN();
    inner->VSGetConstantBuffers(
        StartSlot,
        NumBuffers,
        ppConstantBuffers
    );
    for (UINT i = 0; i < NumBuffers; ++i) {
        ppConstantBuffers[i] = ppConstantBuffers[i] ? current_bs_map.find(ppConstantBuffers[i])->second : NULL;
    }
}

void STDMETHODCALLTYPE MyID3D10Device::PSGetShaderResources(
    UINT StartSlot,
    UINT NumViews,
    ID3D10ShaderResourceView **ppShaderResourceViews
) {
    LOG_MFUN();
    inner->PSGetShaderResources(
        StartSlot,
        NumViews,
        ppShaderResourceViews
    );
    for (UINT i = 0; i < NumViews; ++i) {
        ppShaderResourceViews[i] = ppShaderResourceViews[i] ? current_srvs_map.find(ppShaderResourceViews[i])->second : NULL;
    }
}

void STDMETHODCALLTYPE MyID3D10Device::PSGetShader(
    ID3D10PixelShader **ppPixelShader
) {
    LOG_MFUN();
    inner->PSGetShader(
        ppPixelShader
    );
    if (ppPixelShader) *ppPixelShader = *ppPixelShader ? current_pss_map.find(*ppPixelShader)->second : NULL;
}

void STDMETHODCALLTYPE MyID3D10Device::PSGetSamplers(
    UINT StartSlot,
    UINT NumSamplers,
    ID3D10SamplerState **ppSamplers
) {
    LOG_MFUN();
    inner->PSGetSamplers(
        StartSlot,
        NumSamplers,
        ppSamplers
    );
    for (UINT i = 0; i < NumSamplers; ++i) {
        ppSamplers[i] = ppSamplers[i] ? current_sss_map.find(ppSamplers[i])->second : NULL;
    }
}

void STDMETHODCALLTYPE MyID3D10Device::VSGetShader(
    ID3D10VertexShader **ppVertexShader
) {
    LOG_MFUN();
    inner->VSGetShader(
        ppVertexShader
    );
}

void STDMETHODCALLTYPE MyID3D10Device::PSGetConstantBuffers(
    UINT StartSlot,
    UINT NumBuffers,
    ID3D10Buffer **ppConstantBuffers
) {
    LOG_MFUN();
    inner->PSGetConstantBuffers(
        StartSlot,
        NumBuffers,
        ppConstantBuffers
    );
    for (UINT i = 0; i < NumBuffers; ++i) {
        ppConstantBuffers[i] = ppConstantBuffers[i] ? current_bs_map.find(ppConstantBuffers[i])->second : NULL;
    }
}

void STDMETHODCALLTYPE MyID3D10Device::IAGetInputLayout(
    ID3D10InputLayout **ppInputLayout
) {
    LOG_MFUN();
    inner->IAGetInputLayout(
        ppInputLayout
    );
}

void STDMETHODCALLTYPE MyID3D10Device::IAGetVertexBuffers(
    UINT StartSlot,
    UINT NumBuffers,
    ID3D10Buffer **ppVertexBuffers,
    UINT *pStrides,
    UINT *pOffsets
) {
    LOG_MFUN();
    inner->IAGetVertexBuffers(
        StartSlot,
        NumBuffers,
        ppVertexBuffers,
        pStrides,
        pOffsets
    );
    for (UINT i = 0; i < NumBuffers; ++i) {
        ppVertexBuffers[i] = ppVertexBuffers[i] ? current_bs_map.find(ppVertexBuffers[i])->second : NULL;
    }
}

void STDMETHODCALLTYPE MyID3D10Device::IAGetIndexBuffer(
    ID3D10Buffer **pIndexBuffer,
    DXGI_FORMAT *Format,
    UINT *Offset
) {
    LOG_MFUN();
    inner->IAGetIndexBuffer(
        pIndexBuffer,
        Format,
        Offset
    );
    if (pIndexBuffer) *pIndexBuffer = *pIndexBuffer ? current_bs_map.find(*pIndexBuffer)->second : NULL;
}

void STDMETHODCALLTYPE MyID3D10Device::GSGetConstantBuffers(
    UINT StartSlot,
    UINT NumBuffers,
    ID3D10Buffer **ppConstantBuffers
) {
    LOG_MFUN();
    inner->GSGetConstantBuffers(
        StartSlot,
        NumBuffers,
        ppConstantBuffers
    );
    for (UINT i = 0; i < NumBuffers; ++i) {
        ppConstantBuffers[i] = ppConstantBuffers[i] ? current_bs_map.find(ppConstantBuffers[i])->second : NULL;
    }
}

void STDMETHODCALLTYPE MyID3D10Device::GSGetShader(
    ID3D10GeometryShader **ppGeometryShader
) {
    LOG_MFUN();
    inner->GSGetShader(
        ppGeometryShader
    );
}

void STDMETHODCALLTYPE MyID3D10Device::IAGetPrimitiveTopology(
    D3D10_PRIMITIVE_TOPOLOGY *pTopology
) {
    LOG_MFUN();
    inner->IAGetPrimitiveTopology(
        pTopology
    );
}

void STDMETHODCALLTYPE MyID3D10Device::VSGetShaderResources(
    UINT StartSlot,
    UINT NumViews,
    ID3D10ShaderResourceView **ppShaderResourceViews
) {
    LOG_MFUN();
    inner->VSGetShaderResources(
        StartSlot,
        NumViews,
        ppShaderResourceViews
    );
    for (UINT i = 0; i < NumViews; ++i) {
        ppShaderResourceViews[i] = ppShaderResourceViews[i] ? current_srvs_map.find(ppShaderResourceViews[i])->second : NULL;
    }
}

void STDMETHODCALLTYPE MyID3D10Device::VSGetSamplers(
    UINT StartSlot,
    UINT NumSamplers,
    ID3D10SamplerState **ppSamplers
) {
    LOG_MFUN();
    inner->VSGetSamplers(
        StartSlot,
        NumSamplers,
        ppSamplers
    );
    for (UINT i = 0; i < NumSamplers; ++i) {
        ppSamplers[i] = ppSamplers[i] ? current_sss_map.find(ppSamplers[i])->second : NULL;
    }
}

void STDMETHODCALLTYPE MyID3D10Device::GetPredication(
    ID3D10Predicate **ppPredicate,
    WINBOOL *pPredicateValue
) {
    LOG_MFUN();
    inner->GetPredication(
        ppPredicate,
        pPredicateValue
    );
}

void STDMETHODCALLTYPE MyID3D10Device::GSGetShaderResources(
    UINT StartSlot,
    UINT NumViews,
    ID3D10ShaderResourceView **ppShaderResourceViews
) {
    LOG_MFUN();
    inner->GSGetShaderResources(
        StartSlot,
        NumViews,
        ppShaderResourceViews
    );
    for (UINT i = 0; i < NumViews; ++i) {
        ppShaderResourceViews[i] = ppShaderResourceViews[i] ? current_srvs_map.find(ppShaderResourceViews[i])->second : NULL;
    }
}

void STDMETHODCALLTYPE MyID3D10Device::GSGetSamplers(
    UINT StartSlot,
    UINT NumSamplers,
    ID3D10SamplerState **ppSamplers
) {
    LOG_MFUN();
    inner->GSGetSamplers(
        StartSlot,
        NumSamplers,
        ppSamplers
    );
    for (UINT i = 0; i < NumSamplers; ++i) {
        ppSamplers[i] = ppSamplers[i] ? current_sss_map.find(ppSamplers[i])->second : NULL;
    }
}

void STDMETHODCALLTYPE MyID3D10Device::OMGetRenderTargets(
    UINT NumViews,
    ID3D10RenderTargetView **ppRenderTargetViews,
    ID3D10DepthStencilView **ppDepthStencilView
) {
    LOG_MFUN();
    inner->OMGetRenderTargets(
        NumViews,
        ppRenderTargetViews,
        ppDepthStencilView
    );
    for (UINT i = 0; i < NumViews; ++i) {
        ppRenderTargetViews[i] = ppRenderTargetViews[i] ? current_rtvs_map.find(ppRenderTargetViews[i])->second : NULL;
    }
    if (ppDepthStencilView) *ppDepthStencilView = *ppDepthStencilView ? current_dsvs_map.find(*ppDepthStencilView)->second : NULL;
}

void STDMETHODCALLTYPE MyID3D10Device::OMGetBlendState(
    ID3D10BlendState **ppBlendState,
    FLOAT BlendFactor[4],
    UINT *pSampleMask
) {
    LOG_MFUN();
    inner->OMGetBlendState(
        ppBlendState,
        BlendFactor,
        pSampleMask
    );
}

void STDMETHODCALLTYPE MyID3D10Device::OMGetDepthStencilState(
    ID3D10DepthStencilState **ppDepthStencilState,
    UINT *pStencilRef
) {
    LOG_MFUN();
    inner->OMGetDepthStencilState(
        ppDepthStencilState,
        pStencilRef
    );
}

void STDMETHODCALLTYPE MyID3D10Device::SOGetTargets(
    UINT NumBuffers,
    ID3D10Buffer **ppSOTargets,
    UINT *pOffsets
) {
    LOG_MFUN();
    inner->SOGetTargets(
        NumBuffers,
        ppSOTargets,
        pOffsets
    );
    for (UINT i = 0; i < NumBuffers; ++i) {
        ppSOTargets[i] = ppSOTargets[i] ? current_bs_map.find(ppSOTargets[i])->second : NULL;
    }
}

void STDMETHODCALLTYPE MyID3D10Device::RSGetState(
    ID3D10RasterizerState **ppRasterizerState
) {
    LOG_MFUN();
    inner->RSGetState(
        ppRasterizerState
    );
}

void STDMETHODCALLTYPE MyID3D10Device::RSGetViewports(
    UINT *NumViewports,
    D3D10_VIEWPORT *pViewports
) {
    LOG_MFUN();
    inner->RSGetViewports(
        NumViewports,
        pViewports
    );
}

void STDMETHODCALLTYPE MyID3D10Device::RSGetScissorRects(
    UINT *NumRects,
    D3D10_RECT *pRects
) {
    LOG_MFUN();
    inner->RSGetScissorRects(
        NumRects,
        pRects
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::GetDeviceRemovedReason(
) {
    LOG_MFUN();
    return inner->GetDeviceRemovedReason(
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::SetExceptionMode(
    UINT RaiseFlags
) {
    LOG_MFUN();
    return inner->SetExceptionMode(
        RaiseFlags
    );
}

UINT STDMETHODCALLTYPE MyID3D10Device::GetExceptionMode(
) {
    LOG_MFUN();
    return inner->GetExceptionMode(
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::GetPrivateData(
    REFGUID guid,
    UINT *pDataSize,
    void *pData
) {
    LOG_MFUN();
    return inner->GetPrivateData(
        guid,
        pDataSize,
        pData
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::SetPrivateData(
    REFGUID guid,
    UINT DataSize,
    const void *pData
) {
    LOG_MFUN();
    return inner->SetPrivateData(
        guid,
        DataSize,
        pData
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::SetPrivateDataInterface(
    REFGUID guid,
    const IUnknown *pData
) {
    LOG_MFUN();
    return inner->SetPrivateDataInterface(
        guid,
        pData
    );
}

void STDMETHODCALLTYPE MyID3D10Device::ClearState(
) {
    LOG_MFUN();
    inner->ClearState(
    );
}

void STDMETHODCALLTYPE MyID3D10Device::Flush(
) {
    LOG_MFUN();
    inner->Flush(
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateBuffer(
    const D3D10_BUFFER_DESC *pDesc,
    const D3D10_SUBRESOURCE_DATA *pInitialData,
    ID3D10Buffer **ppBuffer
) {
    HRESULT ret = inner->CreateBuffer(
        pDesc,
        pInitialData,
        ppBuffer
    );
    if (ret == S_OK) {
        LOG_MFUN(_,
            LOG_ARG(pDesc),
            LOG_ARG(*ppBuffer),
            ret
        );
        new MyID3D10Buffer(ppBuffer, pDesc);
    } else {
        LOG_MFUN(_,
            LOG_ARG(pDesc),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateTexture1D(
    const D3D10_TEXTURE1D_DESC *pDesc,
    const D3D10_SUBRESOURCE_DATA *pInitialData,
    ID3D10Texture1D **ppTexture1D
) {
    HRESULT ret = inner->CreateTexture1D(
        pDesc,
        pInitialData,
        ppTexture1D
    );
    if (ret == S_OK) {
        LOG_MFUN(_,
            LOG_ARG(pDesc),
            LOG_ARG(*ppTexture1D),
            ret
        );
        new MyID3D10Texture1D(ppTexture1D, pDesc);
    } else {
        LOG_MFUN(_,
            LOG_ARG(pDesc),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateTexture2D(
    const D3D10_TEXTURE2D_DESC *pDesc,
    const D3D10_SUBRESOURCE_DATA *pInitialData,
    ID3D10Texture2D **ppTexture2D
) {
    HRESULT ret = inner->CreateTexture2D(
        pDesc,
        pInitialData,
        ppTexture2D
    );
    if (ret == S_OK) {
        LOG_MFUN(_,
            LOG_ARG(pDesc),
            LOG_ARG(*ppTexture2D),
            ret
        );
        new MyID3D10Texture2D(ppTexture2D, pDesc);
    } else {
        LOG_MFUN(_,
            LOG_ARG(pDesc),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateTexture3D(
    const D3D10_TEXTURE3D_DESC *pDesc,
    const D3D10_SUBRESOURCE_DATA *pInitialData,
    ID3D10Texture3D **ppTexture3D
) {
    HRESULT ret =inner->CreateTexture3D(
        pDesc,
        pInitialData,
        ppTexture3D
    );
    if (ret == S_OK) {
        LOG_MFUN(_,
            LOG_ARG(pDesc),
            LOG_ARG(*ppTexture3D),
            ret
        );
        new MyID3D10Texture3D(ppTexture3D, pDesc);
    } else {
        LOG_MFUN(_,
            LOG_ARG(pDesc),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateShaderResourceView(
    ID3D10Resource *pResource,
    const D3D10_SHADER_RESOURCE_VIEW_DESC *pDesc,
    ID3D10ShaderResourceView **ppSRView
) {
    D3D10_RESOURCE_DIMENSION type;
    pResource->GetType(&type);
    ID3D10Resource *resource_inner;
    MyID3D10Texture2D *texture_2d = NULL;
    switch (type) {
        case D3D10_RESOURCE_DIMENSION_BUFFER:
            resource_inner = ((MyID3D10Buffer *)pResource)->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
            resource_inner = ((MyID3D10Texture1D *)pResource)->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
            texture_2d = (MyID3D10Texture2D *)pResource;
            resource_inner = texture_2d->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
            resource_inner = ((MyID3D10Texture3D *)pResource)->inner;
            break;

        default:
            break;
    }
    HRESULT ret = inner->CreateShaderResourceView(
        resource_inner,
        pDesc,
        ppSRView
    );
    if (ret == S_OK) {
        LOG_MFUN(_,
            LOG_ARG(pResource),
            LOG_ARG(pDesc),
            LOG_ARG(*ppSRView),
            ret
        );
        MyID3D10ShaderResourceView *srv = new MyID3D10ShaderResourceView(ppSRView, pDesc, pResource);
        if (texture_2d) {
            texture_2d->srvs.insert(srv);
        }
    } else {
        LOG_MFUN(_,
            LOG_ARG(pResource),
            LOG_ARG(pDesc),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateRenderTargetView(
    ID3D10Resource *pResource,
    const D3D10_RENDER_TARGET_VIEW_DESC *pDesc,
    ID3D10RenderTargetView **ppRTView
) {
    D3D10_RESOURCE_DIMENSION type;
    pResource->GetType(&type);
    ID3D10Resource *resource_inner;
    MyID3D10Texture2D *texture_2d = NULL;
    switch (type) {
        case D3D10_RESOURCE_DIMENSION_BUFFER:
            resource_inner = ((MyID3D10Buffer *)pResource)->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
            resource_inner = ((MyID3D10Texture1D *)pResource)->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
            texture_2d = (MyID3D10Texture2D *)pResource;
            resource_inner = texture_2d->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
            resource_inner = ((MyID3D10Texture3D *)pResource)->inner;
            break;

        default:
            break;
    }
    HRESULT ret = inner->CreateRenderTargetView(
        resource_inner,
        pDesc,
        ppRTView
    );
    if (ret == S_OK) {
        LOG_MFUN(_,
            LOG_ARG(pResource),
            LOG_ARG(pDesc),
            LOG_ARG(*ppRTView),
            ret
        );
        MyID3D10RenderTargetView *rtv = new MyID3D10RenderTargetView(ppRTView, pDesc, pResource);
        if (texture_2d) {
            texture_2d->rtvs.insert(rtv);
        }
    } else {
        LOG_MFUN(_,
            LOG_ARG(pResource),
            LOG_ARG(pDesc),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateDepthStencilView(
    ID3D10Resource *pResource,
    const D3D10_DEPTH_STENCIL_VIEW_DESC *pDesc,
    ID3D10DepthStencilView **ppDepthStencilView
) {
    D3D10_RESOURCE_DIMENSION type;
    pResource->GetType(&type);
    ID3D10Resource *resource_inner;
    MyID3D10Texture2D *texture_2d = NULL;
    switch (type) {
        case D3D10_RESOURCE_DIMENSION_BUFFER:
            resource_inner = ((MyID3D10Buffer *)pResource)->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
            resource_inner = ((MyID3D10Texture1D *)pResource)->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
            texture_2d = (MyID3D10Texture2D *)pResource;
            resource_inner = texture_2d->inner;
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
            resource_inner = ((MyID3D10Texture3D *)pResource)->inner;
            break;

        default:
            break;
    }
    HRESULT ret = inner->CreateDepthStencilView(
        resource_inner,
        pDesc,
        ppDepthStencilView
    );
    if (ret == S_OK) {
        LOG_MFUN(_,
            LOG_ARG(pResource),
            LOG_ARG(pDesc),
            LOG_ARG(*ppDepthStencilView),
            ret
        );
        MyID3D10DepthStencilView *dsv = new MyID3D10DepthStencilView(ppDepthStencilView, pDesc, pResource);
        if (texture_2d) {
            texture_2d->dsvs.insert(dsv);
        }
    } else {
        LOG_MFUN(_,
            LOG_ARG(pResource),
            LOG_ARG(pDesc),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateInputLayout(
    const D3D10_INPUT_ELEMENT_DESC *pInputElementDescs,
    UINT NumElements,
    const void *pShaderBytecodeWithInputSignature,
    SIZE_T BytecodeLength,
    ID3D10InputLayout **ppInputLayout
) {
    HRESULT ret = inner->CreateInputLayout(
        pInputElementDescs,
        NumElements,
        pShaderBytecodeWithInputSignature,
        BytecodeLength,
        ppInputLayout
    );
    if (ret == S_OK) {
        LOG_MFUN(_,
            LOG_ARG_TYPE(pInputElementDescs, ArrayLoggerRef, NumElements),
            LOG_ARG_TYPE(ppInputLayout, ArrayLoggerDeref, NumElements),
            ret
        );
    } else {
        LOG_MFUN(_,
            LOG_ARG_TYPE(pInputElementDescs, ArrayLoggerRef, NumElements),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateVertexShader(
    const void *pShaderBytecode,
    SIZE_T BytecodeLength,
    ID3D10VertexShader **ppVertexShader
) {
    HRESULT ret = inner->CreateVertexShader(
        pShaderBytecode,
        BytecodeLength,
        ppVertexShader
    );
    if (ret == S_OK) {
        LOG_MFUN(_,
            LOG_ARG_TYPE(pShaderBytecode, ShaderLogger, BytecodeLength),
            LOG_ARG(BytecodeLength),
            LOG_ARG(*ppVertexShader),
            ret
        );
    } else {
        LOG_MFUN(_,
            LOG_ARG_TYPE(pShaderBytecode, ShaderLogger, BytecodeLength),
            LOG_ARG(BytecodeLength),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateGeometryShader(
    const void *pShaderBytecode,
    SIZE_T BytecodeLength,
    ID3D10GeometryShader **ppGeometryShader
) {
    HRESULT ret = inner->CreateGeometryShader(
        pShaderBytecode,
        BytecodeLength,
        ppGeometryShader
    );
    if (ret == S_OK) {
        LOG_MFUN(_,
            LOG_ARG_TYPE(pShaderBytecode, ShaderLogger, BytecodeLength),
            LOG_ARG(BytecodeLength),
            LOG_ARG(*ppGeometryShader),
            ret
        );
    } else {
        LOG_MFUN(_,
            LOG_ARG_TYPE(pShaderBytecode, ShaderLogger, BytecodeLength),
            LOG_ARG(BytecodeLength),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateGeometryShaderWithStreamOutput(
    const void *pShaderBytecode,
    SIZE_T BytecodeLength,
    const D3D10_SO_DECLARATION_ENTRY *pSODeclaration,
    UINT NumEntries,
    UINT OutputStreamStride,
    ID3D10GeometryShader **ppGeometryShader
) {
    LOG_MFUN();
    return inner->CreateGeometryShaderWithStreamOutput(
        pShaderBytecode,
        BytecodeLength,
        pSODeclaration,
        NumEntries,
        OutputStreamStride,
        ppGeometryShader
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreatePixelShader(
    const void *pShaderBytecode,
    SIZE_T BytecodeLength,
    ID3D10PixelShader **ppPixelShader
) {
    HRESULT ret = inner->CreatePixelShader(
        pShaderBytecode,
        BytecodeLength,
        ppPixelShader
    );
    if (ret == S_OK) {
        DWORD hash;
        MurmurHash3_x86_32(pShaderBytecode, BytecodeLength, 0, &hash);
        new MyID3D10PixelShader(ppPixelShader, hash, BytecodeLength);
        LOG_MFUN(_,
            LOG_ARG_TYPE(pShaderBytecode, ShaderLogger, BytecodeLength),
            LOG_ARG_TYPE(hash, NumHexLogger),
            LOG_ARG(BytecodeLength),
            LOG_ARG(*ppPixelShader),
            ret
        );
    } else {
        LOG_MFUN(_,
            LOG_ARG_TYPE(pShaderBytecode, ShaderLogger, BytecodeLength),
            LOG_ARG(BytecodeLength),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateBlendState(
    const D3D10_BLEND_DESC *pBlendStateDesc,
    ID3D10BlendState **ppBlendState
) {
    LOG_MFUN();
    return inner->CreateBlendState(
        pBlendStateDesc,
        ppBlendState
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateDepthStencilState(
    const D3D10_DEPTH_STENCIL_DESC *pDepthStencilDesc,
    ID3D10DepthStencilState **ppDepthStencilState
) {
    LOG_MFUN();
    return inner->CreateDepthStencilState(
        pDepthStencilDesc,
        ppDepthStencilState
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateRasterizerState(
    const D3D10_RASTERIZER_DESC *pRasterizerDesc,
    ID3D10RasterizerState **ppRasterizerState
) {
    LOG_MFUN();
    return inner->CreateRasterizerState(
        pRasterizerDesc,
        ppRasterizerState
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateSamplerState(
    const D3D10_SAMPLER_DESC *pSamplerDesc,
    ID3D10SamplerState **ppSamplerState
) {
    HRESULT ret = inner->CreateSamplerState(
        pSamplerDesc,
        ppSamplerState
    );
    if (ret == S_OK) {
        LOG_MFUN(_,
            LOG_ARG(pSamplerDesc),
            LOG_ARG_TYPE(*ppSamplerState, NumHexLogger),
            ret
        );
        new MyID3D10SamplerState(ppSamplerState, pSamplerDesc);
    } else {
        LOG_MFUN(_,
            LOG_ARG(pSamplerDesc),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateQuery(
    const D3D10_QUERY_DESC *pQueryDesc,
    ID3D10Query **ppQuery
) {
    LOG_MFUN();
    return inner->CreateQuery(
        pQueryDesc,
        ppQuery
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreatePredicate(
    const D3D10_QUERY_DESC *pPredicateDesc,
    ID3D10Predicate **ppPredicate
) {
    LOG_MFUN();
    return inner->CreatePredicate(
        pPredicateDesc,
        ppPredicate
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateCounter(
    const D3D10_COUNTER_DESC *pCounterDesc,
    ID3D10Counter **ppCounter
) {
    LOG_MFUN();
    return inner->CreateCounter(
        pCounterDesc,
        ppCounter
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CheckFormatSupport(
    DXGI_FORMAT Format,
    UINT *pFormatSupport
) {
    LOG_MFUN();
    return inner->CheckFormatSupport(
        Format,
        pFormatSupport
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CheckMultisampleQualityLevels(
    DXGI_FORMAT Format,
    UINT SampleCount,
    UINT *pNumQualityLevels
) {
    LOG_MFUN();
    return inner->CheckMultisampleQualityLevels(
        Format,
        SampleCount,
        pNumQualityLevels
    );
}

void STDMETHODCALLTYPE MyID3D10Device::CheckCounterInfo(
    D3D10_COUNTER_INFO *pCounterInfo
) {
    LOG_MFUN();
    inner->CheckCounterInfo(
        pCounterInfo
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CheckCounter(
    const D3D10_COUNTER_DESC *pDesc,
    D3D10_COUNTER_TYPE *pType,
    UINT *pActiveCounters,
    char *name,
    UINT *pNameLength,
    char *units,
    UINT *pUnitsLength,
    char *description,
    UINT *pDescriptionLength
) {
    LOG_MFUN();
    return inner->CheckCounter(
        pDesc,
        pType,
        pActiveCounters,
        name,
        pNameLength,
        units,
        pUnitsLength,
        description,
        pDescriptionLength
    );
}

UINT STDMETHODCALLTYPE MyID3D10Device::GetCreationFlags(
) {
    LOG_MFUN();
    return inner->GetCreationFlags(
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::OpenSharedResource(
    HANDLE hResource,
    REFIID ReturnedInterface,
    void **ppResource
) {
    LOG_MFUN();
    return inner->OpenSharedResource(
        hResource,
        ReturnedInterface,
        ppResource
    );
}

void STDMETHODCALLTYPE MyID3D10Device::SetTextFilterSize(
    UINT Width,
    UINT Height
) {
    LOG_MFUN();
    inner->SetTextFilterSize(
        Width,
        Height
    );
}

void STDMETHODCALLTYPE MyID3D10Device::GetTextFilterSize(
    UINT *pWidth,
    UINT *pHeight
) {
    LOG_MFUN();
    inner->GetTextFilterSize(
        pWidth,
        pHeight
    );
}
