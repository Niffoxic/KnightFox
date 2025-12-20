// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  File      : assimp_importer.cpp
 *  -----------------------------------------------------------------------------
 */

#include "pch.h"

#include "engine/render_manager/assets_library/model/assimp_importer.h"
#include "engine/utils/logger.h"

#include <algorithm>
#include <filesystem>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace kfe::import
{
    static Float4x4 ConvertMatrix(const aiMatrix4x4& src)
    {
        Float4x4 dst{};
        dst.m[0][0] = src.a1; dst.m[0][1] = src.a2; dst.m[0][2] = src.a3; dst.m[0][3] = src.a4;
        dst.m[1][0] = src.b1; dst.m[1][1] = src.b2; dst.m[1][2] = src.b3; dst.m[1][3] = src.b4;
        dst.m[2][0] = src.c1; dst.m[2][1] = src.c2; dst.m[2][2] = src.c3; dst.m[2][3] = src.c4;
        dst.m[3][0] = src.d1; dst.m[3][1] = src.d2; dst.m[3][2] = src.d3; dst.m[3][3] = src.d4;
        return dst;
    }

    static void NormalizeSceneToUnitBox(kfe::import::ImportedScene& scene)
    {
        using kfe::import::Float3;

        Float3 globalMin{ 1e30f,  1e30f,  1e30f };
        Float3 globalMax{ -1e30f, -1e30f, -1e30f };

        for (auto& mesh : scene.Meshes)
        {
            globalMin.x = std::min(globalMin.x, mesh.AABBMin.x);
            globalMin.y = std::min(globalMin.y, mesh.AABBMin.y);
            globalMin.z = std::min(globalMin.z, mesh.AABBMin.z);

            globalMax.x = std::max(globalMax.x, mesh.AABBMax.x);
            globalMax.y = std::max(globalMax.y, mesh.AABBMax.y);
            globalMax.z = std::max(globalMax.z, mesh.AABBMax.z);
        }

        Float3 center{
            0.5f * (globalMin.x + globalMax.x),
            0.5f * (globalMin.y + globalMax.y),
            0.5f * (globalMin.z + globalMax.z)
        };

        Float3 extent{
            globalMax.x - globalMin.x,
            globalMax.y - globalMin.y,
            globalMax.z - globalMin.z
        };

        const float maxExtent = std::max(extent.x, std::max(extent.y, extent.z));
        if (maxExtent <= 1e-6f)
            return;

        const float invScale = 1.0f / maxExtent;

        for (auto& mesh : scene.Meshes)
        {
            mesh.AABBMin = { 1e30f,  1e30f,  1e30f };
            mesh.AABBMax = { -1e30f, -1e30f, -1e30f };

            for (auto& v : mesh.Vertices)
            {
                auto p = v.Position;
                p.x = (p.x - center.x) * invScale;
                p.y = (p.y - center.y) * invScale;
                p.z = (p.z - center.z) * invScale;
                v.Position = p;

                mesh.AABBMin.x = std::min(mesh.AABBMin.x, p.x);
                mesh.AABBMin.y = std::min(mesh.AABBMin.y, p.y);
                mesh.AABBMin.z = std::min(mesh.AABBMin.z, p.z);

                mesh.AABBMax.x = std::max(mesh.AABBMax.x, p.x);
                mesh.AABBMax.y = std::max(mesh.AABBMax.y, p.y);
                mesh.AABBMax.z = std::max(mesh.AABBMax.z, p.z);
            }
        }
    }

    static void ConvertNodeRecursive(const aiScene* scene,
        const aiNode* src,
        ImportedNode& dst)
    {
        (void)scene;

        dst.Name = src->mName.C_Str();
        dst.LocalTransform = ConvertMatrix(src->mTransformation);

        dst.MeshIndices.clear();
        dst.MeshIndices.reserve(src->mNumMeshes);

        for (unsigned int i = 0; i < src->mNumMeshes; ++i)
            dst.MeshIndices.push_back(static_cast<std::uint32_t>(src->mMeshes[i]));

        dst.Children.resize(src->mNumChildren);

        for (unsigned int c = 0; c < src->mNumChildren; ++c)
            ConvertNodeRecursive(scene, src->mChildren[c], dst.Children[c]);
    }

    AssimpImporter::AssimpImporter() = default;
    AssimpImporter::~AssimpImporter() = default;

    bool AssimpImporter::LoadFromFile(const std::string& filePath,
        ImportedScene& outScene,
        std::string& outErrorMessage) noexcept
    {
        outScene.Clear();
        outErrorMessage.clear();

        if (!std::filesystem::exists(filePath))
        {
            outErrorMessage = "File does not exist: " + filePath;
            LOG_ERROR("AssimpImporter: {}", outErrorMessage);
            return false;
        }

        LOG_INFO("AssimpImporter: Loading scene '{}'", filePath);

        Assimp::Importer importer;

        const unsigned int flags =
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_ConvertToLeftHanded |
            aiProcess_FlipUVs |
            aiProcess_FlipWindingOrder;

        const aiScene* scene = importer.ReadFile(filePath, flags);

        if (!scene)
        {
            outErrorMessage = importer.GetErrorString();
            LOG_ERROR("AssimpImporter: Failed to load '{}': {}", filePath, outErrorMessage);
            return false;
        }

        if (!scene->mRootNode)
        {
            outErrorMessage = "Scene missing root node";
            LOG_ERROR("AssimpImporter: {}", outErrorMessage);
            return false;
        }

        //~ Meshes
        outScene.Meshes.reserve(scene->mNumMeshes);

        for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
        {
            const aiMesh* srcMesh = scene->mMeshes[meshIndex];
            if (!srcMesh)
                continue;

            ImportedMesh mesh{};
            mesh.Name = srcMesh->mName.C_Str();

            const bool hasNormals = srcMesh->HasNormals();
            const bool hasTangents = srcMesh->HasTangentsAndBitangents();
            bool hasUV[KFE_MAX_UV_CHANNELS] = { false, false };

            for (std::uint32_t ch = 0; ch < KFE_MAX_UV_CHANNELS; ++ch)
                hasUV[ch] = srcMesh->HasTextureCoords(ch);

            mesh.Vertices.resize(srcMesh->mNumVertices);

            for (unsigned int v = 0; v < srcMesh->mNumVertices; ++v)
            {
                ImportedVertex& outV = mesh.Vertices[v];

                const aiVector3D& p = srcMesh->mVertices[v];
                outV.Position = { p.x, p.y, p.z };

                mesh.AABBMin.x = std::min(mesh.AABBMin.x, outV.Position.x);
                mesh.AABBMin.y = std::min(mesh.AABBMin.y, outV.Position.y);
                mesh.AABBMin.z = std::min(mesh.AABBMin.z, outV.Position.z);

                mesh.AABBMax.x = std::max(mesh.AABBMax.x, outV.Position.x);
                mesh.AABBMax.y = std::max(mesh.AABBMax.y, outV.Position.y);
                mesh.AABBMax.z = std::max(mesh.AABBMax.z, outV.Position.z);

                if (hasNormals)
                {
                    const aiVector3D& n = srcMesh->mNormals[v];
                    outV.Normal = { n.x, n.y, n.z };
                    outV.HasNormal = true;
                }
                else
                {
                    outV.HasNormal = false;
                }

                if (hasTangents)
                {
                    outV.HasTangent = true;

                    const aiVector3D& t = srcMesh->mTangents[v];
                    const aiVector3D& bt = srcMesh->mBitangents[v];

                    outV.Tangent = { t.x,  t.y,  t.z };
                    outV.Bitangent = { bt.x, bt.y, bt.z };
                }
                else
                {
                    outV.HasTangent = false;
                }

                for (std::uint32_t ch = 0; ch < KFE_MAX_UV_CHANNELS; ++ch)
                {
                    if (hasUV[ch])
                    {
                        const aiVector3D& uv = srcMesh->mTextureCoords[ch][v];
                        outV.UV[ch] = { uv.x, uv.y };
                        outV.HasUV[ch] = true;
                    }
                    else
                    {
                        outV.HasUV[ch] = false;
                    }
                }
            }

            mesh.Indices.reserve(srcMesh->mNumFaces * 3u);

            for (unsigned int f = 0; f < srcMesh->mNumFaces; ++f)
            {
                const aiFace& face = srcMesh->mFaces[f];
                if (face.mNumIndices != 3u)
                {
                    LOG_WARNING("AssimpImporter: Non triangle face in mesh '{}'", mesh.Name);
                    continue;
                }

                mesh.Indices.push_back(face.mIndices[0]);
                mesh.Indices.push_back(face.mIndices[1]);
                mesh.Indices.push_back(face.mIndices[2]);
            }

            outScene.Meshes.emplace_back(std::move(mesh));
        }

        NormalizeSceneToUnitBox(outScene);

        //~ Node hierarchy
        ConvertNodeRecursive(scene, scene->mRootNode, outScene.RootNode);

        LOG_INFO("AssimpImporter: Imported successfully (Meshes={})",
            outScene.Meshes.size());

        return true;
    }

} // namespace kfe::import
