#include "pch.h"

#include "engine/render_manager/assets_library/model/assimp_importer.h"
#include "engine/utils/logger.h"

#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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

    static void ConvertNodeRecursive(const aiScene* scene,
        const aiNode* src,
        ImportedNode& dst)
    {
        dst.Name = src->mName.C_Str();
        dst.LocalTransform = ConvertMatrix(src->mTransformation);

        dst.MeshIndices.clear();
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
            aiProcess_SortByPType |
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace;

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

        outScene.Meshes.reserve(scene->mNumMeshes);

        for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
        {
            const aiMesh* srcMesh = scene->mMeshes[meshIndex];
            if (!srcMesh)
                continue;

            ImportedMesh mesh{};
            mesh.Name = srcMesh->mName.C_Str();
            mesh.MaterialIndex = static_cast<std::uint32_t>(srcMesh->mMaterialIndex);

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

                if (hasTangents)
                {
                    outV.HasTangent = true;

                    const aiVector3D& t = srcMesh->mTangents[v];
                    const aiVector3D& bt = srcMesh->mBitangents[v];

                    outV.Tangent = { t.x,  t.y,  t.z };
                    outV.Bitangent = { bt.x, bt.y, bt.z };
                }

                for (std::uint32_t ch = 0; ch < KFE_MAX_UV_CHANNELS; ++ch)
                {
                    if (hasUV[ch])
                    {
                        const aiVector3D& uv = srcMesh->mTextureCoords[ch][v];
                        outV.UV[ch] = { uv.x, uv.y };
                        outV.HasUV[ch] = true;
                    }
                }
            }

            mesh.Indices.reserve(srcMesh->mNumFaces * 3u);

            for (unsigned int f = 0; f < srcMesh->mNumFaces; ++f)
            {
                const aiFace& face = srcMesh->mFaces[f];
                if (face.mNumIndices != 3u)
                {
                    LOG_WARNING("AssimpImporter: Non-triangle face in mesh '{}'", mesh.Name);
                    continue;
                }

                mesh.Indices.push_back(face.mIndices[0]);
                mesh.Indices.push_back(face.mIndices[1]);
                mesh.Indices.push_back(face.mIndices[2]);
            }

            outScene.Meshes.emplace_back(std::move(mesh));
        }

        outScene.Materials.reserve(scene->mNumMaterials);

        for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
        {
            ImportedMaterialStub stub{};
            stub.OriginalIndex = i;

            aiString name;
            if (scene->mMaterials[i]->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
                stub.Name = name.C_Str();
            else
                stub.Name = "Material_" + std::to_string(i);

            outScene.Materials.emplace_back(std::move(stub));
        }

        ConvertNodeRecursive(scene, scene->mRootNode, outScene.RootNode);

        LOG_INFO("AssimpImporter: Imported successfully (Meshes={}, Materials={})",
            outScene.Meshes.size(),
            outScene.Materials.size());

        return true;
    }

    void DebugPrintNodeRecursive(const ImportedNode& node,
        const ImportedScene& scene,
        std::uint32_t depth) noexcept
    {
        std::string indent(depth * 2, ' ');

        LOG_INFO("{}Node '{}' (meshes={})",
            indent, node.Name, node.MeshIndices.size());

        for (std::size_t i = 0; i < node.MeshIndices.size(); ++i)
        {
            const std::uint32_t meshIndex = node.MeshIndices[i];

            if (meshIndex >= scene.Meshes.size())
            {
                LOG_WARNING("{}  MeshRef[{}] -> INVALID {}", indent, i, meshIndex);
                continue;
            }

            const ImportedMesh& mesh = scene.Meshes[meshIndex];
            const std::uint32_t mat = mesh.MaterialIndex;

            LOG_INFO("{}  MeshRef[{}] -> Mesh '{}' (mat={}, verts={}, indices={})",
                indent,
                i,
                mesh.Name,
                mat,
                mesh.Vertices.size(),
                mesh.Indices.size());
        }

        for (auto& child : node.Children)
            DebugPrintNodeRecursive(child, scene, depth + 1);
    }

    void DebugPrintSceneTree(const ImportedScene& scene) noexcept
    {
        if (!scene.IsValid())
        {
            LOG_WARNING("DebugPrintSceneTree: INVALID scene");
            return;
        }

        LOG_INFO("IMPORTED SCENE TREE");
        DebugPrintNodeRecursive(scene.RootNode, scene, 0);
    }

} // namespace kfe::import
