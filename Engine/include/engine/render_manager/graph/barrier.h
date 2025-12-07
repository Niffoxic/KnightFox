// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  License   : MIT
 *  -----------------------------------------------------------------------------
 */
#pragma once

#include "types.h" // RGResourceAccess, RGTextureAccess, RGBufferAccess, RenderPassDesc, ResourceState

#include <vector>
#include <cstdint>

struct D3D12_RESOURCE_BARRIER;
enum  D3D12_RESOURCE_STATES;

namespace kfe::rg
{
    /// <summary>
    /// Utility to generate D3D12 resource barriers from access patterns across passes
    /// Tracks current state for textures and buffers separately
    /// </summary>
    class BarrierPlanner
    {
    public:
         BarrierPlanner();
        ~BarrierPlanner();

        BarrierPlanner           (const BarrierPlanner&) = delete;
        BarrierPlanner& operator=(const BarrierPlanner&) = delete;

        BarrierPlanner           (BarrierPlanner&&) noexcept;
        BarrierPlanner& operator=(BarrierPlanner&&) noexcept;

        void InitializeTexture(_In_ std::size_t textureCount);
        void InitializeBuffer (_In_ std::size_t bufferCount);

        void SetInitialTextureState(
            _In_ std::size_t textureIndex,
            _In_ D3D12_RESOURCE_STATES state) noexcept;

        void SetInitialBufferState(
            _In_ std::size_t bufferIndex,
            _In_ D3D12_RESOURCE_STATES state) noexcept;

        // Build transition barriers needed for this pass for textures.
        NODISCARD std::vector<D3D12_RESOURCE_BARRIER>
            BuildTexBarriersForPass(const RenderPassDesc& passDesc);

        // Build transition barriers needed for this pass for buffers.
        NODISCARD std::vector<D3D12_RESOURCE_BARRIER>
            BuildBufferBarriersForPass(const RenderPassDesc& passDesc);

    private:
        static D3D12_RESOURCE_STATES
            GetRequiredState(RGResourceAccess access) noexcept;

    private:
        std::vector<ResourceState> m_textureStates;
        std::vector<ResourceState> m_bufferStates;
    };
} // namespace kfe::rg
