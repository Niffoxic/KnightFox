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
#include "EngineAPI.h"
#include "engine/core.h"
#include "engine/utils/json_loader.h"
#include <memory>
#include <string>
#include <DirectXMath.h>

#include "imgui/imgui.h"
#include "engine/render_manager/api/root_signature.h"
#include "engine/render_manager/api/pso.h"
#include "engine/render_manager/api/sampler.h"
#include "engine/render_manager/api/heap/heap_cbv_srv_uav.h"
#include "engine/render_manager/api/heap/heap_sampler.h"
#include "engine/system/common_types.h"

namespace kfe
{
    struct alignas(16) KFE_COMMON_CB_GPU
    {
        //~ Transposed matrices
        DirectX::XMFLOAT4X4 WorldT;
        DirectX::XMFLOAT4X4 WorldInvTransposeT;

        DirectX::XMFLOAT4X4 ViewT;
        DirectX::XMFLOAT4X4 ProjT;
        DirectX::XMFLOAT4X4 ViewProjT;
        DirectX::XMFLOAT4X4 OrthoT;

        //~ Camera
        DirectX::XMFLOAT3 CameraPosWS;
        float             CameraNear;

        DirectX::XMFLOAT3 CameraForwardWS;
        float             CameraFar;

        DirectX::XMFLOAT3 CameraRightWS;
        float             _PadCamRight;

        DirectX::XMFLOAT3 CameraUpWS;
        float             _PadCamUp;

        DirectX::XMFLOAT3 ObjectPosWS;
        float             _PadObjPos;

        DirectX::XMFLOAT3 PlayerPosWS;
        float             _PadPlayerPos;

        //~ Render target and viewport
        DirectX::XMFLOAT2 Resolution;
        DirectX::XMFLOAT2 InvResolution;

        DirectX::XMFLOAT2 MousePosPixels;
        DirectX::XMFLOAT2 MousePosNDC;

        //~ Time and frame
        float         Time;
        float         DeltaTime;
        float         _PadTime0;
        float         _PadTime1;

        //~ Lights / flags
        std::uint32_t NumTotalLights;
        std::uint32_t RenderFlags;
        std::uint32_t _PadFlags0;
        std::uint32_t _PadFlags1;
    };

    typedef struct _KFE_BUILD_OBJECT_DESC
    {
        KFEDevice* Device;
        ID3D12Fence* Fence;
        std::uint64_t           FenceValue;
        KFEGraphicsCmdQ* ComandQueue;
        ID3D12GraphicsCommandList* CommandList;
        KFEResourceHeap* ResourceHeap;
        KFESamplerHeap* SamplerHeap;
    } KFE_BUILD_OBJECT_DESC;

    typedef struct _KFE_RENDER_OBJECT_DESC
    {
        ID3D12GraphicsCommandList* CommandList;
        ID3D12Fence* Fence;
        std::uint64_t           FenceValue;
        KFEShadowMap* ShadowMap;
    } KFE_RENDER_OBJECT_DESC;

    typedef struct _KFE_UPDATE_OBJECT_DESC
    {
        // Time
        float DeltaTime;
        float Time;
        std::uint32_t FrameIndex;

        // Camera
        DirectX::XMFLOAT3 CameraPosition;
        float             ZNear;

        DirectX::XMFLOAT3 CameraForwardWS;
        float             ZFar;

        DirectX::XMFLOAT3 CameraRightWS;
        DirectX::XMFLOAT3 CameraUpWS;

        // Render target / viewport
        DirectX::XMFLOAT2 Resolution;
        DirectX::XMFLOAT2 MousePosition;

        // Object / gameplay
        DirectX::XMFLOAT3 PlayerPosition;
        std::uint32_t     ObjectID;

        DirectX::XMFLOAT3 ObjectPosition;

        DirectX::XMMATRIX ViewMatrixT;
        DirectX::XMMATRIX PerpectiveMatrixT;
        DirectX::XMMATRIX OrthographicMatrixT;

    } KFE_UPDATE_OBJECT_DESC;

