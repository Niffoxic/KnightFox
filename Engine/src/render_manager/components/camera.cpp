// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  License   : MIT
 *  -----------------------------------------------------------------------------
 */
#include "pch.h"
#include "engine/render_manager/components/camera.h"
#include "engine/system/interface/interface_scene.h"

#include <algorithm>
#include <cmath> 

using namespace DirectX;

namespace
{
	static inline void ExtractEulerFromQuaternion(const DirectX::XMFLOAT4& qIn,
												  float& pitchOut,
												  float& yawOut,
												  float& rollOut) noexcept
	{
		const float x = qIn.x;
		const float y = qIn.y;
		const float z = qIn.z;
		const float w = qIn.w;

		const float ysqr = y * y;

		// roll
		float t0 = +2.0f * (w * x + y * z);
		float t1 = +1.0f - 2.0f * (x * x + ysqr);
		rollOut = std::atan2f(t0, t1);

		// pitch
		float t2 = +2.0f * (w * y - z * x);
		t2 = std::clamp(t2, -1.0f, 1.0f);
		pitchOut = std::asinf(t2);

		// yaw
		float t3 = +2.0f * (w * z + x * y);
		float t4 = +1.0f - 2.0f * (ysqr + z * z);
		yawOut = std::atan2f(t3, t4);
	}
} // namespace

kfe::KFECamera::KFECamera()
{
	XMStoreFloat4x4(&m_viewMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_perspecticProjMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_orthogonalProjMatrix, XMMatrixIdentity());

	const auto q = XMQuaternionRotationRollPitchYaw(m_pitch, m_yaw, m_roll);
	XMStoreFloat4(&m_orientation, q);

	m_viewDirty			  = true;
	m_perspecticProjDirty = true;
	m_orthogonalProjDirty = true;
}

kfe::KFECamera::~KFECamera() = default;

_Use_decl_annotations_
std::string kfe::KFECamera::GetName() const noexcept
{
	return "KFECamera";
}

_Use_decl_annotations_
std::string kfe::KFECamera::GetDescription() const noexcept
{
	return "KnightFox engine camera component";
}

void kfe::KFECamera::Update(float deltaTime)
{
	if (m_followTarget)
	{
		XMFLOAT3 targetPos			= m_followTarget->Transform.Position;
		XMFLOAT4 targetOrientFloat	= m_followTarget->Transform.Orientation;
		XMVECTOR targetOrient		= XMLoadFloat4(&targetOrientFloat);
		XMVECTOR offset				= XMLoadFloat3(&m_followOffset);
		XMVECTOR rotatedOffset		= XMVector3Rotate(offset, targetOrient);
		XMVECTOR desiredCamPos		= XMVectorAdd(XMLoadFloat3(&targetPos), rotatedOffset);

		if (!m_smoothFollowEnabled)
		{
			XMStoreFloat3(&Position, desiredCamPos);
			m_pitch = 0.f;
			m_yaw	= 0.f;
			m_roll	= 0.f;

			//~ Face toward target
			XMVECTOR camPos  = XMLoadFloat3(&Position);
			XMVECTOR forward = XMVector3Normalize(XMVectorSubtract(XMLoadFloat3(&targetPos), camPos));

			float fx = XMVectorGetX(forward);
			float fy = XMVectorGetY(forward);
			float fz = XMVectorGetZ(forward);

			m_pitch = std::asinf(-fy);
			m_yaw   = std::atan2f(fx, fz);
			m_roll  = 0.f;

			XMVECTOR q = XMQuaternionRotationRollPitchYaw(m_pitch, m_yaw, m_roll);
			XMStoreFloat4(&m_orientation, XMQuaternionNormalize(q));

			MarkViewDirty();
			return;
		}

		//~ Smooth follow

		//~ delay before camera reacts
		if (m_followLagSeconds > 0.f)
		{
			if (m_followLagSeconds > deltaTime)
			{
				m_followLagSeconds -= deltaTime;
				return;
			}
		}

		//~ Smooth position interpolation
		{
			XMVECTOR currentPos = XMLoadFloat3(&Position);

			float t			= 1.f - std::expf(-m_followPositionSmoothing * deltaTime);
			XMVECTOR newPos = XMVectorLerp(currentPos, desiredCamPos, t);

			XMStoreFloat3(&Position, newPos);
		}

		//~ Smooth rotation interpolation
		{
			XMVECTOR currentQ = XMLoadFloat4(&m_orientation);
			XMVECTOR desiredQ = targetOrient;

			float t = 1.f - std::expf(-m_followRotationSmoothing * deltaTime);

			XMVECTOR blendedQ = XMQuaternionSlerp(currentQ, desiredQ, t);
			blendedQ		  = XMQuaternionNormalize(blendedQ);

			XMStoreFloat4			  (&m_orientation, blendedQ);
			ExtractEulerFromQuaternion(m_orientation, m_pitch,
									   m_yaw, m_roll);
		}
		MarkViewDirty();
	}
}

