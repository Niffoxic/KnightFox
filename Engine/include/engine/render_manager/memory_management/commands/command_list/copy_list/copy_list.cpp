#include "pch.h"
#include "copy_list.h"

#include <d3d12.h>
#include <wrl/client.h>

#include "engine/utils/logger/logger.h"
#include "engine/core/exception/base_exception.h"
#include "engine/render_manager/components/device/device.h"
#include "engine/render_manager/memory_management/commands/command_allocator/command_allocator.h"
#include "engine/render_manager/memory_management/pool/allocator_pool/allocator_pool.h"

#pragma region Impl_Declaration

class kfe::KFECopyCommandList::Impl
{
public:
    Impl() = default;
    ~Impl()
    {
        if (!Destroy())
        {
            LOG_ERROR("KFECopyCommandList::Impl::~Impl(): Destroy() failed.");
        }
    }

    NODISCARD bool Initialize(_In_ const KFE_COPY_COMMAND_LIST_CREATE_DESC& desc);
    NODISCARD bool Reset(_In_ const KFE_RESET_COMMAND_LIST& reset);

    NODISCARD bool Close() noexcept;
    NODISCARD bool Destroy() noexcept;

    NODISCARD ID3D12GraphicsCommandList* GetNative() const noexcept;
    NODISCARD bool                       IsInitialized() const noexcept;

    void Update() noexcept;

private:
    bool CreateAllocatorPool(const KFE_COPY_COMMAND_LIST_CREATE_DESC& desc);

private:
    inline static constexpr D3D12_COMMAND_LIST_TYPE   m_type{ D3D12_COMMAND_LIST_TYPE_COPY };
    bool                                              m_bInitialized{ false };
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_pList{ nullptr };
    std::unique_ptr<KFECommandAllocatorPool>          m_pPool{ nullptr };
};

#pragma endregion

#pragma region CCL_Implementation

kfe::KFECopyCommandList::KFECopyCommandList() noexcept
    : m_impl(std::make_unique<kfe::KFECopyCommandList::Impl>())
{
}

kfe::KFECopyCommandList::~KFECopyCommandList() = default;

kfe::KFECopyCommandList::KFECopyCommandList(KFECopyCommandList&&)                    noexcept = default;
kfe::KFECopyCommandList& kfe::KFECopyCommandList::operator=(KFECopyCommandList&&)   noexcept = default;

std::string kfe::KFECopyCommandList::GetName() const noexcept
{
    return "KFECopyCommandList";
}

std::string kfe::KFECopyCommandList::GetDescription() const noexcept
{
    return "DirectX 12 Copy Command List";
}

_Use_decl_annotations_
bool kfe::KFECopyCommandList::Initialize(const KFE_COPY_COMMAND_LIST_CREATE_DESC& desc)
{
    return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFECopyCommandList::Reset(const KFE_RESET_COMMAND_LIST& reset)
{
    return m_impl->Reset(reset);
}

_Use_decl_annotations_
bool kfe::KFECopyCommandList::Close() noexcept
{
    return m_impl->Close();
}

_Use_decl_annotations_
bool kfe::KFECopyCommandList::Destroy() noexcept
{
    return m_impl->Destroy();
}

_Use_decl_annotations_
ID3D12GraphicsCommandList* kfe::KFECopyCommandList::GetNative() const noexcept
{
    return m_impl->GetNative();
}

_Use_decl_annotations_
bool kfe::KFECopyCommandList::IsInitialized() const noexcept
{
    return m_impl->IsInitialized();
}

void kfe::KFECopyCommandList::Update() noexcept
{
    m_impl->Update();
}

#pragma endregion

#pragma region Impl_Implementation

_Use_decl_annotations_
bool kfe::KFECopyCommandList::Impl::Initialize(const KFE_COPY_COMMAND_LIST_CREATE_DESC& desc)
{
    if (m_bInitialized)
        return true;

    if (!desc.Device || !desc.Device->GetNative())
    {
        LOG_ERROR("Failed to initialize Copy command list: device is null!");
        return false;
    }

    if (!CreateAllocatorPool(desc))
    {
        LOG_ERROR("Failed to initialize Copy command list: allocator pool creation failed!");
        return false;
    }

    auto* allocObj = m_pPool->GetCommandAllocatorWait();
    if (!allocObj)
    {
        LOG_ERROR("Allocator pool returned null allocator during CopyCommandList initialization!");
        return false;
    }

    auto* alloc = allocObj->GetNative();
    auto* native = desc.Device->GetNative();

    const HRESULT hr = native->CreateCommandList(
        0u,
        m_type,
        alloc,
        nullptr,
        IID_PPV_ARGS(&m_pList));

    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create Copy command list!");
        return false;
    }

    // Command list starts in recording state; close so we can Reset() later
    m_pList->Close();

    m_bInitialized = true;
    return true;
}