    enum class EDrawMode
    {
        Triangle,
        Point,
        WireFrame
    };

    inline static std::string ToString(const EDrawMode mode)
    {
        switch (mode)
        {
        case EDrawMode::Triangle:  return "Triangle";
        case EDrawMode::Point:     return "Point";
        case EDrawMode::WireFrame: return "WireFrame";
        }
        return "Triangle";
    }

    inline static EDrawMode FromStringToDraw(const std::string& mode)
    {
        if (mode == "Triangle")  return EDrawMode::Triangle;
        if (mode == "Point")     return EDrawMode::Point;
        if (mode == "WireFrame") return EDrawMode::WireFrame;

        return EDrawMode::Triangle; // fallback
    }

    enum class ECullMode: uint32_t
    {
        None  = 1,
        Front = 2,
        Back  = 3,
    };

    inline static std::string ToString(const ECullMode mode)
    {
        switch (mode)
        {
        case ECullMode::Front: return "Front";
        case ECullMode::Back:  return "Back";
        case ECullMode::None:  return "None";
        }
        return "None"; // fallback
    }

    inline static ECullMode FromStringToCull(const std::string& mode)
    {
        if (mode == "Front") return ECullMode::Front;
        if (mode == "Back")  return ECullMode::Back;
        if (mode == "None")  return ECullMode::None;

        return ECullMode::None; // fallback
    }

    struct SceneInfo
    {
        bool        Visible             { true };
        bool        Initialized         { false };
        bool        PipelineDirty       { true };
        bool        ShadowPipelineDirty { true };
        bool        AnyDirty            { true };
        std::string SceneType{ "IKFESceneObject" };
        std::string SceneName{ "No Name" };

        void Reset()
        {
            Visible = false;
            Initialized = false;
            PipelineDirty = true;
            ShadowPipelineDirty = true;
            AnyDirty = true;
        }

        void ImguiView(float deltaTime)
        {
            char buffer[256]{};

            std::snprintf(buffer, sizeof(buffer), "%s", SceneName.c_str());

            if (ImGui::InputText("Scene Name", buffer, sizeof(buffer)))
            {
                SceneName = buffer;
                AnyDirty = true;
            }
        }
    };

    struct ShaderInfo
    {
        std::string VertexShader      { "" };
        std::string PixelShader       { "" };
        std::string ShadowVertexShader{ "" };
        bool        Dirty             { false };

        JsonLoader GetJsonData() const
        {
            JsonLoader root{};

            root["VertexShader"]       = VertexShader;
            root["PixelShader"]        = PixelShader;
            root["ShadowVertexShader"] = ShadowVertexShader;
            return root;
        }

        void LoadFromJson(const JsonLoader& loader)
        {
            if (loader.Contains("VertexShader"))       VertexShader       = loader["VertexShader"].GetValue();
            if (loader.Contains("PixelShader"))        PixelShader        = loader["PixelShader"].GetValue();
            if (loader.Contains("ShadowVertexShader")) ShadowVertexShader = loader["ShadowVertexShader"].GetValue();
        }

        void ImguiView(float deltaTime, SceneInfo& info)
        {
            bool changed = false;

            ImGui::SeparatorText("Shaders");

            changed |= ImguiEditString("Vertex Shader", VertexShader);
            changed |= ImguiEditString("Pixel Shader", PixelShader);
            changed |= ImguiEditString("Shadow VS", ShadowVertexShader);

            if (changed)
            {
                Dirty = true;
                info.PipelineDirty = true;
            }
                
        }

    private:
        static inline bool ImguiEditString(const char* label, std::string& value) noexcept
        {
            char buffer[512]{};
            std::snprintf(buffer, sizeof(buffer), "%s", value.c_str());
            if (ImGui::InputText(label, buffer, sizeof(buffer)))
            {
                value = buffer;
                return true;
            }
            return false;
        }
    };

