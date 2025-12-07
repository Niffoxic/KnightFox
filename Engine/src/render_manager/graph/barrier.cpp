// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  License   : MIT
 *  -----------------------------------------------------------------------------
 */

#include "pch.h"
#include "engine/render_manager/graph/barrier.h"
#include <d3d12.h>

namespace
{
    static D3D12_RESOURCE_STATES GetRequiredBufferState(kfe::rg::RGResourceAccess access) noexcept
    {
        switch (access)
        {
        case kfe::rg::RGResourceAccess::Read:
            return D3D12_RESOURCE_STATE_GENERIC_READ;

        case kfe::rg::RGResourceAccess::Write:
        case kfe::rg::RGResourceAccess::ReadWrite:
            return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

        default:
            return D3D12_RESOURCE_STATE_COMMON;
        }
    }
} // namespace


kfe::rg::BarrierPlanner::BarrierPlanner() = default;
kfe::rg::BarrierPlanner::~BarrierPlanner() = default;

kfe::rg::BarrierPlanner::BarrierPlanner(BarrierPlanner&& other) noexcept = default;
kfe::rg::BarrierPlanner& kfe::rg::BarrierPlanner::operator=(BarrierPlanner&& other) noexcept = default;


void kfe::rg::BarrierPlanner::InitializeTexture(std::size_t textureCount)
{
    m_textureStates.clear();
    m_textureStates.resize(textureCount);
}

void kfe::rg::BarrierPlanner::InitializeBuffer(std::size_t bufferCount)
{
    m_bufferStates.clear();
    m_bufferStates.resize(bufferCount);
}

void kfe::rg::BarrierPlanner::SetInitialTextureState(std::size_t textureIndex, D3D12_RESOURCE_STATES state) noexcept
{
    if (textureIndex >= m_textureStates.size())
    {
        return;
    }

    m_textureStates[textureIndex].Set(state);
}

void kfe::rg::BarrierPlanner::SetInitialBufferState(std::size_t bufferIndex, D3D12_RESOURCE_STATES state) noexcept
{
    if (bufferIndex >= m_bufferStates.size())
    {
        return;
    }

    m_bufferStates[bufferIndex].Set(state);
}

std::vector<D3D12_RESOURCE_BARRIER> kfe::rg::BarrierPlanner::BuildTexBarriersForPass(const RenderPassDesc& passDesc)
{
    std::vector<D3D12_RESOURCE_BARRIER> barriers;

    struct TexAgg
    {
        std::uint32_t Index;
        bool          HasRead{ false };
        bool          HasWrite{ false };
    };

    std::vector<TexAgg> aggregated;

    auto find_or_add = [&aggregated](std::uint32_t index) -> TexAgg&
        {
            for (auto& a : aggregated)
            {
                if (a.Index == index)
                    return a;
            }
            aggregated.push_back(TexAgg{ index });
            return aggregated.back();
        };

    for (const auto& texIn : passDesc.TextureInputs)
    {
        if (!texIn.IsValid())
            continue;

        const auto index = texIn.Handle.Index;
        auto& agg = find_or_add(index);

        if (IsReadAccess(texIn.Access))
            agg.HasRead = true;
        if (IsWriteAccess(texIn.Access))
            agg.HasWrite = true;
    }

    for (const auto& texOut : passDesc.TextureOutputs)
    {
        if (!texOut.IsValid())
            continue;

        const auto index = texOut.Handle.Index;
        auto& agg = find_or_add(index);

        if (IsReadAccess(texOut.Access))
            agg.HasRead = true;
        if (IsWriteAccess(texOut.Access))
            agg.HasWrite = true;
    }

    for (const auto& agg : aggregated)
    {
        const auto index = agg.Index;
        if (index >= m_textureStates.size())
        {
            continue;
        }

        RGResourceAccess effectiveAccess = RGResourceAccess::Read;
        if (agg.HasRead && agg.HasWrite)
            effectiveAccess = RGResourceAccess::ReadWrite;
        else if (agg.HasWrite)
            effectiveAccess = RGResourceAccess::Write;
        else
            effectiveAccess = RGResourceAccess::Read;

        const auto requiredState = GetRequiredState(effectiveAccess);
        const auto currentState = m_textureStates[index].Get();

        if (requiredState == currentState)
        {
            continue;
        }

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

        barrier.Transition.pResource   = nullptr;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = currentState;
        barrier.Transition.StateAfter  = requiredState;

        barriers.push_back(barrier);

        m_textureStates[index].Set(requiredState);
    }

    return barriers;
}

std::vector<D3D12_RESOURCE_BARRIER> kfe::rg::BarrierPlanner::BuildBufferBarriersForPass(const RenderPassDesc& passDesc)
{
    std::vector<D3D12_RESOURCE_BARRIER> barriers;

    struct BufAgg
    {
        std::uint32_t Index;
        bool          HasRead{ false };
        bool          HasWrite{ false };
    };

    std::vector<BufAgg> aggregated;

    auto find_or_add = [&aggregated](std::uint32_t index) -> BufAgg&
        {
            for (auto& a : aggregated)
            {
                if (a.Index == index)
                    return a;
            }
            aggregated.push_back(BufAgg{ index });
            return aggregated.back();
        };

    for (const auto& bufIn : passDesc.BufferInputs)
    {
        if (!bufIn.IsValid())
            continue;

        const auto index = bufIn.Handle.Index;
        auto& agg = find_or_add(index);

        if (IsReadAccess(bufIn.Access))
            agg.HasRead = true;
        if (IsWriteAccess(bufIn.Access))
            agg.HasWrite = true;
    }

    for (const auto& bufOut : passDesc.BufferOutputs)
    {
        if (!bufOut.IsValid())
            continue;

        const auto index = bufOut.Handle.Index;
        auto& agg = find_or_add(index);

        if (IsReadAccess(bufOut.Access))
            agg.HasRead = true;
        if (IsWriteAccess(bufOut.Access))
            agg.HasWrite = true;
    }

    for (const auto& agg : aggregated)
    {
        const auto index = agg.Index;
        if (index >= m_bufferStates.size())
        {
            continue;
        }

        RGResourceAccess effectiveAccess = RGResourceAccess::Read;
        if (agg.HasRead && agg.HasWrite)
            effectiveAccess = RGResourceAccess::ReadWrite;
        else if (agg.HasWrite)
            effectiveAccess = RGResourceAccess::Write;
        else
            effectiveAccess = RGResourceAccess::Read;

        const auto requiredState = GetRequiredBufferState(effectiveAccess);
        const auto currentState = m_bufferStates[index].Get();

        if (requiredState == currentState)
        {
            continue;
        }

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

        barrier.Transition.pResource   = nullptr; // TODO: patched later
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = currentState;
        barrier.Transition.StateAfter  = requiredState;

        barriers.push_back(barrier);

        m_bufferStates[index].Set(requiredState);
    }

    return barriers;
}

D3D12_RESOURCE_STATES kfe::rg::BarrierPlanner::GetRequiredState(RGResourceAccess access) noexcept
{
    switch (access)
    {
    case RGResourceAccess::Read:
        return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    case RGResourceAccess::Write:
        return D3D12_RESOURCE_STATE_RENDER_TARGET;

    case RGResourceAccess::ReadWrite:
        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    default:
        return D3D12_RESOURCE_STATE_COMMON;
    }
}
