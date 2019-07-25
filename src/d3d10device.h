#ifndef D3D10DEVICE_H
#define D3D10DEVICE_H

#include "main.h"
#include "unknown.h"
#include "../RetroArch/gfx/drivers/d3d10.h"

#define MAX_SAMPLERS 16
#define MAX_SHADER_RESOURCES 128
#define MAX_CONSTANT_BUFFERS 15

class MyID3D10PixelShader;
class MyID3D10Buffer;
class MyID3D10SamplerState;
class MyID3D10RenderTargetView;
class MyID3D10ShaderResourceView;
class MyID3D10DepthStencilView;
class MyID3D10Texture2D;
class TextureAndViews;
class TextureViewsAndBuffer;

class MyID3D10Device : public ID3D10Device {
    d3d10_video_t *d3d10 = NULL;
    d3d10_video_t *d3d10_3d = NULL;
    UINT64 frame_count = 0;

    struct FilterState {
        MyID3D10ShaderResourceView *srv;
        MyID3D10Texture2D *rtv_tex;
        MyID3D10PixelShader *ps;
        bool t1;
        MyID3D10SamplerState *psss;
        bool x4;
        UINT start_vertex_location;
    } filter_state = {};
    bool filter = false;
    void clear_filter();

    struct Config {
        bool interp;
        bool linear;
        bool enhanced;
    } config = {};
    void update_config();

    static const DXGI_FORMAT TEX_FORMAT;
    void create_sampler(
        D3D10_FILTER filter,
        ID3D10SamplerState *&sampler
    );
    void create_texture(
        UINT width,
        UINT height,
        ID3D10Texture2D *&texture,
        DXGI_FORMAT format = TEX_FORMAT
    );
    void create_texture_mul(
        UINT &orig_width,
        UINT &orig_height,
        ID3D10Texture2D *&texture
    );
    void create_rtv(
        ID3D10Texture2D *tex,
        ID3D10RenderTargetView *&rtv,
        DXGI_FORMAT format = TEX_FORMAT
    );
    void create_srv(
        ID3D10Texture2D *tex,
        ID3D10ShaderResourceView *&srv,
        DXGI_FORMAT format = TEX_FORMAT
    );
    void create_dsv(
        ID3D10Texture2D *tex,
        ID3D10DepthStencilView *&dsv,
        DXGI_FORMAT format
    );
    bool set_render_tex_views_and_update(
        MyID3D10Texture2D *tex,
        UINT width,
        UINT height,
        UINT orig_width,
        UINT orig_height,
        bool need_vp = false
    );
    bool set_render_tex_views_and_update(
        ID3D10Resource *r,
        bool need_vp = false
    );
    void create_tex_and_views_nn(
        TextureAndViews *tex,
        UINT orig_width,
        UINT orig_height
    );
    void create_tex_and_view_1(
        TextureViewsAndBuffer *tex,
        UINT width,
        UINT height,
        UINT orig_width,
        UINT orig_height
    );
    void create_tex_and_view_1_v(
        std::vector<TextureViewsAndBuffer *> &tex_v,
        UINT orig_width,
        UINT orig_height
    );

    struct FilterTemp {
        ID3D10SamplerState *sampler_nn;
        ID3D10SamplerState *sampler_linear;
        TextureAndViews *tex_nn_x1;
        TextureAndViews *tex_nn_x4;
        std::vector<TextureViewsAndBuffer *>tex_1_x1;
        std::vector<TextureViewsAndBuffer *>tex_1_x4;
    } filter_temp = {};
    void filter_temp_init();
    void filter_temp_shutdown();

    struct Size {
        UINT sc_width;
        UINT sc_height;
        UINT render_width;
        UINT render_height;
        UINT render_3d_width;
        UINT render_3d_height;
        void resize(UINT width, UINT height);
    } cached_size = {}, render_size = {};

