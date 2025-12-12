// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  File      : assimp_import_types.h
 *  Purpose   : CPU-side data structures + importer API for KnightFox.
 *  -----------------------------------------------------------------------------
 */
#pragma once

#include "EngineAPI.h"

#include <cstdint>
#include <string>
#include <vector>

#include <DirectXMath.h>

namespace kfe::import
{
    inline constexpr std::uint32_t KFE_MAX_UV_CHANNELS = 2u;

    using Float2   = DirectX::XMFLOAT2;   //~ UVs
    using Float3   = DirectX::XMFLOAT3;   //~ Positions, normals, tangents
    using Float4x4 = DirectX::XMFLOAT4X4; //~ Node transforms

    struct ImportedVertex
    {
        Float3 Position{};
        Float3 Normal{};
        Float3 Tangent{};
        Float3 Bitangent{};

        Float2 UV[KFE_MAX_UV_CHANNELS]{};

        bool HasNormal = false;
        bool HasTangent = false;
        bool HasUV[KFE_MAX_UV_CHANNELS] = { false, false };
    };

    struct ImportedMesh
    {
        std::string                 Name;
        std::vector<ImportedVertex> Vertices;
        std::vector<std::uint32_t>  Indices;

        std::uint32_t MaterialIndex = 0u;

        Float3 AABBMin{ 1e30f,  1e30f,  1e30f };
        Float3 AABBMax{ -1e30f, -1e30f, -1e30f };
    };

    struct ImportedMaterialStub
    {
        std::string   Name;
        std::uint32_t OriginalIndex = 0u;
    };

    struct ImportedNode
    {
        std::string                Name;
        Float4x4                   LocalTransform{};
        std::vector<std::uint32_t> MeshIndices;
        std::vector<ImportedNode>  Children;

        bool HasMeshes()   const noexcept { return !MeshIndices.empty(); }
        bool HasChildren() const noexcept { return !Children.empty(); }
    };

    struct ImportedScene
    {
        std::vector<ImportedMesh>         Meshes;
        std::vector<ImportedMaterialStub> Materials;
        ImportedNode                      RootNode;

        bool IsValid() const noexcept
        {
            return !Meshes.empty() && !RootNode.Name.empty();
        }

        void Clear() noexcept
        {
            Meshes.clear();
            Materials.clear();
            RootNode = ImportedNode{};
        }
    };

    class KFE_API AssimpImporter
    {
    public:
        AssimpImporter();
        ~AssimpImporter();

        AssimpImporter(const AssimpImporter&) = delete;
        AssimpImporter& operator=(const AssimpImporter&) = delete;
        AssimpImporter(AssimpImporter&&) noexcept = delete;
        AssimpImporter& operator=(AssimpImporter&&) noexcept = delete;

        bool LoadFromFile(const std::string& filePath,
            ImportedScene& outScene,
            std::string& outErrorMessage) noexcept;
    };

    // Debug helpers
    void DebugPrintSceneTree(const ImportedScene& scene) noexcept;
    void DebugPrintNodeRecursive(const ImportedNode& node,
        const ImportedScene& scene,
        std::uint32_t depth = 0u) noexcept;

} // namespace kfe::import