_Use_decl_annotations_
bool kfe::KFECopyCommandList::Impl::Reset(const KFE_RESET_COMMAND_LIST& reset)
{
    if (!m_pPool)
    {
        THROW_MSG("Failed to reset CopyCommandList: no allocator pool is present!");
        return false;
    }

    // Try to get a free allocator
    auto* alloc = m_pPool->GetCommandAllocatorWait();
    if (!alloc)
    {
        LOG_WARNING("No free allocator from pool for CopyCommandList, trying to create a new one.");
        alloc = m_pPool->GetCommandAllocatorCreate();

        if (!alloc)
        {
            THROW_MSG("Failed to acquire allocator for CopyCommandList: no free or new allocators available!");
            return false;
        }
    }

    // Make sure allocator has a native pointer
    if (!alloc->GetNative())
    {
        THROW_MSG("CopyCommandList allocator has no native ID3D12CommandAllocator*!");
        return false;
    }

    // Reset allocator before reusing it for the command list
    if (!alloc->Reset())
    {
        THROW_MSG("Failed to reset command allocator in KFECopyCommandList::Impl::Reset!");
        return false;
    }

    auto* nativeAlloc = alloc->GetNative();

    // Reset the copy command list with allocator
    HRESULT hr = m_pList->Reset(nativeAlloc, nullptr);
    if (FAILED(hr))
    {
        LOG_ERROR("Copy command list Reset FAILED! HRESULT = 0x{:08X}", static_cast<unsigned>(hr));
        return false;
    }

    // Attach fence info so allocator knows when it's safe to reuse
    if (reset.Fence)
    {
        KFE_CA_ATTACH_FENCE fence{};
        fence.Fence          = reset.Fence;
        fence.FenceWaitValue = reset.FenceValue;

        if (!alloc->AttachFence(fence))
        {
            LOG_WARNING("Failed to attach fence to allocator after CopyCommandList reset.");
        }
    }

    LOG_SUCCESS("Copy command list Reset: Success!");
    return true;
}

_Use_decl_annotations_
bool kfe::KFECopyCommandList::Impl::Close() noexcept
{
    if (!m_pList)
        return false;

    const HRESULT hr = m_pList->Close();

    if (FAILED(hr))
    {
        LOG_ERROR("Failed to close Copy command list! 'KFECopyCommandList::Impl::Close()'");
        return false;
    }

    return true;
}

_Use_decl_annotations_
bool kfe::KFECopyCommandList::Impl::Destroy() noexcept
{
    if (!m_bInitialized)
        return true;

    if (m_pPool)
    {
        if (!m_pPool->DestroyAllForce())
        {
            LOG_ERROR("KFECopyCommandList::Impl::Destroy(): DestroyAllForce() failed.");
        }
        m_pPool.reset();
    }

    m_pList.Reset();
    m_bInitialized = false;
    return true;
}

_Use_decl_annotations_
ID3D12GraphicsCommandList* kfe::KFECopyCommandList::Impl::GetNative() const noexcept
{
    return m_pList.Get();
}

_Use_decl_annotations_
bool kfe::KFECopyCommandList::Impl::IsInitialized() const noexcept
{
    return m_bInitialized;
}

bool kfe::KFECopyCommandList::Impl::CreateAllocatorPool(const KFE_COPY_COMMAND_LIST_CREATE_DESC& desc)
{
    if (m_pPool) return true;

    m_pPool = std::make_unique<KFECommandAllocatorPool>();

    KFE_CA_POOL_CREATE_DESC pool{};
    pool.BlockMaxTime   = desc.BlockMaxTime;
    pool.CmdListType    = m_type;
    pool.Device         = desc.Device;
    pool.InitialCounts  = desc.InitialCounts;
    pool.MaxCounts      = desc.MaxCounts;

    return m_pPool->Initialize(pool);
}

void kfe::KFECopyCommandList::Impl::Update() noexcept
{
    if (m_pPool)
    {
        m_pPool->UpdateAllocators();
    }
}

#pragma endregion
