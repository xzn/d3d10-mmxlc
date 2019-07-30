#ifndef D3D10DEVICE_H
#define D3D10DEVICE_H

#include "main.h"
#include "unknown.h"

class Overlay;
class Config;

class MyID3D10Device : public ID3D10Device {
    template<class T> friend struct LogItem;
    class Impl;
    Impl *impl;

public:
    MyID3D10Device(
        ID3D10Device **inner,
        UINT width,
        UINT height
    );

    virtual ~MyID3D10Device();

    IUNKNOWN_DECL(ID3D10Device)

    void set_overlay(Overlay *overlay);
    void set_config(Config *config);

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
