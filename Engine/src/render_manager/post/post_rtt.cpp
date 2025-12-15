// This is a personal academic project. Dear PVS-Studio, please check it.

#include "pch.h"
#include "engine/render_manager/post/post_rtt.h"

#include "engine/render_manager/api/components/device.h"
#include "engine/render_manager/api/texture/texture.h"
#include "engine/render_manager/api/texture/texture_rtv.h"
#include "engine/render_manager/api/texture/texture_srv.h"

#include "engine/render_manager/api/heap/heap_rtv.h"
#include "engine/render_manager/api/heap/heap_cbv_srv_uav.h"

#include "engine/utils/logger.h"

#include <utility>

class kfe::KFERenderTargetTexture::Impl
{
public:
    NODISCARD bool Initialize(_In_ const KFE_RT_TEXTURE_CREATE_DESC& desc);
    NODISCARD bool Destroy() noexcept;
    NODISCARD bool IsInitialized() const noexcept;

    NODISCARD KFE_RT_DRAW_STATE GetDrawState() const noexcept { return m_drawState; }
    NODISCARD D3D12_RESOURCE_STATES GetResourceState() const noexcept { return m_state; }
    void SetDrawState(KFE_RT_DRAW_STATE s) noexcept { m_drawState = s; }

    NODISCARD bool Transition(_In_ ID3D12GraphicsCommandList* cmd,
        _In_ KFE_RT_DRAW_STATE target) noexcept;

    NODISCARD bool SetAsRenderTarget(_In_ ID3D12GraphicsCommandList* cmd,
        _In_opt_ const D3D12_CPU_DESCRIPTOR_HANDLE* dsv) noexcept;

    NODISCARD bool BindSRV(_In_ ID3D12GraphicsCommandList* cmd,
        _In_ std::uint32_t rootParamIndex) noexcept;

    NODISCARD KFETexture* GetTexture() const noexcept { return m_texture.get(); }
    NODISCARD KFETextureRTV* GetRTV() const noexcept { return m_rtv.get(); }
    NODISCARD KFETextureSRV* GetSRV() const noexcept { return m_srv.get(); }

    NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPU() const noexcept;
    NODISCARD D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPU() const noexcept;

    NODISCARD std::uint32_t GetRTVIndex() const noexcept;
    NODISCARD std::uint32_t GetSRVIndex() const noexcept;

private:
    NODISCARD bool Barrier(_In_ ID3D12GraphicsCommandList* cmd,
        _In_ D3D12_RESOURCE_STATES after) noexcept;

private:
    bool m_bInitialized{ false };

    KFEDevice* m_device{ nullptr };
    KFERTVHeap* m_rtvHeap{ nullptr };
    KFEResourceHeap* m_srvHeap{ nullptr };

    std::unique_ptr<KFETexture>    m_texture;
    std::unique_ptr<KFETextureRTV> m_rtv;
    std::unique_ptr<KFETextureSRV> m_srv;

    KFE_RT_DRAW_STATE      m_drawState{ KFE_RT_DRAW_STATE::Unknown };
    D3D12_RESOURCE_STATES  m_state{ D3D12_RESOURCE_STATE_COMMON };
};

#pragma region Public_Wrapper

kfe::KFERenderTargetTexture::KFERenderTargetTexture() noexcept
    : m_impl(std::make_unique<KFERenderTargetTexture::Impl>())
{
}

kfe::KFERenderTargetTexture::~KFERenderTargetTexture() noexcept = default;

kfe::KFERenderTargetTexture::KFERenderTargetTexture(KFERenderTargetTexture&&) noexcept = default;
kfe::KFERenderTargetTexture& kfe::KFERenderTargetTexture::operator=(KFERenderTargetTexture&&) noexcept = default;

