// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "pch.h"
#include "engine/render_manager/light/light_manager.h"

#include "engine/utils/logger.h"

#include <d3d12.h>
#include <utility>
#include <cstring>

namespace
{
    static std::uint64_t BytesForLights(std::uint32_t capacity) noexcept
    {
        return static_cast<std::uint64_t>(sizeof(kfe::KFE_LIGHT_DATA_GPU)) *
            static_cast<std::uint64_t>(capacity);
    }
}

#pragma region Ctor_Dtor_Move

kfe::KFELightManager::KFELightManager() noexcept = default;

kfe::KFELightManager::~KFELightManager() noexcept
{
    (void)Destroy();
}

kfe::KFELightManager::KFELightManager(KFELightManager&& other) noexcept
{
    *this = std::move(other);
}

kfe::KFELightManager& kfe::KFELightManager::operator=(KFELightManager&& other) noexcept
{
    if (this == &other)
        return *this;

    (void)Destroy();

    m_pDevice       = other.m_pDevice;
    m_pHeap         = other.m_pHeap;
    m_capacity      = other.m_capacity;
    m_srvIndex      = other.m_srvIndex;
    m_bInitialized  = other.m_bInitialized;

    m_lights        = std::move(other.m_lights);
    m_lightAccessor = std::move(other.m_lightAccessor);

    m_cpuPacked         = std::move(other.m_cpuPacked);
    m_lastPackedCount   = other.m_lastPackedCount;
    m_bDirty            = other.m_bDirty;

    m_staging           = std::move(other.m_staging);
    m_structuredBuffer  = std::move(other.m_structuredBuffer);

    other.m_pDevice         = nullptr;
    other.m_pHeap           = nullptr;
    other.m_capacity        = 0u;
    other.m_srvIndex        = KFE_INVALID_INDEX;
    other.m_bInitialized    = false;
    other.m_lastPackedCount = 0u;
    other.m_bDirty          = true;

    return *this;
}

#pragma endregion

#pragma region IKFEObject

std::string kfe::KFELightManager::GetName() const noexcept
{
    return "KFELightManager";
}

std::string kfe::KFELightManager::GetDescription() const noexcept
{
    return "Manages IKFELight collection and uploads packed KFE_LIGHT_DATA_DESC[] to a structured SRV.";
}

#pragma endregion

#pragma region Lifetime

_Use_decl_annotations_
bool kfe::KFELightManager::Initialize(const KFE_CREATE_LIGHT_MANAGER& desc) noexcept
{
    if (!desc.Device || !desc.Device->GetNative())
    {
        LOG_ERROR("KFELightManager::Initialize: Device or native device is null.");
        return false;
    }

    if (!desc.Heap)
    {
        LOG_ERROR("KFELightManager::Initialize: Heap is null.");
        return false;
    }

    if (desc.Capacity == 0u)
    {
        LOG_ERROR("KFELightManager::Initialize: Capacity must be > 0.");
        return false;
    }

    if (IsInitialized())
    {
        LOG_WARNING("KFELightManager::Initialize: Already initialized. Destroying and recreating.");
        if (!Destroy())
        {
            LOG_ERROR("KFELightManager::Initialize: Destroy failed; cannot reinitialize.");
            return false;
        }
    }

    m_pDevice = desc.Device;
    m_pHeap = desc.Heap;
    m_capacity = 0u;
    m_srvIndex = KFE_INVALID_INDEX;
    m_bInitialized = false;

    m_lights.clear();
    m_lightAccessor.clear();
    m_cpuPacked.clear();
    m_lastPackedCount = 0u;
    m_bDirty = true;

    if (!CreateGPUResources(desc.Capacity, desc.DebugName ? desc.DebugName : "KFELightManager_Lights"))
        return false;

    m_bInitialized = true;
    m_defaultState = D3D12_RESOURCE_STATE_COPY_DEST;

    LOG_SUCCESS("Initialized. Capacity={}, SRVIndex={}.",
        m_capacity, m_srvIndex);

    return true;
}