    struct DrawInfo
    {
        ECullMode CullMode{ ECullMode::Front };
        EDrawMode DrawMode{ EDrawMode::Triangle };

        JsonLoader GetJsonData() const
        {
            JsonLoader root{};
            root["CullMode"] = ToString(CullMode);
            root["DrawMode"] = ToString(DrawMode);
            return root;
        }

        void LoadFromJson(const JsonLoader& loader)
        {
            if (loader.Contains("CullMode")) CullMode = FromStringToCull(loader["CullMode"].GetValue());
            if (loader.Contains("DrawMode")) DrawMode = FromStringToDraw(loader["CullMode"].GetValue());
        }

        void ImguiView(float deltaTime, SceneInfo& info)
        {
            {
                const char* items[] = { "Front", "Back", "None" };
                int cur = 2;

                const std::string curStr = ToString(CullMode);
                if (curStr == "Front") cur = 0;
                else if (curStr == "Back") cur = 1;
                else cur = 2;

                if (ImGui::Combo("Cull Mode", &cur, items, 3))
                {
                    CullMode = FromStringToCull(items[cur]);
                    info.PipelineDirty = true;
                }
            }

            {
                const char* items[] = { "Triangle", "Point", "WireFrame" };
                int cur = 0;

                const std::string curStr = ToString(DrawMode);
                if (curStr == "Triangle") cur = 0;
                else if (curStr == "Point") cur = 1;
                else cur = 2;

                if (ImGui::Combo("Draw Mode", &cur, items, 3))
                    DrawMode = FromStringToDraw(items[cur]);
            }
        }
    };

    struct PassInfo
    {
        std::unique_ptr<KFERootSignature>  RootSignature{ nullptr };
        std::unique_ptr<KFEPipelineState>  Pipeline     { nullptr };
        std::unique_ptr<KFESampler>        Sampler      { nullptr };
        std::uint32_t                      SamplerIndex { KFE_INVALID_INDEX };
        KFEResourceHeap*                   ResourceHeap { nullptr };
        KFESamplerHeap*                    SamplerHeap  { nullptr };

        void FreeSamplerHeap() 
        {
            if (SamplerIndex == KFE_INVALID_INDEX) return;
            if (!SamplerHeap) return;
            (void)SamplerHeap->Free(SamplerIndex);
            SamplerIndex = KFE_INVALID_INDEX;
        }

        void FreeSignature() 
        {
            if (!RootSignature) return;
            (void)RootSignature->Destroy();
            RootSignature.reset();
        }

        void FreeSample() 
        {
            if (!Sampler) return;
            (void)Sampler->Destroy();
            Sampler.reset();
        }

        bool IsValidHeap() const
        {
            return ResourceHeap != nullptr && SamplerHeap != nullptr;
        }
    };

