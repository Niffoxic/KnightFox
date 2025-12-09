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

	/// <summary>
	/// Base interface for any scene object
	/// </summary>
    class KFE_API IKFESceneObject : public IKFEObject
    {
    public:
        virtual ~IKFESceneObject() override = default;

                  virtual void SetVisible(bool visible) noexcept;
        NODISCARD virtual bool IsVisible () const       noexcept;

        virtual void Update(const KFE_UPDATE_OBJECT_DESC& desc) = 0;

        NODISCARD virtual bool Build    (_In_ const KFE_BUILD_OBJECT_DESC& desc) = 0;
        NODISCARD virtual bool Destroy  () = 0;

        virtual void Render(_In_ const KFE_RENDER_OBJECT_DESC& desc) = 0;

        //  Transform
        NODISCARD virtual DirectX::XMFLOAT3 GetPosition() const noexcept;
        
        virtual void SetPosition(_In_ const DirectX::XMFLOAT3& position) noexcept;
        virtual void AddPosition(_In_ const DirectX::XMFLOAT3& delta)    noexcept;
        
        virtual void AddPositionX(float x) noexcept;
        virtual void AddPositionY(float y) noexcept;
        virtual void AddPositionZ(float z) noexcept;

        NODISCARD virtual DirectX::XMFLOAT4 GetOrientation() const noexcept;
        
        virtual void SetOrientation         (_In_ const DirectX::XMFLOAT4& orientation) noexcept;
        virtual void SetOrientationFromEuler(float pitch, float yaw, float roll)        noexcept;

        virtual void AddPitch(float pitch) noexcept;
        virtual void AddYaw  (float yaw)   noexcept;
        virtual void AddRoll (float roll)  noexcept;

        NODISCARD virtual DirectX::XMFLOAT3 GetScale() const noexcept;
        
        virtual void SetScale       (_In_ const DirectX::XMFLOAT3& scale) noexcept;
        virtual void SetUniformScale(float s)                             noexcept;
        virtual void AddScale       (_In_ const DirectX::XMFLOAT3& delta) noexcept;
        virtual void AddScaleX      (float x)                             noexcept;
        virtual void AddScaleY      (float y)                             noexcept;
        virtual void AddScaleZ      (float z)                             noexcept;

        NODISCARD virtual const DirectX::XMMATRIX& GetWorldMatrix() const noexcept;

        NODISCARD bool IsDirty   () const noexcept;
                  void SetDirty  () const noexcept;
                  void ResetDirty() const noexcept;

        NODISCARD bool IsInitialized() const noexcept;

    protected:
        DirectX::XMFLOAT3 m_position    { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT4 m_orientation { 0.0f, 0.0f, 0.0f, 1.0f };
        DirectX::XMFLOAT3 m_scale       { 1.0f, 1.0f, 1.0f };
        DirectX::XMMATRIX m_transform   { DirectX::XMMatrixIdentity() };
        bool m_bVisible    { true };
        bool m_bInitialized{ false };

    private:
        mutable bool m_bDirty{ false };
    };
}