bool kfe::KFELightManager::Destroy() noexcept
{
    if (m_structuredBuffer && m_structuredBuffer->IsInitialized())
    {
        (void)m_structuredBuffer->Destroy();
    }
    m_structuredBuffer.reset();

    if (m_staging && m_staging->IsInitialized())
    {
        (void)m_staging->Destroy();
    }
    m_staging.reset();

    m_lights.clear();
    m_lightAccessor.clear();
    m_cpuPacked.clear();

    m_pDevice = nullptr;
    m_pHeap = nullptr;
    m_capacity = 0u;
    m_srvIndex = KFE_INVALID_INDEX;
    m_lastPackedCount = 0u;
    m_bDirty = true;
    m_bInitialized = false;

    return true;
}

bool kfe::KFELightManager::IsInitialized() const noexcept
{
    return m_bInitialized;
}

#pragma endregion

#pragma region Collection

void kfe::KFELightManager::AttachLight(_In_ IKFELight* light)
{
    if (!light)
        return;

    const KID id = light->GetAssignedKey();

    if (m_lights.contains(id)) return;

    m_lights[id] = light;

    LOG_SUCCESS("Light Attached to the manager!");
    RebuildAccessor();
    MarkDirty      ();
}

void kfe::KFELightManager::DetachLight(_In_ IKFELight* light)
{
    if (!light)
        return;

    const KID id = light->GetAssignedKey();
    DetachLight(id);
}

void kfe::KFELightManager::DetachLight(_In_ KID lightId)
{
    auto it = m_lights.find(lightId);
    if (it == m_lights.end())
        return;

    m_lights.erase(it);

    RebuildAccessor();
    MarkDirty();
}