    struct TransformInfo
    {
        DirectX::XMFLOAT3 Position{ 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT4 Orientation{ 0.0f, 0.0f, 0.0f, 1.0f };
        DirectX::XMFLOAT3 Scale{ 1.0f, 1.0f, 1.0f };

        NODISCARD inline const DirectX::XMMATRIX& GetMatrix() const noexcept
        {
            if (m_dirty)
            {
                RebuildMatrix();
                m_dirty = false;
            }
            return m_transform;
        }

        inline void SetPosition(const DirectX::XMFLOAT3& p) noexcept
        {
            Position = p;
            m_dirty = true;
        }

        inline void AddPosition(const DirectX::XMFLOAT3& d) noexcept
        {
            Position.x += d.x;
            Position.y += d.y;
            Position.z += d.z;
            m_dirty = true;
        }

        inline void AddPositionX(float x) noexcept { Position.x += x; m_dirty = true; }
        inline void AddPositionY(float y) noexcept { Position.y += y; m_dirty = true; }
        inline void AddPositionZ(float z) noexcept { Position.z += z; m_dirty = true; }

        inline void SetScale(const DirectX::XMFLOAT3& s) noexcept
        {
            Scale = s;
            m_dirty = true;
        }

        inline void SetUniformScale(float s) noexcept
        {
            Scale = { s, s, s };
            m_dirty = true;
        }

        inline void AddScale(const DirectX::XMFLOAT3& d) noexcept
        {
            Scale.x += d.x;
            Scale.y += d.y;
            Scale.z += d.z;
            m_dirty = true;
        }

        inline void AddScaleX(float x) noexcept { Scale.x += x; m_dirty = true; }
        inline void AddScaleY(float y) noexcept { Scale.y += y; m_dirty = true; }
        inline void AddScaleZ(float z) noexcept { Scale.z += z; m_dirty = true; }

        inline void SetOrientation(const DirectX::XMFLOAT4& q) noexcept
        {
            using namespace DirectX;
            Orientation = q;

            XMVECTOR v = XMLoadFloat4(&Orientation);
            v = XMQuaternionNormalize(v);
            XMStoreFloat4(&Orientation, v);

            m_dirty = true;
        }

        inline void SetOrientationFromEuler(float pitch, float yaw, float roll) noexcept
        {
            using namespace DirectX;
            XMVECTOR q = XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);
            q = XMQuaternionNormalize(q);
            XMStoreFloat4(&Orientation, q);
            m_dirty = true;
        }

