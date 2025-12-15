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

#include <DirectXMath.h>
#include <string>

namespace kfe
{
	class IKFESceneObject;

	class KFE_API KFECamera final : public IKFEObject
	{
	public:
		 KFECamera();
		~KFECamera();

		KFECamera(const KFECamera&) noexcept = delete;
		KFECamera(KFECamera&&)      noexcept = delete;

		KFECamera& operator=(const KFECamera&) noexcept = delete;
		KFECamera& operator=(KFECamera&&)      noexcept = delete;

		//~ IKFEObject
		NODISCARD std::string GetName()        const noexcept override;
		NODISCARD std::string GetDescription() const noexcept override;

		void Update(float deltaTime);

		//~ View and Projections getters
		NODISCARD DirectX::XMMATRIX GetViewMatrix		 () const noexcept;
		NODISCARD DirectX::XMMATRIX GetPerspectiveMatrix () const noexcept;
		NODISCARD DirectX::XMMATRIX GetOrthographicMatrix() const noexcept;

		//~ Position Getters
		NODISCARD const DirectX::XMFLOAT3& GetPosition() const noexcept;
		NODISCARD float GetPositionX() const noexcept;
		NODISCARD float GetPositionY() const noexcept;
		NODISCARD float GetPositionZ() const noexcept;

		//~ Orientation Getters
		NODISCARD const DirectX::XMFLOAT4& GetOrientation() const noexcept;
		NODISCARD float GetPitch() const noexcept;
		NODISCARD float GetYaw	() const noexcept;
		NODISCARD float GetRoll	() const noexcept;

		//~ Direction Vectors
		NODISCARD DirectX::XMVECTOR GetForwardVector() const noexcept;
		NODISCARD DirectX::XMVECTOR GetRightVector  () const noexcept;
		NODISCARD DirectX::XMVECTOR GetUpVector		() const noexcept;

		//~ Projection Properties
		NODISCARD float GetFOV		() const noexcept;
		NODISCARD float GetNearZ	() const noexcept;
		NODISCARD float GetFarZ		() const noexcept;
		NODISCARD float GetAspect	() const noexcept;
		NODISCARD float GetOrthoSize() const noexcept;

		//~ Position Setters
		void SetPosition(const DirectX::XMFLOAT3& pos) noexcept;
		void SetPosition(float x, float y, float z)    noexcept;

		void SetPositionX(float x) noexcept;
		void SetPositionY(float y) noexcept;
		void SetPositionZ(float z) noexcept;

		//~ Additive movement
		void AddPosition(const DirectX::XMFLOAT3& delta) noexcept;
		void AddPosition(float dx, float dy, float dz)   noexcept;
		
		void AddPositionX(float dx) noexcept;
		void AddPositionY(float dy) noexcept;
		void AddPositionZ(float dz) noexcept;

		//~ Rotation Setters in rad
		void SetPitch	   (float pitch) noexcept;
		void SetYaw		   (float yaw)   noexcept;
		void SetRoll	   (float roll)  noexcept;
		
		void SetEulerAngles(float pitch,
							float yaw,
							float roll) noexcept;

		//~ Additive rotation (in radians)
		void AddPitch(float dp) noexcept;
		void AddYaw	 (float dy) noexcept;
		void AddRoll (float dr) noexcept;

		//~ Orientation setter
		void SetOrientation(const DirectX::XMFLOAT4& quat) noexcept;

		//~ Movement API
		void MoveForward (float dt) noexcept;
		void MoveBackward(float dt) noexcept;
		void MoveRight	 (float dt) noexcept;
		void MoveLeft	 (float dt) noexcept;
		void MoveUp		 (float dt) noexcept;
		void MoveDown	 (float dt) noexcept;

		//~ Rotation API
		void RotateYaw  (float dt) noexcept;
		void RotatePitch(float dt) noexcept;
		void RotateRoll (float dt) noexcept;

		//~ Utility
		void LookAt(const DirectX::XMFLOAT3& target) noexcept;
		void LookAt(float x, float y, float z)		 noexcept;

		void FollowObject(IKFESceneObject* object) noexcept;

		void SetProjection(
			float fovRadians,
			float aspect,
			float nearZ,
			float farZ) noexcept;

		void SetOrthographic(
			float size,
			float aspect,
			float nearZ,
			float farZ) noexcept;

		void SetMovementSpeed(float v) noexcept;
		void SetRotationSpeed(float v) noexcept;

		NODISCARD float GetMovementSpeed() const noexcept;
		NODISCARD float GetRotationSpeed() const noexcept;

		//~ Smooth Follow Configuration
				  void EnableSmoothFollow(bool enable) noexcept;
		NODISCARD bool IsSmoothFollowEnabled() const   noexcept;

		// Offset from target
		void SetFollowOffset(const DirectX::XMFLOAT3& offset) noexcept;
		void SetFollowOffset(float x, float y, float z)		  noexcept;

		NODISCARD const DirectX::XMFLOAT3& GetFollowOffset() const noexcept;

				  void  SetFollowPositionSmoothing(float smoothing) noexcept;
		NODISCARD float GetFollowPositionSmoothing() const noexcept;

				  void  SetFollowRotationSmoothing(float smoothing) noexcept;
		NODISCARD float GetFollowRotationSmoothing() const noexcept;

				  void  SetFollowLagSeconds(float lagSeconds) noexcept;
		NODISCARD float GetFollowLagSeconds() const noexcept;

	private:
		void RecalculateViewMatrix		() noexcept;
		void RecalculateProjectionMatrix() noexcept;

		void MarkViewDirty		() const noexcept;
		void MarkProjectionDirty() const noexcept;

	private:
		//~ Transform
		DirectX::XMFLOAT3 Position{ 0.f, 0.f, -5.f };
		DirectX::XMFLOAT3 m_scale	{ 1.f, 1.f, 1.f };

		//~ Euler rotation in rad
		float m_pitch{ 0.f };
		float m_yaw  { 0.f };
		float m_roll { 0.f };

		//~ Quaternion orientation
		DirectX::XMFLOAT4 m_orientation{ 0.f, 0.f, 0.f, 1.f };

		//~ Projection Settings
		float m_fovRadians { DirectX::XM_PIDIV4 };
		float m_aspectRatio{ 1.777f };
		float m_nearZ	   { 0.1f };
		float m_farZ	   { 5000.f };
		float m_orthoSize  { 10.f };

		//~ Cached Matrices and Dirty Flags
		mutable DirectX::XMFLOAT4X4 m_viewMatrix{};
		mutable DirectX::XMFLOAT4X4 m_perspecticProjMatrix{};
		mutable DirectX::XMFLOAT4X4 m_orthogonalProjMatrix{};

		mutable bool m_viewDirty{ true };
		mutable bool m_perspecticProjDirty{ true };
		mutable bool m_orthogonalProjDirty{ true };

		//~ Movement Parameters
		float m_movementSpeed{ 10.f };
		float m_rotationSpeed{ 1.f }; 

		//~ Follow Target and Smooth Follow Params
		IKFESceneObject* m_followTarget{ nullptr };

		bool m_smoothFollowEnabled{ false };

		//~ Offset relative to target space
		DirectX::XMFLOAT3 m_followOffset{ 0.f, 5.f, -10.f };

		//~ Smoothing factors
		float m_followPositionSmoothing{ 8.f };
		float m_followRotationSmoothing{ 8.f };
		float m_followLagSeconds	   { 0.f };

	};
} // namespace kfe
