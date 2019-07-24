#include "d3d10_base.c"
#include "d3d10.h"

d3d10_video_t *my_d3d10_gfx_init(ID3D10Device *device, DXGI_FORMAT format) {
    d3d10_video_t *d3d10 = (d3d10_video_t *)calloc(1, sizeof(d3d10_video_t));
    if (!d3d10) return NULL;

    d3d10->device = device;

    d3d10->frame.texture[0].desc.Format = format;
    d3d10->frame.texture[0].desc.Usage  = D3D10_USAGE_DEFAULT;
    d3d10->frame.texture[0].desc.Width  = 4;
    d3d10->frame.texture[0].desc.Height = 4;
    d3d10_init_texture(d3d10->device, &d3d10->frame.texture[0]);

    matrix_4x4_ortho(d3d10->ubo_values.mvp, 0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);

    {
        D3D10_SUBRESOURCE_DATA ubo_data;
        D3D10_BUFFER_DESC desc;

        desc.ByteWidth            = sizeof(d3d10->ubo_values);
        desc.Usage                = D3D10_USAGE_DYNAMIC;
        desc.BindFlags            = D3D10_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags       = D3D10_CPU_ACCESS_WRITE;
        desc.MiscFlags            = 0;

        ubo_data.pSysMem          = &d3d10->ubo_values;
        ubo_data.SysMemPitch      = 0;
        ubo_data.SysMemSlicePitch = 0;

        D3D10CreateBuffer(d3d10->device, &desc, &ubo_data, &d3d10->ubo);
        D3D10CreateBuffer(d3d10->device, &desc, NULL, &d3d10->frame.ubo);
    }

    d3d10_gfx_set_rotation(d3d10, 0);

    {
        D3D10_SAMPLER_DESC desc = {};
        desc.MaxAnisotropy      = 1;
        desc.ComparisonFunc     = D3D10_COMPARISON_NEVER;
        desc.MinLOD             = -D3D10_FLOAT32_MAX;
        desc.MaxLOD             = D3D10_FLOAT32_MAX;

        for (unsigned i = 0; i < RARCH_WRAP_MAX; ++i) {
            switch (i) {
                case RARCH_WRAP_BORDER:
                    desc.AddressU = D3D10_TEXTURE_ADDRESS_BORDER;
                    break;

                case RARCH_WRAP_EDGE:
                    desc.AddressU = D3D10_TEXTURE_ADDRESS_CLAMP;
                    break;

                case RARCH_WRAP_REPEAT:
                    desc.AddressU = D3D10_TEXTURE_ADDRESS_WRAP;
                    break;

                case RARCH_WRAP_MIRRORED_REPEAT:
                    desc.AddressU = D3D10_TEXTURE_ADDRESS_MIRROR;
                    break;
            }
            desc.AddressV = desc.AddressU;
            desc.AddressW = desc.AddressU;

            desc.Filter = D3D10_FILTER_MIN_MAG_MIP_LINEAR;
            D3D10CreateSamplerState(d3d10->device, &desc, &d3d10->samplers[RARCH_FILTER_LINEAR][i]);

            desc.Filter = D3D10_FILTER_MIN_MAG_MIP_POINT;
            D3D10CreateSamplerState(d3d10->device, &desc, &d3d10->samplers[RARCH_FILTER_NEAREST][i]);
        }
    }

    d3d10_set_filtering(d3d10, 0, true);

    {
        d3d10_vertex_t vertices[] = {
            { { 0.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
            { { 0.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
            { { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
            { { 1.0f, 1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
        };
        D3D10_SUBRESOURCE_DATA vertexData = { vertices };

        D3D10_BUFFER_DESC desc = {};
        desc.ByteWidth      = sizeof(vertices);
        desc.Usage          = D3D10_USAGE_IMMUTABLE;
        desc.BindFlags      = D3D10_BIND_VERTEX_BUFFER;

        D3D10CreateBuffer(d3d10->device, &desc, &vertexData, &d3d10->frame.vbo);
    }

    {
        D3D10_INPUT_ELEMENT_DESC desc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(d3d10_vertex_t, position), D3D10_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(d3d10_vertex_t, texcoord), D3D10_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(d3d10_vertex_t, color), D3D10_INPUT_PER_VERTEX_DATA, 0 },
        };

        static const char shader[] =
#include "d3d_shaders/opaque_sm5.hlsl.h"
        ;

        if (!d3d10_init_shader(d3d10->device, shader, sizeof(shader), NULL, "VSMain", "PSMain", NULL, desc, countof(desc), &d3d10->shaders[VIDEO_SHADER_STOCK_BLEND])) {
            goto error;
        }
    }

    {
        D3D10_BLEND_DESC blend_desc = { 0 };

        blend_desc.AlphaToCoverageEnable    = FALSE;
        blend_desc.BlendEnable[0]           = TRUE;
        blend_desc.SrcBlend                 = D3D10_BLEND_SRC_ALPHA;
        blend_desc.DestBlend                = D3D10_BLEND_INV_SRC_ALPHA;
        blend_desc.BlendOp                  = D3D10_BLEND_OP_ADD;
        blend_desc.SrcBlendAlpha            = D3D10_BLEND_SRC_ALPHA;
        blend_desc.DestBlendAlpha           = D3D10_BLEND_INV_SRC_ALPHA;
        blend_desc.BlendOpAlpha             = D3D10_BLEND_OP_ADD;
        blend_desc.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;
        D3D10CreateBlendState(d3d10->device, &blend_desc, &d3d10->blend_enable);

        blend_desc.SrcBlend  = D3D10_BLEND_ONE;
        blend_desc.DestBlend = D3D10_BLEND_ONE;
        D3D10CreateBlendState(d3d10->device, &blend_desc, &d3d10->blend_pipeline);

        blend_desc.BlendEnable[0] = FALSE;
        D3D10CreateBlendState(d3d10->device, &blend_desc, &d3d10->blend_disable);
    }

    {
        D3D10_RASTERIZER_DESC desc = { (D3D10_FILL_MODE)0 };

        desc.FillMode = D3D10_FILL_SOLID;
        desc.CullMode = D3D10_CULL_NONE;

        D3D10CreateRasterizerState(d3d10->device, &desc, &d3d10->state);
    }

    return d3d10;

error:
    d3d10_gfx_free(d3d10);
    return NULL;
}

void my_d3d10_gfx_free(d3d10_video_t *d3d10) {
    if (!d3d10) return;

    d3d10_free_shader_preset(d3d10);

    d3d10_release_texture(&d3d10->frame.texture[0]);
    Release(d3d10->frame.ubo);
    Release(d3d10->frame.vbo);

    for (unsigned i = 0; i < GFX_MAX_SHADERS; i++) {
        d3d10_release_shader(&d3d10->shaders[i]);
    }

    Release(d3d10->blend_pipeline);
    Release(d3d10->ubo);
    Release(d3d10->blend_enable);
    Release(d3d10->blend_disable);

    for (unsigned i = 0; i < RARCH_WRAP_MAX; i++) {
        Release(d3d10->samplers[RARCH_FILTER_LINEAR][i]);
        Release(d3d10->samplers[RARCH_FILTER_NEAREST][i]);
    }

    Release(d3d10->state);

    free(d3d10);
}

bool my_d3d10_gfx_set_shader(d3d10_video_t *d3d10, const char* path) {
    return d3d10_gfx_set_shader(d3d10, RARCH_SHADER_SLANG, path);
}

bool my_d3d10_gfx_frame(d3d10_video_t *d3d10, d3d10_texture_t *texture, UINT64 frame_count) {
    D3D10Device context = d3d10->device;
    unsigned width = texture->desc.Width;
    unsigned height = texture->desc.Height;

    D3D10SetPrimitiveTopology(context, D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

if (d3d10->shader_preset) {
    if (d3d10->frame.texture[0].desc.Width != width || d3d10->frame.texture[0].desc.Height != height) {
        d3d10->resize_render_targets = true;
    }

    if (d3d10->resize_render_targets) {
        for (unsigned i = 0; i < d3d10->shader_preset->passes; ++i) {
            d3d10_release_texture(&d3d10->pass[i].rt);
            d3d10_release_texture(&d3d10->pass[i].feedback);
            memset(&d3d10->pass[i].rt, 0, sizeof(d3d10->pass[i].rt));
            memset(&d3d10->pass[i].feedback, 0, sizeof(d3d10->pass[i].feedback));
        }
    }

    if (d3d10->shader_preset->history_size) {
        if (d3d10->init_history) {
            d3d10_init_history(d3d10, width, height);
        } else {
            d3d10_texture_t tmp = d3d10->frame.texture[d3d10->shader_preset->history_size];
            for (int k = d3d10->shader_preset->history_size; k > 0; --k) {
                d3d10->frame.texture[k] = d3d10->frame.texture[k - 1];
            }
            d3d10->frame.texture[0] = tmp;
        }
    }
}

    if (d3d10->frame.texture[0].desc.Width != width || d3d10->frame.texture[0].desc.Height != height) {
        d3d10->frame.texture[0].desc.Width  = width;
        d3d10->frame.texture[0].desc.Height = height;
        d3d10_init_texture(d3d10->device, &d3d10->frame.texture[0]);
    }

if (d3d10->shader_preset) {
    if (d3d10->resize_render_targets) {
        d3d10_init_render_targets(d3d10, width, height);
    }
}

    D3D10SetVertexBuffer(context, 0, d3d10->frame.vbo, sizeof(d3d10_vertex_t), 0);
    D3D10SetBlendState(context, d3d10->blend_disable, NULL, D3D10_DEFAULT_SAMPLE_MASK);

    D3D10CopyResource(context, (D3D10Resource)d3d10->frame.texture[0].handle, (D3D10Resource)texture->handle);

    texture = d3d10->frame.texture;

if (d3d10->shader_preset) {
    for (unsigned i = 0; i < d3d10->shader_preset->passes; ++i) {
        if (d3d10->shader_preset->pass[i].feedback) {
            d3d10_texture_t tmp     = d3d10->pass[i].feedback;
            d3d10->pass[i].feedback = d3d10->pass[i].rt;
            d3d10->pass[i].rt       = tmp;
        }
    }

    for (unsigned i = 0; i < d3d10->shader_preset->passes; ++i) {
        d3d10_set_shader(context, &d3d10->pass[i].shader);

        if (d3d10->shader_preset->pass[i].frame_count_mod) {
            d3d10->pass[i].frame_count = frame_count % d3d10->shader_preset->pass[i].frame_count_mod;
        } else {
            d3d10->pass[i].frame_count = frame_count;
        }
        d3d10->pass[i].frame_direction = 1;

        for (unsigned j = 0; j < SLANG_CBUFFER_MAX; ++j) {
            D3D10Buffer    buffer     = d3d10->pass[i].buffers[j];
            cbuffer_sem_t *buffer_sem = &d3d10->pass[i].semantics.cbuffers[j];

            if (buffer_sem->stage_mask && buffer_sem->uniforms) {
                void *data;
                uniform_sem_t *uniform = buffer_sem->uniforms;

                D3D10MapBuffer(buffer, D3D10_MAP_WRITE_DISCARD, 0, &data);
                while (uniform->size) {
                    if (uniform->data) {
                        memcpy((char *)data + uniform->offset, uniform->data, uniform->size);
                    }
                    uniform++;
                }
                D3D10UnmapBuffer(buffer);

                if (buffer_sem->stage_mask & SLANG_STAGE_VERTEX_MASK) {
                    D3D10SetVShaderConstantBuffers(context, buffer_sem->binding, 1, &buffer);
                }

                if (buffer_sem->stage_mask & SLANG_STAGE_FRAGMENT_MASK) {
                    D3D10SetPShaderConstantBuffers(context, buffer_sem->binding, 1, &buffer);
                }
            }
        }

        {
            D3D10RenderTargetView null_rt = NULL;
            D3D10SetRenderTargets(context, 1, &null_rt, NULL);
        }

        {
            D3D10ShaderResourceView textures[SLANG_NUM_BINDINGS] = { NULL };
            D3D10SamplerState       samplers[SLANG_NUM_BINDINGS] = { NULL };

            texture_sem_t *texture_sem = d3d10->pass[i].semantics.textures;
            while (texture_sem->stage_mask) {
                int binding       = texture_sem->binding;
                textures[binding] = *(D3D10ShaderResourceView*)texture_sem->texture_data;
                samplers[binding] = d3d10->samplers[texture_sem->filter][texture_sem->wrap];
                texture_sem++;
            }

            D3D10SetPShaderResources(context, 0, SLANG_NUM_BINDINGS, textures);
            D3D10SetPShaderSamplers(context, 0, SLANG_NUM_BINDINGS, samplers);
        }

        if (d3d10->pass[i].rt.handle) {
            D3D10SetRenderTargets(context, 1, &d3d10->pass[i].rt.rt_view, NULL);

            D3D10SetViewports(context, 1, &d3d10->pass[i].viewport);

            D3D10Draw(context, 4, 0);
            texture = &d3d10->pass[i].rt;
        } else {
            texture = NULL;
            break;
        }
    }
}

    D3D10SetRenderTargets(context, 1, &d3d10->renderTargetView, NULL);

    if (texture) {
        d3d10_set_shader(context, &d3d10->shaders[VIDEO_SHADER_STOCK_BLEND]);

        D3D10SetPShaderResources(context, 0, 1, &texture->view);
        D3D10SetPShaderSamplers(context, 0, 1, &d3d10->samplers[RARCH_FILTER_UNSPEC][RARCH_WRAP_DEFAULT]);
        D3D10SetVShaderConstantBuffers(context, 0, 1, &d3d10->frame.ubo);
    }

    D3D10SetViewports(context, 1, &d3d10->frame.viewport);
    D3D10Draw(context, 4, 0);

    return true;
}

void my_d3d10_update_viewport(d3d10_video_t *d3d10, D3D10RenderTargetView renderTargetView, video_viewport_t *viewport) {
    d3d10->renderTargetView = renderTargetView;
    d3d10->vp = *viewport;

    d3d10->frame.viewport.TopLeftX = d3d10->vp.x;
    d3d10->frame.viewport.TopLeftY = d3d10->vp.y;
    d3d10->frame.viewport.Width    = d3d10->vp.width;
    d3d10->frame.viewport.Height   = d3d10->vp.height;
    d3d10->frame.viewport.MaxDepth = 0.0f;
    d3d10->frame.viewport.MaxDepth = 1.0f;

    d3d10->viewport = d3d10->frame.viewport;

    d3d10->ubo_values.OutputSize.width  = d3d10->viewport.Width;
    d3d10->ubo_values.OutputSize.height = d3d10->viewport.Height;

    if (d3d10->shader_preset && (d3d10->frame.output_size.x != d3d10->vp.width || d3d10->frame.output_size.y != d3d10->vp.height)) {
        d3d10->resize_render_targets = true;
    }

    d3d10->frame.output_size.x = d3d10->vp.width;
    d3d10->frame.output_size.y = d3d10->vp.height;
    d3d10->frame.output_size.z = 1.0f / d3d10->vp.width;
    d3d10->frame.output_size.w = 1.0f / d3d10->vp.height;
}