_Use_decl_annotations_
DirectX::XMMATRIX kfe::KFECamera::GetViewMatrix() const noexcept
{
	if (m_viewDirty)
	{
		const_cast<KFECamera*>(this)->RecalculateViewMatrix();
	}
	return XMLoadFloat4x4(&m_viewMatrix);
}

_Use_decl_annotations_
DirectX::XMMATRIX kfe::KFECamera::GetPerspectiveMatrix() const noexcept
{
	if (m_perspecticProjDirty)
	{
		const_cast<KFECamera*>(this)->RecalculateProjectionMatrix();
	}
	return XMLoadFloat4x4(&m_perspecticProjMatrix);
}

_Use_decl_annotations_
DirectX::XMMATRIX kfe::KFECamera::GetOrthographicMatrix() const noexcept
{
	if (m_orthogonalProjDirty)
	{
		const_cast<KFECamera*>(this)->RecalculateProjectionMatrix();
	}
	return XMLoadFloat4x4(&m_orthogonalProjMatrix);
}

_Use_decl_annotations_
const DirectX::XMFLOAT3& kfe::KFECamera::GetPosition() const noexcept
{
	return Position;
}

_Use_decl_annotations_
float kfe::KFECamera::GetPositionX() const noexcept
{
	return Position.x;
}

_Use_decl_annotations_
float kfe::KFECamera::GetPositionY() const noexcept
{
	return Position.y;
}

_Use_decl_annotations_
float kfe::KFECamera::GetPositionZ() const noexcept
{
	return Position.z;
}

_Use_decl_annotations_
const DirectX::XMFLOAT4& kfe::KFECamera::GetOrientation() const noexcept
{
	return m_orientation;
}

_Use_decl_annotations_
float kfe::KFECamera::GetPitch() const noexcept
{
	return m_pitch;
}

_Use_decl_annotations_
float kfe::KFECamera::GetYaw() const noexcept
{
	return m_yaw;
}

_Use_decl_annotations_
float kfe::KFECamera::GetRoll() const noexcept
{
	return m_roll;
}

_Use_decl_annotations_
DirectX::XMVECTOR kfe::KFECamera::GetForwardVector() const noexcept
{
	XMVECTOR q   = XMLoadFloat4(&m_orientation);
	XMMATRIX rot = XMMatrixRotationQuaternion(q);
	return XMVector3TransformNormal(XMVectorSet(0.f, 0.f, 1.f, 0.f), rot);
}

_Use_decl_annotations_
DirectX::XMVECTOR kfe::KFECamera::GetRightVector() const noexcept
{
	XMVECTOR q   = XMLoadFloat4(&m_orientation);
	XMMATRIX rot = XMMatrixRotationQuaternion(q);
	return XMVector3TransformNormal(XMVectorSet(1.f, 0.f, 0.f, 0.f), rot);
}

_Use_decl_annotations_
DirectX::XMVECTOR kfe::KFECamera::GetUpVector() const noexcept
{
	XMVECTOR q   = XMLoadFloat4(&m_orientation);
	XMMATRIX rot = XMMatrixRotationQuaternion(q);
	return XMVector3TransformNormal(XMVectorSet(0.f, 1.f, 0.f, 0.f), rot);
}