void kfe::KFELightManager::SetDrawState(
    ID3D12GraphicsCommandList* cmdList,
    D3D12_RESOURCE_STATES before)
{
    if (!cmdList || !m_staging || !m_staging->IsInitialized())
        return;

    auto* defBuf = m_staging->GetDefaultBuffer();
    if (!defBuf || !defBuf->GetNative())
        return;

    const auto shaderState =
        static_cast<D3D12_RESOURCE_STATES>(
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    if (before == shaderState)
        return;

    D3D12_RESOURCE_BARRIER b{};
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    b.Transition.pResource = defBuf->GetNative();
    b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b.Transition.StateBefore = before;
    b.Transition.StateAfter = shaderState;

    cmdList->ResourceBarrier(1u, &b);

    m_defaultState = shaderState;
}

void kfe::KFELightManager::ClearLights() noexcept
{
    if (m_lights.empty() && m_lightAccessor.empty())
        return;

    m_lights.clear();
    m_lightAccessor.clear();
    MarkDirty();
}

bool kfe::KFELightManager::Contains(_In_ KID lightId) const noexcept
{
    return (m_lights.find(lightId) != m_lights.end());
}

std::uint32_t kfe::KFELightManager::GetLightCounts() const noexcept
{
    return static_cast<std::uint32_t>(m_lights.size());
}

std::uint32_t kfe::KFELightManager::GetCapacity() const noexcept
{
    return m_capacity;
}

const std::vector<kfe::IKFELight*>& kfe::KFELightManager::GetAllLights() const noexcept
{
    return m_lightAccessor;
}

#pragma endregion

#pragma region Dirty_Pack_Upload

void kfe::KFELightManager::MarkDirty() noexcept
{
    m_bDirty = true;
}

void kfe::KFELightManager::PackData() noexcept
{
    if (!IsInitialized())
        return;

    if (m_lightAccessor.size() != m_lights.size())
        RebuildAccessor();

    const std::uint32_t active    = static_cast<std::uint32_t>(m_lightAccessor.size());
    const std::uint32_t packCount = (active > m_capacity) ? m_capacity : active;

    if (m_cpuPacked.size() != m_capacity)
        m_cpuPacked.resize(m_capacity);

    if (packCount > 0)
        std::memset(m_cpuPacked.data(), 0, sizeof(KFE_LIGHT_DATA_GPU) * packCount);

    for (std::uint32_t i = 0u; i < packCount; ++i)
    {
        const IKFELight* src = m_lightAccessor[i];
        PackOne(src, m_cpuPacked[i]);
    }

    m_lastPackedCount = packCount;
    m_bDirty = false;
}

_Use_decl_annotations_
bool kfe::KFELightManager::RecordUpload(ID3D12GraphicsCommandList* cmdList) noexcept
{
    if (!IsInitialized())
    {
        LOG_ERROR("KFELightManager::RecordUpload: Not initialized.");
        return false;
    }

    if (!cmdList)
    {
        LOG_ERROR("KFELightManager::RecordUpload: cmdList is null.");
        return false;
    }

    if (m_bDirty)
        PackData();

    if (!m_staging || !m_staging->IsInitialized())
    {
        LOG_ERROR("KFELightManager::RecordUpload: Staging buffer not initialized.");
        return false;
    }

    const std::uint32_t count = m_lastPackedCount;
    if (count == 0u)
        return true;

    const std::uint64_t bytes =
        static_cast<std::uint64_t>(sizeof(KFE_LIGHT_DATA_GPU)) *
        static_cast<std::uint64_t>(count);

    if (!m_staging->WriteBytes(m_cpuPacked.data(), bytes, 0u))
    {
        LOG_ERROR("KFELightManager::RecordUpload: WriteBytes failed.");
        return false;
    }

    const auto shaderState =
        static_cast<D3D12_RESOURCE_STATES>(
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    if (!m_staging->RecordUploadToDefaultWithBarriers(
        cmdList,
        bytes,
        0u,
        0u,
        m_defaultState,
        shaderState))
    {
        LOG_ERROR("RecordUploadToDefaultWithBarriers failed.");
        return false;
    }

    m_defaultState = shaderState;

    return true;
}


_Use_decl_annotations_
bool kfe::KFELightManager::UpdateAndRecord(ID3D12GraphicsCommandList* cmdList) noexcept
{
    if (m_bDirty)
        PackData();

    return RecordUpload(cmdList);
}

std::uint32_t kfe::KFELightManager::GetPackedCount() const noexcept
{
    return m_lastPackedCount;
}

const std::vector<kfe::KFE_LIGHT_DATA_GPU>& kfe::KFELightManager::GetPackedCPUData() const noexcept
{
    return m_cpuPacked;
}

#pragma endregion

#pragma region SRV_Binding

std::uint32_t kfe::KFELightManager::GetSRVDescriptorIndex() const noexcept
{
    return m_srvIndex;
}

const kfe::KFEStructuredBuffer* kfe::KFELightManager::GetStructuredBuffer() const noexcept
{
    return m_structuredBuffer.get();
}

kfe::KFEStructuredBuffer* kfe::KFELightManager::GetStructuredBuffer() noexcept
{
    return m_structuredBuffer.get();
}

const kfe::KFEStagingBuffer* kfe::KFELightManager::GetStagingBuffer() const noexcept
{
    return m_staging.get();
}

kfe::KFEStagingBuffer* kfe::KFELightManager::GetStagingBuffer() noexcept
{
    return m_staging.get();
}

#pragma endregion

#pragma region Resize

_Use_decl_annotations_
bool kfe::KFELightManager::Resize(std::uint32_t newCapacity, const char* newDebugName) noexcept
{
    if (!IsInitialized())
    {
        LOG_ERROR("KFELightManager::Resize: Not initialized.");
        return false;
    }

    if (newCapacity == 0u)
    {
        LOG_ERROR("KFELightManager::Resize: newCapacity must be > 0.");
        return false;
    }

    if (newCapacity == m_capacity)
        return true;

    // Create new resources first
    const char* dbg = (newDebugName && newDebugName[0] != '\0') ? newDebugName : "KFELightManager_Lights_Resized";

    auto newStaging = std::make_unique<KFEStagingBuffer>();
    auto newSB = std::make_unique<KFEStructuredBuffer>();

    {
        KFE_STAGING_BUFFER_CREATE_DESC sdesc{};
        sdesc.Device = m_pDevice;
        sdesc.SizeInBytes = BytesForLights(newCapacity);

        if (!newStaging->Initialize(sdesc))
        {
            LOG_ERROR("Failed to create new staging buffer.");
            return false;
        }
    }

    {
        KFE_STRUCTURED_BUFFER_CREATE_DESC sbDesc{};
        sbDesc.Device = m_pDevice;
        sbDesc.ResourceBuffer = newStaging->GetDefaultBuffer();
        sbDesc.ResourceHeap = m_pHeap;
        sbDesc.ElementStride = static_cast<std::uint32_t>(sizeof(KFE_LIGHT_DATA_GPU));
        sbDesc.ElementCount = newCapacity;
        sbDesc.OffsetInBytes = 0u;

        if (!newSB->Initialize(sbDesc))
        {
            LOG_ERROR("KFELightManager::Resize: Failed to initialize new structured buffer wrapper.");
            (void)newStaging->Destroy();
            return false;
        }

        // Create SRV view for whole buffer
        KFE_STRUCTURED_SRV_DESC srv{};
        srv.FirstElement = 0u;
        srv.NumElements = newCapacity;

        const std::uint32_t srvIdx = newSB->CreateSRV(srv);
        if (srvIdx == KFE_INVALID_INDEX)
        {
            LOG_ERROR("KFELightManager::Resize: Failed to create SRV for new structured buffer.");
            (void)newSB->Destroy();
            (void)newStaging->Destroy();
            return false;
        }


        m_staging = std::move(newStaging);
        m_structuredBuffer = std::move(newSB);

        m_capacity = newCapacity;
        m_srvIndex = srvIdx;
        m_defaultState = D3D12_RESOURCE_STATE_COPY_DEST;
    }

    // Resize CPU packed storage
    m_cpuPacked.resize(m_capacity);
    m_lastPackedCount = 0u;
    MarkDirty();
    LOG_SUCCESS("Resized. Capacity={}, SRVIndex={}.", m_capacity, m_srvIndex);
    return true;
}

#pragma endregion

#pragma region Internal_Helpers

_Use_decl_annotations_
bool kfe::KFELightManager::CreateGPUResources(std::uint32_t capacity, const char* debugName) noexcept
{
    m_staging = std::make_unique<KFEStagingBuffer>();
    m_structuredBuffer = std::make_unique<KFEStructuredBuffer>();

    KFE_STAGING_BUFFER_CREATE_DESC sdesc{};
    sdesc.Device = m_pDevice;
    sdesc.SizeInBytes = BytesForLights(capacity);

    if (!m_staging->Initialize(sdesc))
    {
        LOG_ERROR("Failed to initialize staging buffer.");
        m_staging.reset();
        m_structuredBuffer.reset();
        return false;
    }

    KFE_STRUCTURED_BUFFER_CREATE_DESC sbDesc{};
    sbDesc.Device = m_pDevice;
    sbDesc.ResourceBuffer = m_staging->GetDefaultBuffer();
    sbDesc.ResourceHeap = m_pHeap;
    sbDesc.ElementStride = static_cast<std::uint32_t>(sizeof(KFE_LIGHT_DATA_GPU));
    sbDesc.ElementCount = capacity;
    sbDesc.OffsetInBytes = 0u;

    if (!m_structuredBuffer->Initialize(sbDesc))
    {
        LOG_ERROR("Failed to initialize structured buffer wrapper.");
        (void)m_staging->Destroy();
        m_staging.reset();
        m_structuredBuffer.reset();
        return false;
    }

    m_capacity = capacity;

    // CPU packed allocation
    m_cpuPacked.resize(m_capacity);
    m_lastPackedCount = 0u;

    if (!RecreateSRV())
    {
        LOG_ERROR("Failed to create SRV.");
        (void)m_structuredBuffer->Destroy();
        (void)m_staging->Destroy();
        m_staging.reset();
        m_structuredBuffer.reset();
        m_capacity = 0u;
        return false;
    }

    return true;
}

bool kfe::KFELightManager::RecreateSRV() noexcept
{
    if (!m_structuredBuffer || !m_structuredBuffer->IsInitialized())
    {
        LOG_ERROR("Structured buffer wrapper not initialized.");
        return false;
    }

    KFE_STRUCTURED_SRV_DESC srv{};
    srv.FirstElement = 0u;
    srv.NumElements = m_capacity;

    m_srvIndex = m_structuredBuffer->CreateSRV(srv);
    if (m_srvIndex == KFE_INVALID_INDEX)
    {
        LOG_ERROR("CreateSRV failed.");
        return false;
    }

    return true;
}

void kfe::KFELightManager::RebuildAccessor() noexcept
{
    m_lightAccessor.clear();
    m_lightAccessor.reserve(m_lights.size());

    for (auto& kv : m_lights)
    {
        if (kv.second)
            m_lightAccessor.push_back(kv.second);
    }
}

void kfe::KFELightManager::PackOne(_In_ const IKFELight* src, _Out_ KFE_LIGHT_DATA_GPU& dst) noexcept
{
    std::memset(&dst, 0, sizeof(KFE_LIGHT_DATA_GPU));

    if (!src)
        return;

    dst = src->GetLightData();
}

#pragma endregion