_Use_decl_annotations_
bool kfe::KFERenderTargetTexture::Initialize(const KFE_RT_TEXTURE_CREATE_DESC& desc)
{
    return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFERenderTargetTexture::Destroy() noexcept
{
    return m_impl->Destroy();
}

_Use_decl_annotations_
bool kfe::KFERenderTargetTexture::IsInitialized() const noexcept
{
    return m_impl->IsInitialized();
}

kfe::KFE_RT_DRAW_STATE kfe::KFERenderTargetTexture::GetDrawState() const noexcept
{
    return m_impl->GetDrawState();
}

D3D12_RESOURCE_STATES kfe::KFERenderTargetTexture::GetResourceState() const noexcept
{
    return m_impl->GetResourceState();
}

void kfe::KFERenderTargetTexture::SetDrawState(KFE_RT_DRAW_STATE state) noexcept
{
    m_impl->SetDrawState(state);
}

_Use_decl_annotations_
bool kfe::KFERenderTargetTexture::Transition(ID3D12GraphicsCommandList* cmd, KFE_RT_DRAW_STATE target) noexcept
{
    return m_impl->Transition(cmd, target);
}

_Use_decl_annotations_
bool kfe::KFERenderTargetTexture::SetAsRenderTarget(ID3D12GraphicsCommandList* cmd,
    const D3D12_CPU_DESCRIPTOR_HANDLE* dsv) noexcept
{
    return m_impl->SetAsRenderTarget(cmd, dsv);
}

_Use_decl_annotations_
bool kfe::KFERenderTargetTexture::BindSRV(ID3D12GraphicsCommandList* cmd,
    std::uint32_t rootParamIndex) noexcept
{
    return m_impl->BindSRV(cmd, rootParamIndex);
}

_Use_decl_annotations_
kfe::KFETexture* kfe::KFERenderTargetTexture::GetTexture() const noexcept { return m_impl->GetTexture(); }

_Use_decl_annotations_
kfe::KFETextureRTV* kfe::KFERenderTargetTexture::GetRTV() const noexcept { return m_impl->GetRTV(); }

_Use_decl_annotations_
kfe::KFETextureSRV* kfe::KFERenderTargetTexture::GetSRV() const noexcept { return m_impl->GetSRV(); }

_Use_decl_annotations_
D3D12_CPU_DESCRIPTOR_HANDLE kfe::KFERenderTargetTexture::GetRTVCPU() const noexcept { return m_impl->GetRTVCPU(); }

_Use_decl_annotations_
D3D12_GPU_DESCRIPTOR_HANDLE kfe::KFERenderTargetTexture::GetSRVGPU() const noexcept { return m_impl->GetSRVGPU(); }

_Use_decl_annotations_
std::uint32_t kfe::KFERenderTargetTexture::GetRTVIndex() const noexcept { return m_impl->GetRTVIndex(); }

_Use_decl_annotations_
std::uint32_t kfe::KFERenderTargetTexture::GetSRVIndex() const noexcept { return m_impl->GetSRVIndex(); }

std::string kfe::KFERenderTargetTexture::GetName() const noexcept { return "KFERenderTargetTexture"; }
std::string kfe::KFERenderTargetTexture::GetDescription() const noexcept
{
    return "KFERenderTargetTexture: Wraps Texture + RTV + SRV + State tracking.";
}

#pragma endregion

#pragma region Impl

_Use_decl_annotations_
bool kfe::KFERenderTargetTexture::Impl::Initialize(const KFE_RT_TEXTURE_CREATE_DESC& desc)
{
    if (m_bInitialized)
    {
        LOG_WARNING("KFERenderTargetTexture::Initialize: Already initialized. Destroying previous.");
        (void)Destroy();
    }

    if (!desc.Device || !desc.RTVHeap || !desc.SRVHeap)
    {
        LOG_ERROR("KFERenderTargetTexture::Initialize: Device/RTVHeap/SRVHeap must be valid.");
        return false;
    }

    if (desc.Width == 0u || desc.Height == 0u)
    {
        LOG_ERROR("KFERenderTargetTexture::Initialize: Width/Height must be > 0.");
        return false;
    }

    m_device = desc.Device;
    m_rtvHeap = desc.RTVHeap;
    m_srvHeap = desc.SRVHeap;

    m_texture = std::make_unique<KFETexture>();
    m_rtv = std::make_unique<KFETextureRTV>();
    m_srv = std::make_unique<KFETextureSRV>();

    //~ Create the resource
    KFE_TEXTURE_CREATE_DESC t{};
    t.Device = desc.Device;
    t.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    t.Width = desc.Width;
    t.Height = desc.Height;
    t.DepthOrArraySize = 1;
    t.MipLevels = desc.MipLevels;
    t.Format = desc.Format;
    t.SampleDesc = desc.SampleDesc;
    t.HeapType = D3D12_HEAP_TYPE_DEFAULT;
    t.ResourceFlags = desc.ResourceFlags;
    t.InitialState = desc.InitialState;
    t.ClearValue = desc.ClearValue;

    if (!m_texture->Initialize(t))
    {
        LOG_ERROR("KFERenderTargetTexture::Initialize: Failed to create texture resource.");
        (void)Destroy();
        return false;
    }

    //~ Create RTV
    KFE_RTV_CREATE_DESC rtvDesc{};
    rtvDesc.Device = desc.Device;
    rtvDesc.Heap = desc.RTVHeap;
    rtvDesc.Texture = m_texture.get();
    rtvDesc.Format = desc.Format;
    rtvDesc.ViewDimension = desc.RTVViewDimension;
    rtvDesc.MipSlice = 0u;
    rtvDesc.PlaneSlice = 0u;
    rtvDesc.DescriptorIndex = desc.RTVDescriptorIndex;

    if (!m_rtv->Initialize(rtvDesc))
    {
        LOG_ERROR("KFERenderTargetTexture::Initialize: Failed to create RTV.");
        (void)Destroy();
        return false;
    }

    //~ Create SRV
    KFE_SRV_CREATE_DESC srvDesc{};
    srvDesc.Device = desc.Device;
    srvDesc.Heap = desc.SRVHeap;
    srvDesc.Texture = m_texture.get();
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = desc.SRVViewDimension;
    srvDesc.MostDetailedMip = 0u;
    srvDesc.MipLevels = desc.MipLevels;
    srvDesc.PlaneSlice = 0u;
    srvDesc.ResourceMinLODClamp = 0.0f;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.DescriptorIndex = desc.SRVDescriptorIndex;

    if (!m_srv->Initialize(srvDesc))
    {
        LOG_ERROR("KFERenderTargetTexture::Initialize: Failed to create SRV.");
        (void)Destroy();
        return false;
    }

    m_state = desc.InitialState;
    m_drawState = (desc.InitialState & D3D12_RESOURCE_STATE_RENDER_TARGET)
        ? KFE_RT_DRAW_STATE::RenderTarget
        : KFE_RT_DRAW_STATE::ShaderResource;

    m_bInitialized = true;

    LOG_SUCCESS("KFERenderTargetTexture::Initialize: Created {}x{} (RTV idx {}, SRV idx {}).",
        desc.Width, desc.Height, GetRTVIndex(), GetSRVIndex());

    return true;
}

_Use_decl_annotations_
bool kfe::KFERenderTargetTexture::Impl::Destroy() noexcept
{
    if (!m_bInitialized && !m_texture)
        return true;

    if (m_rtv) (void)m_rtv->Destroy();
    if (m_srv) (void)m_srv->Destroy();
    if (m_texture) (void)m_texture->Destroy();

    m_rtv    .reset();
    m_srv    .reset();
    m_texture.reset();

    m_device  = nullptr;
    m_rtvHeap = nullptr;
    m_srvHeap = nullptr;

    m_drawState = KFE_RT_DRAW_STATE::Unknown;
    m_state     = D3D12_RESOURCE_STATE_COMMON;

    m_bInitialized = false;
    return true;
}

_Use_decl_annotations_
bool kfe::KFERenderTargetTexture::Impl::IsInitialized() const noexcept
{
    return m_bInitialized && m_texture && m_texture->IsInitialized();
}

_Use_decl_annotations_
bool kfe::KFERenderTargetTexture::Impl::Barrier(ID3D12GraphicsCommandList* cmd,
    D3D12_RESOURCE_STATES after) noexcept
{
    if (!cmd || !IsInitialized())
        return false;

    if (m_state == after)
        return true;

    ID3D12Resource* res = m_texture->GetNative();
    if (!res)
        return false;

    D3D12_RESOURCE_BARRIER b{};
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    b.Transition.pResource = res;
    b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b.Transition.StateBefore = m_state;
    b.Transition.StateAfter = after;

    cmd->ResourceBarrier(1u, &b);
    m_state = after;
    return true;
}

_Use_decl_annotations_
bool kfe::KFERenderTargetTexture::Impl::Transition(ID3D12GraphicsCommandList* cmd,
    KFE_RT_DRAW_STATE target) noexcept
{
    if (!cmd || !IsInitialized())
        return false;

    switch (target)
    {
    case KFE_RT_DRAW_STATE::RenderTarget:
        if (!Barrier(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET))
            return false;
        m_drawState = target;
        return true;

    case KFE_RT_DRAW_STATE::ShaderResource:
        if (!Barrier(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE))
            return false;
        m_drawState = target;
        return true;

    default:
        LOG_WARNING("KFERenderTargetTexture::Transition: Target draw state is Unknown.");
        return false;
    }
}

_Use_decl_annotations_
bool kfe::KFERenderTargetTexture::Impl::SetAsRenderTarget(ID3D12GraphicsCommandList* cmd,
    const D3D12_CPU_DESCRIPTOR_HANDLE* dsv) noexcept
{
    if (!cmd || !IsInitialized() || !m_rtv)
        return false;

    if (m_drawState != KFE_RT_DRAW_STATE::RenderTarget)
    {
        LOG_WARNING("KFERenderTargetTexture::SetAsRenderTarget: Not in RenderTarget state. Call Transition().");
    }

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_rtv->GetCPUHandle();
    if (rtv.ptr == 0)
        return false;

    cmd->OMSetRenderTargets(1u, &rtv, FALSE, dsv);
    return true;
}

_Use_decl_annotations_
bool kfe::KFERenderTargetTexture::Impl::BindSRV(ID3D12GraphicsCommandList* cmd,
    std::uint32_t rootParamIndex) noexcept
{
    if (!cmd || !IsInitialized() || !m_srv || !m_srvHeap)
        return false;

    if (m_drawState != KFE_RT_DRAW_STATE::ShaderResource)
    {
        LOG_WARNING("KFERenderTargetTexture::BindSRV: Not in ShaderResource state. Call Transition().");
    }

    const D3D12_GPU_DESCRIPTOR_HANDLE gpu = GetSRVGPU();
    if (gpu.ptr == 0)
    {
        LOG_ERROR("KFERenderTargetTexture::BindSRV: SRV GPU handle is invalid (heap not shader-visible?).");
        return false;
    }

    cmd->SetGraphicsRootDescriptorTable(rootParamIndex, gpu);
    return true;
}

_Use_decl_annotations_
D3D12_CPU_DESCRIPTOR_HANDLE kfe::KFERenderTargetTexture::Impl::GetRTVCPU() const noexcept
{
    if (!m_rtv) { D3D12_CPU_DESCRIPTOR_HANDLE h{}; h.ptr = 0; return h; }
    return m_rtv->GetCPUHandle();
}

_Use_decl_annotations_
D3D12_GPU_DESCRIPTOR_HANDLE kfe::KFERenderTargetTexture::Impl::GetSRVGPU() const noexcept
{
    D3D12_GPU_DESCRIPTOR_HANDLE h{};
    h.ptr = 0;

    if (!m_srv || !m_srvHeap) return h;
    if (!m_srv->HasValidDescriptor()) return h;

    return m_srvHeap->GetGPUHandle(m_srv->GetDescriptorIndex());
}

_Use_decl_annotations_
std::uint32_t kfe::KFERenderTargetTexture::Impl::GetRTVIndex() const noexcept
{
    return (m_rtv) ? m_rtv->GetDescriptorIndex() : KFE_INVALID_INDEX;
}

_Use_decl_annotations_
std::uint32_t kfe::KFERenderTargetTexture::Impl::GetSRVIndex() const noexcept
{
    return (m_srv) ? m_srv->GetDescriptorIndex() : KFE_INVALID_INDEX;
}

#pragma endregion