_Use_decl_annotations_
float kfe::KFECamera::GetFOV() const noexcept
{
	return m_fovRadians;
}

_Use_decl_annotations_
float kfe::KFECamera::GetNearZ() const noexcept
{
	return m_nearZ;
}

_Use_decl_annotations_
float kfe::KFECamera::GetFarZ() const noexcept
{
	return m_farZ;
}

_Use_decl_annotations_
float kfe::KFECamera::GetAspect() const noexcept
{
	return m_aspectRatio;
}

_Use_decl_annotations_
float kfe::KFECamera::GetOrthoSize() const noexcept
{
	return m_orthoSize;
}

void kfe::KFECamera::SetPosition(const DirectX::XMFLOAT3& pos) noexcept
{
	Position = pos;
	MarkViewDirty();
}

void kfe::KFECamera::SetPosition(float x, float y, float z) noexcept
{
	Position.x = x;
	Position.y = y;
	Position.z = z;
	MarkViewDirty();
}

void kfe::KFECamera::SetPositionX(float x) noexcept
{
	Position.x = x;
	MarkViewDirty();
}

void kfe::KFECamera::SetPositionY(float y) noexcept
{
	Position.y = y;
	MarkViewDirty();
}

void kfe::KFECamera::SetPositionZ(float z) noexcept
{
	Position.z = z;
	MarkViewDirty();
}

void kfe::KFECamera::AddPosition(const DirectX::XMFLOAT3& delta) noexcept
{
	Position.x += delta.x;
	Position.y += delta.y;
	Position.z += delta.z;
	MarkViewDirty();
}

void kfe::KFECamera::AddPosition(float dx, float dy, float dz) noexcept
{
	Position.x += dx;
	Position.y += dy;
	Position.z += dz;
	MarkViewDirty();
}

void kfe::KFECamera::AddPositionX(float dx) noexcept
{
	Position.x += dx;
	MarkViewDirty();
}

void kfe::KFECamera::AddPositionY(float dy) noexcept
{
	Position.y += dy;
	MarkViewDirty();
}

void kfe::KFECamera::AddPositionZ(float dz) noexcept
{
	Position.z += dz;
	MarkViewDirty();
}

void kfe::KFECamera::SetPitch(float pitch) noexcept
{
	m_pitch = pitch;
	XMVECTOR q = XMQuaternionRotationRollPitchYaw(m_pitch, m_yaw, m_roll);
	XMStoreFloat4(&m_orientation, XMQuaternionNormalize(q));
	MarkViewDirty();
}

void kfe::KFECamera::SetYaw(float yaw) noexcept
{
	m_yaw = yaw;
	XMVECTOR q = XMQuaternionRotationRollPitchYaw(m_pitch, m_yaw, m_roll);
	XMStoreFloat4(&m_orientation, XMQuaternionNormalize(q));
	MarkViewDirty();
}

void kfe::KFECamera::SetRoll(float roll) noexcept
{
	m_roll = roll;
	XMVECTOR q = XMQuaternionRotationRollPitchYaw(m_pitch, m_yaw, m_roll);
	XMStoreFloat4(&m_orientation, XMQuaternionNormalize(q));
	MarkViewDirty();
}

void kfe::KFECamera::SetEulerAngles(float pitch, float yaw, float roll) noexcept
{
	m_pitch = pitch;
	m_yaw = yaw;
	m_roll = roll;

	XMVECTOR q = XMQuaternionRotationRollPitchYaw(m_pitch, m_yaw, m_roll);
	XMStoreFloat4(&m_orientation, XMQuaternionNormalize(q));
	MarkViewDirty();
}

void kfe::KFECamera::AddPitch(float dp) noexcept
{
	m_pitch += dp;
	XMVECTOR q = XMQuaternionRotationRollPitchYaw(m_pitch, m_yaw, m_roll);
	XMStoreFloat4(&m_orientation, XMQuaternionNormalize(q));
	MarkViewDirty();
}

void kfe::KFECamera::AddYaw(float dy) noexcept
{
	m_yaw += dy;
	XMVECTOR q = XMQuaternionRotationRollPitchYaw(m_pitch, m_yaw, m_roll);
	XMStoreFloat4(&m_orientation, XMQuaternionNormalize(q));
	MarkViewDirty();
}

