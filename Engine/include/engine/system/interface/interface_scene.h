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

#include <cstdint>
#include <string_view>
#include <DirectXMath.h>

struct ID3D12Fence;

namespace kfe
{
	class KFEDevice;
	class KFEGraphicsCommandList;
    class KFEGraphicsCmdQ;
    class KFEResourceHeap;

    typedef struct _KFE_COMMON_VERTEX_AND_PIXEL_CB_DESC
    {
        DirectX::XMMATRIX WorldMatrix;
        DirectX::XMMATRIX ViewMatrix;
        DirectX::XMMATRIX ProjectionMatrix;
        DirectX::XMMATRIX OrthogonalMatrix;
        DirectX::XMFLOAT2 Resolution;
        DirectX::XMFLOAT2 MousePosition;
        DirectX::XMFLOAT3 ObjectPosition;
        float _PaddingObjectPos;

        DirectX::XMFLOAT3 CameraPosition;
        float _PaddingCameraPos;

        DirectX::XMFLOAT3 PlayerPosition;
        float _PaddingPlayerPos;

        float         Time;
        std::uint32_t FrameIndex;
        float         DeltaTime;
        float         ZNear;
        float         ZFar;

        float _PaddingFinal[3];

    } KFE_COMMON_VERTEX_AND_PIXEL_CB_DESC;

    typedef struct _KFE_BUILD_OBJECT_DESC
    {
        KFEDevice*              Device;
        ID3D12Fence*            Fence;
        std::uint64_t           FenceValue;
        KFEGraphicsCmdQ*        ComandQueue;
        KFEGraphicsCommandList* CommandList;
        KFEResourceHeap*        ResourceHeap;
    } KFE_BUILD_OBJECT_DESC;

    typedef struct _KFE_RENDER_OBJECT_DESC
    {
        KFEGraphicsCommandList* CommandList;
        ID3D12Fence*            Fence;
        std::uint64_t           FenceValue;
    } KFE_RENDER_OBJECT_DESC;

    typedef struct _KFE_UPDATE_OBJECT_DESC
    {
        float deltaTime;
        float ZNear;
        float ZFar;
        DirectX::XMFLOAT3 CameraPosition;
        DirectX::XMFLOAT2 Resolution;
        DirectX::XMFLOAT2 MousePosition;
        DirectX::XMFLOAT3 PlayerPosition;
        DirectX::XMMATRIX ViewMatrix;
        DirectX::XMMATRIX PerpectiveMatrix;
        DirectX::XMMATRIX OrthographicMatrix;
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

    enum class ECullMode
    {
        Front,
        Back,
        None
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

	/// <summary>
	/// Base interface for any scene object
	/// </summary>

    class KFE_API IKFESceneObject : public IKFEObject
    {
    public:
        virtual ~IKFESceneObject() override = default;

        // Visibility
        virtual void SetVisible(bool visible);
        NODISCARD virtual bool IsVisible() const;

        // Core lifecycle
        virtual void Update(const KFE_UPDATE_OBJECT_DESC& desc) = 0;

        NODISCARD virtual bool Build(_In_ const KFE_BUILD_OBJECT_DESC& desc) = 0;
        NODISCARD virtual bool Destroy() = 0;

        virtual void Render(_In_ const KFE_RENDER_OBJECT_DESC& desc) = 0;

        // Serialization
        virtual JsonLoader GetJsonData() const = 0;
        virtual void       LoadFromJson(const JsonLoader& loader) = 0;

        // Shader properties
        virtual void        SetVertexShader(const std::string& path) = 0;
        NODISCARD virtual std::string VertexShader() const = 0;

        virtual void        SetPixelShader(const std::string& path) = 0;
        NODISCARD virtual std::string PixelShader() const = 0;

        virtual void        SetGeometryShader(const std::string& path) = 0;
        NODISCARD virtual std::string GeometryShader() const = 0;

        virtual void        SetHullShader(const std::string& path) = 0;
        NODISCARD virtual std::string HullShader() const = 0;

        virtual void        SetDomainShader(const std::string& path) = 0;
        NODISCARD virtual std::string DomainShader() const = 0;

        virtual void        SetComputeShader(const std::string& path) = 0;
        NODISCARD virtual std::string ComputeShader() const = 0;

        // Draw properties
        virtual void SetCullMode(const ECullMode mode) = 0;
        virtual void SetCullMode(const std::string& mode) = 0;

        virtual void SetDrawMode(const EDrawMode mode) = 0;
        virtual void SetDrawMode(const std::string& mode) = 0;

        virtual ECullMode   GetCullMode() const = 0;
        virtual std::string GetCullModeString() const = 0;
        virtual EDrawMode   GetDrawMode() const = 0;
        virtual std::string GetDrawModeString() const = 0;

        // Transform
        NODISCARD virtual DirectX::XMFLOAT3 GetPosition() const;

        virtual void SetPosition(_In_ const DirectX::XMFLOAT3& position);
        virtual void AddPosition(_In_ const DirectX::XMFLOAT3& delta);

        virtual void AddPositionX(float x);
        virtual void AddPositionY(float y);
        virtual void AddPositionZ(float z);

        NODISCARD virtual DirectX::XMFLOAT4 GetOrientation() const;

        virtual void SetOrientation(_In_ const DirectX::XMFLOAT4& orientation);
        virtual void SetOrientationFromEuler(float pitch, float yaw, float roll);

        virtual void AddPitch(float pitch);
        virtual void AddYaw(float yaw);
        virtual void AddRoll(float roll);

        NODISCARD virtual DirectX::XMFLOAT3 GetScale() const;

        virtual void SetScale(_In_ const DirectX::XMFLOAT3& scale);
        virtual void SetUniformScale(float s);
        virtual void AddScale(_In_ const DirectX::XMFLOAT3& delta);
        virtual void AddScaleX(float x);
        virtual void AddScaleY(float y);
        virtual void AddScaleZ(float z);

        NODISCARD virtual const DirectX::XMMATRIX& GetWorldMatrix() const;

        // These are non-virtual helpers -> noexcept is fine
        NODISCARD bool IsDirty() const noexcept;
        void SetDirty() const noexcept;
        void ResetDirty() const noexcept;

        NODISCARD bool IsInitialized() const noexcept;

        JsonLoader GetTransformJsonData() const noexcept;
        void       LoadTransformFromJson(const JsonLoader& loader) noexcept;

        // Type name
        void        SetTypeName(const std::string& typeName) { m_szTypeName = typeName; }
        std::string GetTypeName() const { return m_szTypeName; }

    protected:
        DirectX::XMFLOAT3 m_position{ 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT4 m_orientation{ 0.0f, 0.0f, 0.0f, 1.0f };
        DirectX::XMFLOAT3 m_scale{ 1.0f, 1.0f, 1.0f };
        DirectX::XMMATRIX m_transform{ DirectX::XMMatrixIdentity() };

        bool m_bVisible{ true };
        bool m_bInitialized{ false };

    private:
        std::string  m_szTypeName{ "Unknown" };
        mutable bool m_bDirty{ false };
    };
}
