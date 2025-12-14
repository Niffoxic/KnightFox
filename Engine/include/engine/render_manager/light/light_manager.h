// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  File      : light_manager.h
 *  -----------------------------------------------------------------------------
 */
#pragma once

#include "EngineAPI.h"
#include "engine/core.h"

#include "engine/system/interface/interface_light.h"

#include "engine/render_manager/api/components/device.h"
#include "engine/render_manager/api/buffer/staging_buffer.h"
#include "engine/render_manager/api/buffer/buffer.h"
#include "engine/render_manager/api/buffer/structured_buffer.h"
#include "engine/render_manager/api/heap/heap_cbv_srv_uav.h"
#include "engine/system/common_types.h"

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

struct ID3D12GraphicsCommandList;

namespace kfe
{
    typedef struct _KFE_CREATE_LIGHT_MANAGER
    {
        KFEDevice*       Device     = nullptr;
        KFEResourceHeap* Heap       = nullptr;
        std::uint32_t    Capacity   = 64u;
        const char* DebugName = "KFELightManager_Lights";
    } KFE_CREATE_LIGHT_MANAGER;

    /// <summary>
    /// Owns All Light data attached one 1 object
    /// Pack and prepares data for pixel buffer
    /// 'D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE'
    /// </summary>
    class KFE_API KFELightManager final : public IKFEObject
    {
    public:
         KFELightManager() noexcept;
        ~KFELightManager() noexcept;

        KFELightManager(const KFELightManager&) = delete;
        KFELightManager& operator=(const KFELightManager&) = delete;

        KFELightManager(KFELightManager&&) noexcept;
        KFELightManager& operator=(KFELightManager&&) noexcept;

        // IKFEObject
        NODISCARD std::string GetName       () const noexcept override;
        NODISCARD std::string GetDescription() const noexcept override;

        // Lifetime
        NODISCARD bool Initialize(_In_ const KFE_CREATE_LIGHT_MANAGER& desc) noexcept;
        NODISCARD bool Destroy() noexcept;
        NODISCARD bool IsInitialized() const noexcept;

        // Light collection
        void AttachLight(_In_ IKFELight* light);
        void DetachLight(_In_ IKFELight* light);
        void DetachLight(_In_ KID lightId);

        void SetDrawState(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES before);

        void ClearLights() noexcept;

        NODISCARD bool Contains(_In_ KID lightId) const noexcept;

        // Active attached light count
        NODISCARD std::uint32_t GetLightCounts() const noexcept;

        // Max lights that can be packed and uploaded
        NODISCARD std::uint32_t GetCapacity() const noexcept;

        // snapshot
        NODISCARD const std::vector<IKFELight*>& GetAllLights() const noexcept;

        void MarkDirty() noexcept;

        // Packs all attached lights into contiguous KFE_LIGHT_DATA_DESC array
        void PackData() noexcept;

        NODISCARD bool RecordUpload(_In_ ID3D12GraphicsCommandList* cmdList) noexcept;

        // PackData if dirty then RecordUpload
        NODISCARD bool UpdateAndRecord(_In_ ID3D12GraphicsCommandList* cmdList) noexcept;

        // How many lights were packed last time <= Capacity
        NODISCARD std::uint32_t GetPackedCount() const noexcept;

        // Direct access to packed CPU data
        NODISCARD const std::vector<KFE_LIGHT_DATA_DESC>& GetPackedCPUData() const noexcept;

        // Descriptor index for the structured SRV
        NODISCARD std::uint32_t GetSRVDescriptorIndex() const noexcept;

        // View wrapper
        NODISCARD const KFEStructuredBuffer* GetStructuredBuffer() const noexcept;
        NODISCARD       KFEStructuredBuffer* GetStructuredBuffer()       noexcept;

        // Underlying staging pair for barriers and state tracking
        NODISCARD const KFEStagingBuffer* GetStagingBuffer() const noexcept;
        NODISCARD       KFEStagingBuffer* GetStagingBuffer()       noexcept;

        // Reallocs GPU buffers and recreates SRV
        NODISCARD bool Resize(_In_ std::uint32_t newCapacity, _In_opt_ const char* newDebugName = nullptr) noexcept;

    private:
        NODISCARD bool CreateGPUResources(_In_ std::uint32_t capacity, _In_ const char* debugName) noexcept;
        NODISCARD bool RecreateSRV() noexcept;

        void RebuildAccessor() noexcept;

        static void PackOne(_In_ const IKFELight* src, _Out_ KFE_LIGHT_DATA_DESC& dst) noexcept;

    private:
        // Init params
        KFEDevice*       m_pDevice{ nullptr };
        KFEResourceHeap* m_pHeap{ nullptr };

        std::uint32_t    m_capacity{ 0u };
        std::uint32_t    m_srvIndex{ KFE_INVALID_INDEX };
        bool             m_bInitialized{ false };

        // CPU light registry
        std::unordered_map<KID, IKFELight*> m_lights;
        std::vector<IKFELight*>             m_lightAccessor;

        // CPU packed data
        std::vector<KFE_LIGHT_DATA_DESC>    m_cpuPacked;
        std::uint32_t                       m_lastPackedCount{ 0u };
        bool                                m_bDirty{ true };

        // GPU resources
        std::unique_ptr<KFEStagingBuffer>    m_staging;
        std::unique_ptr<KFEStructuredBuffer> m_structuredBuffer;
        D3D12_RESOURCE_STATES m_defaultState{ D3D12_RESOURCE_STATE_COPY_DEST };
    };

} // namespace kfe
