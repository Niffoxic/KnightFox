// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  File      : engine_conventions.h
 *  -----------------------------------------------------------------------------
 */
#pragma once

#include <cstdint>

namespace kfe
{
    namespace engine_conventions
    {
        inline constexpr bool WORLD_LEFT_HANDED = true;

        inline constexpr float WORLD_UP[3]      = { 0.0f, 1.0f, 0.0f };
        inline constexpr float WORLD_FORWARD[3] = { 0.0f, 0.0f, 1.0f };
        inline constexpr float WORLD_RIGHT[3]   = { 1.0f, 0.0f, 0.0f };
    }

    //~ Later after the project ended add support for opengl
    enum class NormalMapConvention : std::uint8_t
    {
        DirectX_PlusY,
    };

    inline constexpr NormalMapConvention kEngineNormalMapConvention =
        NormalMapConvention::DirectX_PlusY;

    enum class TextureColorSpace : std::uint8_t
    {
        Linear = 0,
        SRGB = 1
    };

    // Semantic material texture slots
    enum class MaterialTextureSlot : std::uint8_t
    {
        BaseColor = 0,  //~ for Albedo or diffuse and sRGB
        Normal,         //~ Tangent space normal and linear

        //~ Packed Occlusion and Roughness and Metallic texture
        ORM,

        Emissive,       //~ Emissive color, sRGB
        Opacity,        //~ Opacity or mask, linear
        Height,         //~ Height or parallax or displacement, linear

        Count
    };

    inline constexpr bool IsColorTextureSlot(MaterialTextureSlot slot) noexcept
    {
        switch (slot)
        {
        case MaterialTextureSlot::BaseColor:
        case MaterialTextureSlot::Emissive:
            return true;

        default:
            return false;
        }
    }

    inline constexpr TextureColorSpace GetColorSpaceForSlot(MaterialTextureSlot slot) noexcept
    {
        return IsColorTextureSlot(slot)
            ? TextureColorSpace::SRGB
            : TextureColorSpace::Linear;
    }

    inline constexpr bool IsPackedORM(MaterialTextureSlot slot) noexcept
    {
        return slot == MaterialTextureSlot::ORM;
    }

    struct ImportSpaceDescription
    {
        bool  LeftHanded;
        float Up[3];
        float Forward[3];
        float Right[3];
    };

    inline constexpr ImportSpaceDescription kEngineImportSpace
    {
        engine_conventions::WORLD_LEFT_HANDED,
        { engine_conventions::WORLD_UP[0],      engine_conventions::WORLD_UP[1],      engine_conventions::WORLD_UP[2]      },
        { engine_conventions::WORLD_FORWARD[0], engine_conventions::WORLD_FORWARD[1], engine_conventions::WORLD_FORWARD[2] },
        { engine_conventions::WORLD_RIGHT[0],   engine_conventions::WORLD_RIGHT[1],   engine_conventions::WORLD_RIGHT[2]   }
    };
}