void kfe::KFECamera::AddRoll(float dr) noexcept
{
	m_roll += dr;
	XMVECTOR q = XMQuaternionRotationRollPitchYaw(m_pitch, m_yaw, m_roll);
	XMStoreFloat4(&m_orientation, XMQuaternionNormalize(q));
	MarkViewDirty();
}

void kfe::KFECamera::SetOrientation(const DirectX::XMFLOAT4& quat) noexcept
{
	XMVECTOR q = XMLoadFloat4(&quat);
	q = XMQuaternionNormalize(q);
	XMStoreFloat4(&m_orientation, q);

	ExtractEulerFromQuaternion(m_orientation, m_pitch, m_yaw, m_roll);

	MarkViewDirty();
}

void kfe::KFECamera::MoveForward(float dt) noexcept
{
	XMVECTOR dir = GetForwardVector();
	XMVECTOR delta = XMVectorScale(dir, m_movementSpeed * dt);

	XMFLOAT3 d{};
	XMStoreFloat3(&d, delta);
	AddPosition(d);
}

void kfe::KFECamera::MoveBackward(float dt) noexcept
{
	MoveForward(-dt);
}

void kfe::KFECamera::MoveRight(float dt) noexcept
{
	XMVECTOR dir = GetRightVector();
	XMVECTOR delta = XMVectorScale(dir, m_movementSpeed * dt);

	XMFLOAT3 d{};
	XMStoreFloat3(&d, delta);
	AddPosition(d);
}

void kfe::KFECamera::MoveLeft(float dt) noexcept
{
	MoveRight(-dt);
}

void kfe::KFECamera::MoveUp(float dt) noexcept
{
	XMVECTOR dir = GetUpVector();
	XMVECTOR delta = XMVectorScale(dir, m_movementSpeed * dt);

	XMFLOAT3 d{};
	XMStoreFloat3(&d, delta);
	AddPosition(d);
}

void kfe::KFECamera::MoveDown(float dt) noexcept
{
	MoveUp(-dt);
}

void kfe::KFECamera::RotateYaw(float dt) noexcept
{
	AddYaw(m_rotationSpeed * dt);
}

void kfe::KFECamera::RotatePitch(float dt) noexcept
{
	AddPitch(m_rotationSpeed * dt);
}

void kfe::KFECamera::RotateRoll(float dt) noexcept
{
	AddRoll(m_rotationSpeed * dt);
}

void kfe::KFECamera::LookAt(const DirectX::XMFLOAT3& target) noexcept
{
	XMVECTOR eye = XMLoadFloat3(&Position);
	XMVECTOR targetV = XMLoadFloat3(&target);
	XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	XMMATRIX view = XMMatrixLookAtLH(eye, targetV, up);
	XMStoreFloat4x4(&m_viewMatrix, view);
	m_viewDirty = false;

	XMMATRIX invView = XMMatrixInverse(nullptr, view);
	XMVECTOR forward = XMVector3Normalize(invView.r[2]);

	const float fx = XMVectorGetX(forward);
	const float fy = XMVectorGetY(forward);
	const float fz = XMVectorGetZ(forward);

	m_pitch = std::asinf(-fy);
	m_yaw	= std::atan2f(fx, fz);
	m_roll	= 0.f;

	XMVECTOR q = XMQuaternionRotationRollPitchYaw(m_pitch, m_yaw, m_roll);
	XMStoreFloat4(&m_orientation, XMQuaternionNormalize(q));
}

void kfe::KFECamera::LookAt(float x, float y, float z) noexcept
{
	XMFLOAT3 target{ x, y, z };
	LookAt(target);
}

void kfe::KFECamera::FollowObject(IKFESceneObject* object) noexcept
{
	m_followTarget = object;
}

void kfe::KFECamera::SetProjection(float fovRadians, float aspect, float nearZ, float farZ) noexcept
{
	m_fovRadians	= fovRadians;
	m_aspectRatio	= aspect;
	m_nearZ			= nearZ;
	m_farZ			= farZ;

	MarkProjectionDirty();
}