    MyID3D10PixelShader *cached_ps = NULL;
    ID3D10VertexShader *cached_vs = NULL;
    ID3D10GeometryShader *cached_gs = NULL;
    ID3D10InputLayout *cached_il = NULL;
    MyID3D10SamplerState *cached_psss = NULL;
    MyID3D10SamplerState *cached_pssss[MAX_SAMPLERS] = {};
    ID3D10SamplerState *render_pssss[MAX_SAMPLERS] = {};
    MyID3D10RenderTargetView *cached_rtv = NULL;
    MyID3D10DepthStencilView *cached_dsv = NULL;
    MyID3D10ShaderResourceView *cached_pssrv = NULL;
    MyID3D10ShaderResourceView *cached_pssrvs[MAX_SHADER_RESOURCES] = {};
    ID3D10ShaderResourceView *render_pssrvs[MAX_SHADER_RESOURCES] = {};
    D3D10_PRIMITIVE_TOPOLOGY cached_pt = D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED;
    struct BlendState {
        ID3D10BlendState *pBlendState;
        FLOAT BlendFactor[4];
        UINT SampleMask;
    } cached_bs = {};
    struct VertexBuffers {
        ID3D10Buffer *ppVertexBuffers[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
        UINT pStrides[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
        UINT pOffsets[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    } cached_vbs = {};
    ID3D10Buffer *cached_pscbs[MAX_CONSTANT_BUFFERS] = {};
    ID3D10Buffer *cached_vscbs[MAX_CONSTANT_BUFFERS] = {};
    UINT render_width = 0;
    UINT render_height = 0;
    UINT render_orig_width = 0;
    UINT render_orig_height = 0;
    D3D10_VIEWPORT render_vp = {};
    D3D10_VIEWPORT cached_vp = {};
    bool need_render_vp = false;
    bool is_render_vp = false;
    void set_render_vp();
    void reset_render_vp();
    bool render_3d = false;
    UINT render_3d_width = 0;
    UINT render_3d_height = 0;

public:
    MyID3D10Device(
        ID3D10Device **inner,
        UINT width,
        UINT height
    );

    virtual ~MyID3D10Device();

    IUNKNOWN_DECL(MyID3D10Device, ID3D10Device)

    void present();
    void resize_render_3d(UINT width, UINT height);
    void resize_buffers(UINT width, UINT height);
    void resize_orig_buffers(UINT width, UINT height);

    // ID3D10Device

    virtual void STDMETHODCALLTYPE VSSetConstantBuffers(
        UINT StartSlot,
        UINT NumBuffers,
        ID3D10Buffer *const *ppConstantBuffers
    );

    virtual void STDMETHODCALLTYPE PSSetShaderResources(
        UINT StartSlot,
        UINT NumViews,
        ID3D10ShaderResourceView *const *ppShaderResourceViews
    );

    virtual void STDMETHODCALLTYPE PSSetShader(
        ID3D10PixelShader *pPixelShader
    );

    virtual void STDMETHODCALLTYPE PSSetSamplers(
        UINT StartSlot,
        UINT NumSamplers,
        ID3D10SamplerState *const *ppSamplers
    );

    virtual void STDMETHODCALLTYPE VSSetShader(
        ID3D10VertexShader *pVertexShader
    );

    virtual void STDMETHODCALLTYPE DrawIndexed(
        UINT IndexCount,
        UINT StartIndexLocation,
        INT BaseVertexLocation
    );

    virtual void STDMETHODCALLTYPE Draw(
        UINT VertexCount,
        UINT StartVertexLocation
    );

    virtual void STDMETHODCALLTYPE PSSetConstantBuffers(
        UINT StartSlot,
        UINT NumBuffers,
        ID3D10Buffer *const *ppConstantBuffers
    );

    virtual void STDMETHODCALLTYPE IASetInputLayout(
        ID3D10InputLayout *pInputLayout
    );

    virtual void STDMETHODCALLTYPE IASetVertexBuffers(
        UINT StartSlot,
        UINT NumBuffers,
        ID3D10Buffer *const *ppVertexBuffers,
        const UINT *pStrides,
        const UINT *pOffsets
    );

    virtual void STDMETHODCALLTYPE IASetIndexBuffer(
        ID3D10Buffer *pIndexBuffer,
        DXGI_FORMAT Format,
        UINT Offset
    );

    virtual void STDMETHODCALLTYPE DrawIndexedInstanced(
        UINT IndexCountPerInstance,
        UINT InstanceCount,
        UINT StartIndexLocation,
        INT BaseVertexLocation,
        UINT StartInstanceLocation
    );

    virtual void STDMETHODCALLTYPE DrawInstanced(
        UINT VertexCountPerInstance,
        UINT InstanceCount,
        UINT StartVertexLocation,
        UINT StartInstanceLocation
    );

    virtual void STDMETHODCALLTYPE GSSetConstantBuffers(
        UINT StartSlot,
        UINT NumBuffers,
        ID3D10Buffer *const *ppConstantBuffers
    );

    virtual void STDMETHODCALLTYPE GSSetShader(
        ID3D10GeometryShader *pShader
    );

    virtual void STDMETHODCALLTYPE IASetPrimitiveTopology(
        D3D10_PRIMITIVE_TOPOLOGY Topology
    );

    virtual void STDMETHODCALLTYPE VSSetShaderResources(
        UINT StartSlot,
        UINT NumViews,
        ID3D10ShaderResourceView *const *ppShaderResourceViews
    );

    virtual void STDMETHODCALLTYPE VSSetSamplers(
        UINT StartSlot,
        UINT NumSamplers,
        ID3D10SamplerState *const *ppSamplers
    );

    virtual void STDMETHODCALLTYPE SetPredication(
        ID3D10Predicate *pPredicate,
        WINBOOL PredicateValue
    );

    virtual void STDMETHODCALLTYPE GSSetShaderResources(
        UINT StartSlot,
        UINT NumViews,
        ID3D10ShaderResourceView *const *ppShaderResourceViews
    );

    virtual void STDMETHODCALLTYPE GSSetSamplers(
        UINT StartSlot,
        UINT NumSamplers,
        ID3D10SamplerState *const *ppSamplers
    );

    virtual void STDMETHODCALLTYPE OMSetRenderTargets(
        UINT NumViews,
        ID3D10RenderTargetView *const *ppRenderTargetViews,
        ID3D10DepthStencilView *pDepthStencilView
    );

    virtual void STDMETHODCALLTYPE OMSetBlendState(
        ID3D10BlendState *pBlendState,
        const FLOAT BlendFactor[4],
        UINT SampleMask
    );

    virtual void STDMETHODCALLTYPE OMSetDepthStencilState(
        ID3D10DepthStencilState *pDepthStencilState,
        UINT StencilRef
    );

    virtual void STDMETHODCALLTYPE SOSetTargets(
        UINT NumBuffers,
        ID3D10Buffer *const *ppSOTargets,
        const UINT *pOffsets
    );

    virtual void STDMETHODCALLTYPE DrawAuto();

    virtual void STDMETHODCALLTYPE RSSetState(
        ID3D10RasterizerState *pRasterizerState
    );

    virtual void STDMETHODCALLTYPE RSSetViewports(
        UINT NumViewports,
        const D3D10_VIEWPORT *pViewports
    );

    virtual void STDMETHODCALLTYPE RSSetScissorRects(
        UINT NumRects,
        const D3D10_RECT *pRects
    );

    virtual void STDMETHODCALLTYPE CopySubresourceRegion(
        ID3D10Resource *pDstResource,
        UINT DstSubresource,
        UINT DstX,
        UINT DstY,
        UINT DstZ,
        ID3D10Resource *pSrcResource,
        UINT SrcSubresource,
        const D3D10_BOX *pSrcBox
    );

    virtual void STDMETHODCALLTYPE CopyResource(
        ID3D10Resource *pDstResource,
        ID3D10Resource *pSrcResource
    );

    virtual void STDMETHODCALLTYPE UpdateSubresource(
        ID3D10Resource *pDstResource,
        UINT DstSubresource,
        const D3D10_BOX *pDstBox,
        const void *pSrcData,
        UINT SrcRowPitch,
        UINT SrcDepthPitch
    );

    virtual void STDMETHODCALLTYPE ClearRenderTargetView(
        ID3D10RenderTargetView *pRenderTargetView,
        const FLOAT ColorRGBA[4]
    );

    virtual void STDMETHODCALLTYPE ClearDepthStencilView(
        ID3D10DepthStencilView *pDepthStencilView,
        UINT ClearFlags,
        FLOAT Depth,
        UINT8 Stencil
    );

    virtual void STDMETHODCALLTYPE GenerateMips(
        ID3D10ShaderResourceView *pShaderResourceView
    );

    virtual void STDMETHODCALLTYPE ResolveSubresource(
        ID3D10Resource *pDstResource,
        UINT DstSubresource,
        ID3D10Resource *pSrcResource,
        UINT SrcSubresource,
        DXGI_FORMAT Format
    );

    virtual void STDMETHODCALLTYPE VSGetConstantBuffers(
        UINT StartSlot,
        UINT NumBuffers,
        ID3D10Buffer **ppConstantBuffers
    );

    virtual void STDMETHODCALLTYPE PSGetShaderResources(
        UINT StartSlot,
        UINT NumViews,
        ID3D10ShaderResourceView **ppShaderResourceViews
    );

    virtual void STDMETHODCALLTYPE PSGetShader(
        ID3D10PixelShader **ppPixelShader
    );

    virtual void STDMETHODCALLTYPE PSGetSamplers(
        UINT StartSlot,
        UINT NumSamplers,
        ID3D10SamplerState **ppSamplers
    );

    virtual void STDMETHODCALLTYPE VSGetShader(
        ID3D10VertexShader **ppVertexShader
    );

    virtual void STDMETHODCALLTYPE PSGetConstantBuffers(
        UINT StartSlot,
        UINT NumBuffers,
        ID3D10Buffer **ppConstantBuffers
    );

    virtual void STDMETHODCALLTYPE IAGetInputLayout(
        ID3D10InputLayout **ppInputLayout
    );

    virtual void STDMETHODCALLTYPE IAGetVertexBuffers(
        UINT StartSlot,
        UINT NumBuffers,
        ID3D10Buffer **ppVertexBuffers,
        UINT *pStrides,
        UINT *pOffsets
    );

    virtual void STDMETHODCALLTYPE IAGetIndexBuffer(
        ID3D10Buffer **pIndexBuffer,
        DXGI_FORMAT *Format,
        UINT *Offset
    );

    virtual void STDMETHODCALLTYPE GSGetConstantBuffers(
        UINT StartSlot,
        UINT NumBuffers,
        ID3D10Buffer **ppConstantBuffers
    );

    virtual void STDMETHODCALLTYPE GSGetShader(
        ID3D10GeometryShader **ppGeometryShader
    );

    virtual void STDMETHODCALLTYPE IAGetPrimitiveTopology(
        D3D10_PRIMITIVE_TOPOLOGY *pTopology
    );

    virtual void STDMETHODCALLTYPE VSGetShaderResources(
        UINT StartSlot,
        UINT NumViews,
        ID3D10ShaderResourceView **ppShaderResourceViews
    );

    virtual void STDMETHODCALLTYPE VSGetSamplers(
        UINT StartSlot,
        UINT NumSamplers,
        ID3D10SamplerState **ppSamplers
    );

    virtual void STDMETHODCALLTYPE GetPredication(
        ID3D10Predicate **ppPredicate,
        WINBOOL *pPredicateValue
    );

    virtual void STDMETHODCALLTYPE GSGetShaderResources(
        UINT StartSlot,
        UINT NumViews,
        ID3D10ShaderResourceView **ppShaderResourceViews
    );

    virtual void STDMETHODCALLTYPE GSGetSamplers(
        UINT StartSlot,
        UINT NumSamplers,
        ID3D10SamplerState **ppSamplers
    );

    virtual void STDMETHODCALLTYPE OMGetRenderTargets(
        UINT NumViews,
        ID3D10RenderTargetView **ppRenderTargetViews,
        ID3D10DepthStencilView **ppDepthStencilView
    );

    virtual void STDMETHODCALLTYPE OMGetBlendState(
        ID3D10BlendState **ppBlendState,
        FLOAT BlendFactor[4],
        UINT *pSampleMask
    );

    virtual void STDMETHODCALLTYPE OMGetDepthStencilState(
        ID3D10DepthStencilState **ppDepthStencilState,
        UINT *pStencilRef
    );

    virtual void STDMETHODCALLTYPE SOGetTargets(
        UINT NumBuffers,
        ID3D10Buffer **ppSOTargets,
        UINT *pOffsets
    );

    virtual void STDMETHODCALLTYPE RSGetState(
        ID3D10RasterizerState **ppRasterizerState
    );

    virtual void STDMETHODCALLTYPE RSGetViewports(
        UINT *NumViewports,
        D3D10_VIEWPORT *pViewports
    );

    virtual void STDMETHODCALLTYPE RSGetScissorRects(
        UINT *NumRects,
        D3D10_RECT *pRects
    );

    virtual HRESULT STDMETHODCALLTYPE GetDeviceRemovedReason();

    virtual HRESULT STDMETHODCALLTYPE SetExceptionMode(
        UINT RaiseFlags
    );

    virtual UINT STDMETHODCALLTYPE GetExceptionMode();

    virtual HRESULT STDMETHODCALLTYPE GetPrivateData(
        REFGUID guid,
        UINT *pDataSize,
        void *pData
    );

    virtual HRESULT STDMETHODCALLTYPE SetPrivateData(
        REFGUID guid,
        UINT DataSize,
        const void *pData
    );

    virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(
        REFGUID guid,
        const IUnknown *pData
    );

    virtual void STDMETHODCALLTYPE ClearState();

    virtual void STDMETHODCALLTYPE Flush();

    virtual HRESULT STDMETHODCALLTYPE CreateBuffer(
        const D3D10_BUFFER_DESC *pDesc,
        const D3D10_SUBRESOURCE_DATA *pInitialData,
        ID3D10Buffer **ppBuffer
    );

    virtual HRESULT STDMETHODCALLTYPE CreateTexture1D(
        const D3D10_TEXTURE1D_DESC *pDesc,
        const D3D10_SUBRESOURCE_DATA *pInitialData,
        ID3D10Texture1D **ppTexture1D
    );

    virtual HRESULT STDMETHODCALLTYPE CreateTexture2D(
        const D3D10_TEXTURE2D_DESC *pDesc,
        const D3D10_SUBRESOURCE_DATA *pInitialData,
        ID3D10Texture2D **ppTexture2D
    );

    virtual HRESULT STDMETHODCALLTYPE CreateTexture3D(
        const D3D10_TEXTURE3D_DESC *pDesc,
        const D3D10_SUBRESOURCE_DATA *pInitialData,
        ID3D10Texture3D **ppTexture3D
    );

    virtual HRESULT STDMETHODCALLTYPE CreateShaderResourceView(
        ID3D10Resource *pResource,
        const D3D10_SHADER_RESOURCE_VIEW_DESC *pDesc,
        ID3D10ShaderResourceView **ppSRView
    );

    virtual HRESULT STDMETHODCALLTYPE CreateRenderTargetView(
        ID3D10Resource *pResource,
        const D3D10_RENDER_TARGET_VIEW_DESC *pDesc,
        ID3D10RenderTargetView **ppRTView
    );

    virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilView(
        ID3D10Resource *pResource,
        const D3D10_DEPTH_STENCIL_VIEW_DESC *pDesc,
        ID3D10DepthStencilView **ppDepthStencilView
    );

    virtual HRESULT STDMETHODCALLTYPE CreateInputLayout(
        const D3D10_INPUT_ELEMENT_DESC *pInputElementDescs,
        UINT NumElements,
        const void *pShaderBytecodeWithInputSignature,
        SIZE_T BytecodeLength,
        ID3D10InputLayout **ppInputLayout
    );

    virtual HRESULT STDMETHODCALLTYPE CreateVertexShader(
        const void *pShaderBytecode,
        SIZE_T BytecodeLength,
        ID3D10VertexShader **ppVertexShader
    );

    virtual HRESULT STDMETHODCALLTYPE CreateGeometryShader(
        const void *pShaderBytecode,
        SIZE_T BytecodeLength,
        ID3D10GeometryShader **ppGeometryShader
    );

    virtual HRESULT STDMETHODCALLTYPE CreateGeometryShaderWithStreamOutput(
        const void *pShaderBytecode,
        SIZE_T BytecodeLength,
        const D3D10_SO_DECLARATION_ENTRY *pSODeclaration,
        UINT NumEntries,
        UINT OutputStreamStride,
        ID3D10GeometryShader **ppGeometryShader
    );

    virtual HRESULT STDMETHODCALLTYPE CreatePixelShader(
        const void *pShaderBytecode,
        SIZE_T BytecodeLength,
        ID3D10PixelShader **ppPixelShader
    );

    virtual HRESULT STDMETHODCALLTYPE CreateBlendState(
        const D3D10_BLEND_DESC *pBlendStateDesc,
        ID3D10BlendState **ppBlendState
    );

    virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilState(
        const D3D10_DEPTH_STENCIL_DESC *pDepthStencilDesc,
        ID3D10DepthStencilState **ppDepthStencilState
    );

    virtual HRESULT STDMETHODCALLTYPE CreateRasterizerState(
        const D3D10_RASTERIZER_DESC *pRasterizerDesc,
        ID3D10RasterizerState **ppRasterizerState
    );

    virtual HRESULT STDMETHODCALLTYPE CreateSamplerState(
        const D3D10_SAMPLER_DESC *pSamplerDesc,
        ID3D10SamplerState **ppSamplerState
    );

    virtual HRESULT STDMETHODCALLTYPE CreateQuery(
        const D3D10_QUERY_DESC *pQueryDesc,
        ID3D10Query **ppQuery
    );

    virtual HRESULT STDMETHODCALLTYPE CreatePredicate(
        const D3D10_QUERY_DESC *pPredicateDesc,
        ID3D10Predicate **ppPredicate
    );

    virtual HRESULT STDMETHODCALLTYPE CreateCounter(
        const D3D10_COUNTER_DESC *pCounterDesc,
        ID3D10Counter **ppCounter
    );

    virtual HRESULT STDMETHODCALLTYPE CheckFormatSupport(
        DXGI_FORMAT Format,
        UINT *pFormatSupport
    );

    virtual HRESULT STDMETHODCALLTYPE CheckMultisampleQualityLevels(
        DXGI_FORMAT Format,
        UINT SampleCount,
        UINT *pNumQualityLevels
    );

    virtual void STDMETHODCALLTYPE CheckCounterInfo(
        D3D10_COUNTER_INFO *pCounterInfo
    );

    virtual HRESULT STDMETHODCALLTYPE CheckCounter(
        const D3D10_COUNTER_DESC *pDesc,
        D3D10_COUNTER_TYPE *pType,
        UINT *pActiveCounters,
        char *name,
        UINT *pNameLength,
        char *units,
        UINT *pUnitsLength,
        char *description,
        UINT *pDescriptionLength
    );

    virtual UINT STDMETHODCALLTYPE GetCreationFlags();

    virtual HRESULT STDMETHODCALLTYPE OpenSharedResource(
        HANDLE hResource,
        REFIID ReturnedInterface,
        void **ppResource
    );

    virtual void STDMETHODCALLTYPE SetTextFilterSize(
        UINT Width,
        UINT Height
    );

    virtual void STDMETHODCALLTYPE GetTextFilterSize(
        UINT *pWidth,
        UINT *pHeight
    );
};

#endif