        inline void AddPitch(float pitch) noexcept
        {
            using namespace DirectX;
            XMVECTOR cur = XMLoadFloat4(&Orientation);
            XMVECTOR dq = XMQuaternionRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), pitch);
            XMVECTOR out = XMQuaternionNormalize(XMQuaternionMultiply(cur, dq));
            XMStoreFloat4(&Orientation, out);
            m_dirty = true;
        }

        inline void AddYaw(float yaw) noexcept
        {
            using namespace DirectX;
            XMVECTOR cur = XMLoadFloat4(&Orientation);
            XMVECTOR dq = XMQuaternionRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), yaw);
            XMVECTOR out = XMQuaternionNormalize(XMQuaternionMultiply(cur, dq));
            XMStoreFloat4(&Orientation, out);
            m_dirty = true;
        }

        inline void AddRoll(float roll) noexcept
        {
            using namespace DirectX;
            XMVECTOR cur    = XMLoadFloat4(&Orientation);
            XMVECTOR dq     = XMQuaternionRotationAxis(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), roll);
            XMVECTOR out    = XMQuaternionNormalize(XMQuaternionMultiply(cur, dq));
            XMStoreFloat4(&Orientation, out);
            m_dirty = true;
        }

        NODISCARD inline JsonLoader GetJsonData() const
        {
            JsonLoader j{};

            j["Position"]["x"] = Position.x;
            j["Position"]["y"] = Position.y;
            j["Position"]["z"] = Position.z;

            j["Orientation"]["x"] = Orientation.x;
            j["Orientation"]["y"] = Orientation.y;
            j["Orientation"]["z"] = Orientation.z;
            j["Orientation"]["w"] = Orientation.w;

            j["Scale"]["x"] = Scale.x;
            j["Scale"]["y"] = Scale.y;
            j["Scale"]["z"] = Scale.z;

            return j;
        }

        inline void LoadFromJson(const JsonLoader& loader) noexcept
        {
            if (loader.Contains("Position"))
            {
                const auto& p = loader["Position"];
                if (p.Contains("x")) Position.x = p["x"].AsFloat();
                if (p.Contains("y")) Position.y = p["y"].AsFloat();
                if (p.Contains("z")) Position.z = p["z"].AsFloat();
            }

            if (loader.Contains("Orientation"))
            {
                const auto& q = loader["Orientation"];
                if (q.Contains("x")) Orientation.x = q["x"].AsFloat();
                if (q.Contains("y")) Orientation.y = q["y"].AsFloat();
                if (q.Contains("z")) Orientation.z = q["z"].AsFloat();
                if (q.Contains("w")) Orientation.w = q["w"].AsFloat();

                using namespace DirectX;
                XMVECTOR v = XMLoadFloat4(&Orientation);
                v = XMQuaternionNormalize(v);
                XMStoreFloat4(&Orientation, v);
            }

            if (loader.Contains("Scale"))
            {
                const auto& s = loader["Scale"];
                if (s.Contains("x")) Scale.x = s["x"].AsFloat();
                if (s.Contains("y")) Scale.y = s["y"].AsFloat();
                if (s.Contains("z")) Scale.z = s["z"].AsFloat();
            }

            m_dirty = true;
        }

        void ImguiView(float deltaTime)
        {
            bool changed = false;

            changed |= ImGui::DragFloat3("Position", &Position.x, 0.01f);

            DirectX::XMFLOAT3 eulerDeg = GetEulerDegrees();
            if (ImGui::DragFloat3("Rotation (deg)", &eulerDeg.x, 0.25f))
            {
                SetOrientationFromEuler(
                    DirectX::XMConvertToRadians(eulerDeg.x),
                    DirectX::XMConvertToRadians(eulerDeg.y),
                    DirectX::XMConvertToRadians(eulerDeg.z));
                changed = true;
            }

            changed |= ImGui::DragFloat3("Scale", &Scale.x, 0.01f);

            if (ImGui::Button("Reset Transform"))
            {
                Position    = { 0.0f, 0.0f, 0.0f };
                Orientation = { 0.0f, 0.0f, 0.0f, 1.0f };
                Scale       = { 1.0f, 1.0f, 1.0f };
                changed = true;
            }

            if (changed)
                m_dirty = true;
        }

    private:

        NODISCARD inline DirectX::XMFLOAT3 GetEulerDegrees() const noexcept
        {
            using namespace DirectX;

            XMVECTOR q = XMLoadFloat4(&Orientation);
            q = XMQuaternionNormalize(q);

            const XMMATRIX R = XMMatrixRotationQuaternion(q);

            XMVECTOR scl{}, rotQ{}, trans{};
            XMMatrixDecompose(&scl, &rotQ, &trans, R);

            XMFLOAT4 rq{};
            XMStoreFloat4(&rq, rotQ);

            float pitch = 0.0f;
            float yaw = 0.0f;
            float roll = 0.0f;

            {
                const float x = rq.x, y = rq.y, z = rq.z, w = rq.w;

                const float sinp = 2.0f * (w * x - z * y);
                if (fabsf(sinp) >= 1.0f) pitch = copysignf(DirectX::XM_PIDIV2, sinp);
                else                      pitch = asinf(sinp);

                const float siny = 2.0f * (w * y + x * z);
                const float cosy = 1.0f - 2.0f * (y * y + x * x);
                yaw = atan2f(siny, cosy);

                const float sinr = 2.0f * (w * z + y * x);
                const float cosr = 1.0f - 2.0f * (z * z + x * x);
                roll = atan2f(sinr, cosr);
            }

            return {
                XMConvertToDegrees(pitch),
                XMConvertToDegrees(yaw),
                XMConvertToDegrees(roll)
            };
        }

        inline void RebuildMatrix() const noexcept
        {
            using namespace DirectX;

            const XMVECTOR p = XMLoadFloat3(&Position);

            XMVECTOR q = XMLoadFloat4(&Orientation);
            q = XMQuaternionNormalize(q);

            const XMMATRIX S = XMMatrixScaling(Scale.x, Scale.y, Scale.z);
            const XMMATRIX R = XMMatrixRotationQuaternion(q);
            const XMMATRIX T = XMMatrixTranslationFromVector(p);

            m_transform = T * R * S;
        }

    private:
        mutable bool              m_dirty    { true };
        mutable DirectX::XMMATRIX m_transform{ DirectX::XMMatrixIdentity() };
    };

  
} // namespace kfe