void kfe::KFECamera::SetOrthographic(float size, float aspect, float nearZ, float farZ) noexcept
{
	m_orthoSize   = size;
	m_aspectRatio = aspect;
	m_nearZ		  = nearZ;
	m_farZ		  = farZ;

	MarkProjectionDirty();
}

void kfe::KFECamera::SetMovementSpeed(float v) noexcept
{
	m_movementSpeed = (v < 0.f) ? 0.f : v;
}

void kfe::KFECamera::SetRotationSpeed(float v) noexcept
{
	m_rotationSpeed = (v < 0.f) ? 0.f : v;
}

_Use_decl_annotations_
float kfe::KFECamera::GetMovementSpeed() const noexcept
{
	return m_movementSpeed;
}

_Use_decl_annotations_
float kfe::KFECamera::GetRotationSpeed() const noexcept
{
	return m_rotationSpeed;
}

void kfe::KFECamera::EnableSmoothFollow(bool enable) noexcept
{
	m_smoothFollowEnabled = enable;
}

_Use_decl_annotations_
bool kfe::KFECamera::IsSmoothFollowEnabled() const noexcept
{
	return m_smoothFollowEnabled;
}

void kfe::KFECamera::SetFollowOffset(const DirectX::XMFLOAT3& offset) noexcept
{
	m_followOffset = offset;
}

void kfe::KFECamera::SetFollowOffset(float x, float y, float z) noexcept
{
	m_followOffset.x = x;
	m_followOffset.y = y;
	m_followOffset.z = z;
}

_Use_decl_annotations_
const DirectX::XMFLOAT3& kfe::KFECamera::GetFollowOffset() const noexcept
{
	return m_followOffset;
}

void kfe::KFECamera::SetFollowPositionSmoothing(float smoothing) noexcept
{
	m_followPositionSmoothing = (smoothing < 0.f) ? 0.f : smoothing;
}

_Use_decl_annotations_
float kfe::KFECamera::GetFollowPositionSmoothing() const noexcept
{
	return m_followPositionSmoothing;
}

void kfe::KFECamera::SetFollowRotationSmoothing(float smoothing) noexcept
{
	m_followRotationSmoothing = (smoothing < 0.f) ? 0.f : smoothing;
}

_Use_decl_annotations_
float kfe::KFECamera::GetFollowRotationSmoothing() const noexcept
{
	return m_followRotationSmoothing;
}

void kfe::KFECamera::SetFollowLagSeconds(float lagSeconds) noexcept
{
	m_followLagSeconds = (lagSeconds < 0.f) ? 0.f : lagSeconds;
}

_Use_decl_annotations_
float kfe::KFECamera::GetFollowLagSeconds() const noexcept
{
	return m_followLagSeconds;
}

void kfe::KFECamera::RecalculateViewMatrix() noexcept
{
	XMVECTOR eye	 = XMLoadFloat3(&Position);
	XMVECTOR forward = GetForwardVector();
	XMVECTOR up		 = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	XMMATRIX view = XMMatrixLookToLH(eye, forward, up);
	XMStoreFloat4x4(&m_viewMatrix, view);

	m_viewDirty = false;
}

void kfe::KFECamera::RecalculateProjectionMatrix() noexcept
{
	// Perspective
	XMMATRIX persp = XMMatrixPerspectiveFovLH(m_fovRadians, m_aspectRatio, m_nearZ, m_farZ);
	XMStoreFloat4x4(&m_perspecticProjMatrix, persp);

	// Orthographic
	const float width	= m_orthoSize * m_aspectRatio;
	const float height	= m_orthoSize;
	XMMATRIX ortho		= XMMatrixOrthographicLH(width, height, m_nearZ, m_farZ);
	XMStoreFloat4x4(&m_orthogonalProjMatrix, ortho);

	m_perspecticProjDirty = false;
	m_orthogonalProjDirty = false;
}

void kfe::KFECamera::MarkViewDirty() const noexcept
{
	m_viewDirty = true;
}

void kfe::KFECamera::MarkProjectionDirty() const noexcept
{
	m_perspecticProjDirty = true;
	m_orthogonalProjDirty = true;
}
