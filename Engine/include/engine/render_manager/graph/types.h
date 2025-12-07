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

    NODISCARD inline const char* ToString(RGTextureUsage u) noexcept
    {
        switch (u)
        {
        case RGTextureUsage::Objects:      return "Objects";
        case RGTextureUsage::Depth:        return "Depth";
        case RGTextureUsage::Lighting:     return "Lighting";
        case RGTextureUsage::PostProcess:  return "PostProcess";
        case RGTextureUsage::UI:           return "UI";
        case RGTextureUsage::Present:      return "Present";
        default:                           return "Unknown";
        }
    }

    enum class RGBufferUsage : std::uint8_t
    {
        Unknown = 0,

        Vertex,
        Index,
        Constant,
        Structured,
    };

    NODISCARD inline const char* ToString(RGBufferUsage u) noexcept
    {
        switch (u)
        {
        case RGBufferUsage::Vertex:        return "Vertex";
        case RGBufferUsage::Index:         return "Index";
        case RGBufferUsage::Constant:      return "Constant";
        case RGBufferUsage::Structured:    return "Structured";
        default:                           return "Unknown";
        }
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
