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

#include "engine/system/common_types.h"
#include <dxgiformat.h>

namespace kfe::rg
{
    //~ Tags for textures thats going to be in render graph.
    //~ CPU side only
    enum class RGTextureUsage : std::uint8_t
    {
        Unknown = 0,

        Objects,
        Depth,
        Lighting,
        PostProcess,
        UI,
        Present
    };

    enum class RGBufferUsage : std::uint8_t
    {
        Unknown = 0,

        Vertex,
        Index,
        Constant,
        Structured,
    };

    enum class RGResourceAccess : std::uint8_t
    {
        Read = 0,
        Write,
        ReadWrite
    };

    NODISCARD constexpr bool IsReadAccess(RGResourceAccess access) noexcept
    {
        return  access == RGResourceAccess::Read ||
                access == RGResourceAccess::ReadWrite;
    }

    NODISCARD constexpr bool IsWriteAccess(RGResourceAccess access) noexcept
    {
        return  access == RGResourceAccess::Write ||
                access == RGResourceAccess::ReadWrite;
    }

    struct RGTextureHandle
    {
        std::uint32_t Index{ KFE_INVALID_INDEX };

        NODISCARD constexpr bool IsValid() const noexcept
        {
            return Index != KFE_INVALID_INDEX;
        }

        NODISCARD static constexpr RGTextureHandle Invalid() noexcept
        {
            return { KFE_INVALID_INDEX };
        }

        NODISCARD friend constexpr bool operator==(
            RGTextureHandle left,
            RGTextureHandle right) noexcept
        {
            return left.Index == right.Index;
        }

        NODISCARD friend constexpr bool operator!=(
            RGTextureHandle left,
            RGTextureHandle right) noexcept
        {
            return left.Index != right.Index;
        }
    };

    struct RGBufferHandle
    {
        std::uint32_t Index{ KFE_INVALID_INDEX };

        NODISCARD constexpr bool IsValid() const noexcept
        {
            return Index != KFE_INVALID_INDEX;
        }

        NODISCARD static constexpr RGBufferHandle Invalid() noexcept
        {
            return { KFE_INVALID_INDEX };
        }

        NODISCARD friend constexpr bool operator==(
            RGBufferHandle left,
            RGBufferHandle right) noexcept
        {
            return left.Index == right.Index;
        }

        NODISCARD friend constexpr bool operator!=(
            RGBufferHandle left,
            RGBufferHandle right) noexcept
        {
            return left.Index != right.Index;
        }
    };

    //~ Component Description

    struct RGTextureDesc
    {
        std::uint32_t   Width       { 0u };
        std::uint32_t   Height      { 0u };
        DXGI_FORMAT     Format      { DXGI_FORMAT_UNKNOWN };
        std::uint32_t   MipLevels   { 1u };
        std::uint32_t   SampleCount { 1u };
        RGTextureUsage  Usage       { RGTextureUsage::Unknown };
        bool            IsHistory   { false };  // for TAA

        NODISCARD constexpr bool IsValid() const noexcept
        {
            return  Width  > 0 &&
                    Height > 0 &&
                    Format != DXGI_FORMAT_UNKNOWN;
        }
    };

    struct RGBufferDesc
    {
        std::uint32_t SizeInBytes{ 0u };
        RGBufferUsage Usage      { RGBufferUsage::Unknown };

        NODISCARD constexpr bool IsValid() const noexcept
        {
            return SizeInBytes > 0;
        }
    };

    struct RGTextureAccess
    {
        RGTextureHandle  Handle{};
        RGResourceAccess Access{ RGResourceAccess::Read };

        NODISCARD constexpr bool IsValid() const noexcept
        {
            return Handle.IsValid();
        }
    };

    struct RGBufferAccess
    {
        RGBufferHandle   Handle{};
        RGResourceAccess Access{ RGResourceAccess::Read };

        NODISCARD constexpr bool IsValid() const noexcept
        {
            return Handle.IsValid();
        }
    };

    struct RenderPassDesc
    {
        std::string                   Name;
        std::vector<RGTextureAccess>  TextureInputs; //~ textures
        std::vector<RGTextureAccess>  TextureOutputs;
        std::vector<RGBufferAccess>   BufferInputs; //~ buffers
        std::vector<RGBufferAccess>   BufferOutputs;

        NODISCARD bool IsValid() const noexcept
        {
            return !Name.empty();
        }

        void Clear() noexcept
        {
            Name            .clear();
            TextureInputs   .clear();
            TextureOutputs  .clear();
            BufferInputs    .clear();
            BufferOutputs   .clear();
        }
    };

    struct PassNode
    {
        std::uint32_t PassIndex{ KFE_INVALID_INDEX };
        std::vector<uint32_t> Dependencies;

        NODISCARD constexpr bool IsValid() const noexcept
        {
            return PassIndex != KFE_INVALID_INDEX;
        }

        void AddDependency(std::uint32_t index) noexcept
        {
            if (index == KFE_INVALID_INDEX)
            {
                return;
            }
            Dependencies.push_back(index);
        }

        void Clear() noexcept
        {
            PassIndex = KFE_INVALID_INDEX;
            Dependencies.clear();
        }
    };

    struct ResourceLifetime
    {
        std::uint32_t FirstUsePass{ KFE_INVALID_INDEX };
        std::uint32_t LastUsePass{ 0u };

        NODISCARD constexpr bool IsValid() const noexcept
        {
            return FirstUsePass != KFE_INVALID_INDEX;
        }

        void Reset() noexcept
        {
            FirstUsePass = KFE_INVALID_INDEX;
            LastUsePass  = 0u;
        }

        void RegisterUse(std::uint32_t passIndex) noexcept
        {
            if (!IsValid())
            {
                FirstUsePass = passIndex;
                LastUsePass  = passIndex;
                return;
            }

            if (passIndex < FirstUsePass)
            {
                FirstUsePass = passIndex;
            }
            if (passIndex > LastUsePass)
            {
                LastUsePass = passIndex;
            }
        }
    };

    enum ::D3D12_RESOURCE_STATES;
    struct ResourceState
    {
        D3D12_RESOURCE_STATES CurrentState;

        constexpr          ResourceState() noexcept;
        constexpr explicit ResourceState(D3D12_RESOURCE_STATES initial) noexcept;
        
        NODISCARD constexpr D3D12_RESOURCE_STATES Get() const noexcept;
                  constexpr void Set(D3D12_RESOURCE_STATES newState) noexcept;
    };

} // namespace kfe::rg

//~ hashing 

// Texture
template<>
struct std::hash<kfe::rg::RGTextureHandle>
{
    std::size_t operator()(const kfe::rg::RGTextureHandle& h) const noexcept
    {
        return std::hash<std::uint32_t>{}(h.Index);
    }
};

// Buffer
template<>
struct std::hash<kfe::rg::RGBufferHandle>
{
    std::size_t operator()(const kfe::rg::RGBufferHandle& h) const noexcept
    {
        return std::hash<std::uint32_t>{}(h.Index);
    }
};
