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
#include "engine/render_manager/scene/interface_scene.h"

#include <DirectXMath.h>

using namespace DirectX;

void kfe::IKFESceneObject::SetVisible(bool visible) noexcept
{
	m_bVisible = visible;
}

_Use_decl_annotations_
bool kfe::IKFESceneObject::IsVisible() const noexcept
{
	return m_bVisible;
}

_Use_decl_annotations_
XMFLOAT3 kfe::IKFESceneObject::GetPosition() const noexcept
{
	return m_position;
}

_Use_decl_annotations_
void kfe::IKFESceneObject::SetPosition(const XMFLOAT3& position) noexcept
{
	m_position = position;
	SetDirty();
}

_Use_decl_annotations_
void kfe::IKFESceneObject::AddPosition(const XMFLOAT3& delta) noexcept
{
	m_position.x += delta.x;
	m_position.y += delta.y;
	m_position.z += delta.z;
	SetDirty();
}

void kfe::IKFESceneObject::AddPositionX(float x) noexcept
{
	m_position.x += x;
	SetDirty();
}

void kfe::IKFESceneObject::AddPositionY(float y) noexcept
{
	m_position.y += y;
	SetDirty();
}

void kfe::IKFESceneObject::AddPositionZ(float z) noexcept
{
	m_position.z += z;
	SetDirty();
}

_Use_decl_annotations_
XMFLOAT4 kfe::IKFESceneObject::GetOrientation() const noexcept
{
	return m_orientation;
}

_Use_decl_annotations_
void kfe::IKFESceneObject::SetOrientation(const XMFLOAT4& orientation) noexcept
{
	m_orientation = orientation;
	SetDirty();
}

void kfe::IKFESceneObject::SetOrientationFromEuler(float pitch, float yaw, float roll) noexcept
{
	const XMVECTOR q = XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);
	XMStoreFloat4(&m_orientation, q);
	SetDirty();
}

void kfe::IKFESceneObject::AddPitch(float pitch) noexcept
{
	const XMVECTOR current = XMLoadFloat4(&m_orientation);
	const XMVECTOR delta = XMQuaternionRotationRollPitchYaw(pitch, 0.0f, 0.0f);

	XMVECTOR result = XMQuaternionMultiply(delta, current);
	result = XMQuaternionNormalize(result);

	XMStoreFloat4(&m_orientation, result);
	SetDirty();
}

void kfe::IKFESceneObject::AddYaw(float yaw) noexcept
{
	const XMVECTOR current = XMLoadFloat4(&m_orientation);
	const XMVECTOR delta = XMQuaternionRotationRollPitchYaw(0.0f, yaw, 0.0f);

	XMVECTOR result = XMQuaternionMultiply(delta, current);
	result = XMQuaternionNormalize(result);

	XMStoreFloat4(&m_orientation, result);
	SetDirty();
}

void kfe::IKFESceneObject::AddRoll(float roll) noexcept
{
	const XMVECTOR current = XMLoadFloat4(&m_orientation);
	const XMVECTOR delta = XMQuaternionRotationRollPitchYaw(0.0f, 0.0f, roll);

	XMVECTOR result = XMQuaternionMultiply(delta, current);
	result = XMQuaternionNormalize(result);

	XMStoreFloat4(&m_orientation, result);
	SetDirty();
}

_Use_decl_annotations_
XMFLOAT3 kfe::IKFESceneObject::GetScale() const noexcept
{
	return m_scale;
}

_Use_decl_annotations_
void kfe::IKFESceneObject::SetScale(const XMFLOAT3& scale) noexcept
{
	m_scale = scale;
	SetDirty();
}

void kfe::IKFESceneObject::SetUniformScale(float s) noexcept
{
	m_scale = XMFLOAT3(s, s, s);
	SetDirty();
}

_Use_decl_annotations_
void kfe::IKFESceneObject::AddScale(const XMFLOAT3& delta) noexcept
{
	m_scale.x += delta.x;
	m_scale.y += delta.y;
	m_scale.z += delta.z;
	SetDirty();
}

void kfe::IKFESceneObject::AddScaleX(float x) noexcept
{
	m_scale.x += x;
	SetDirty();
}

void kfe::IKFESceneObject::AddScaleY(float y) noexcept
{
	m_scale.y += y;
	SetDirty();
}

void kfe::IKFESceneObject::AddScaleZ(float z) noexcept
{
	m_scale.z += z;
	SetDirty();
}

_Use_decl_annotations_
const XMMATRIX& kfe::IKFESceneObject::GetWorldMatrix() const noexcept
{
	if (IsDirty())
	{
		const XMVECTOR S = XMLoadFloat3(&m_scale);
		const XMVECTOR R = XMLoadFloat4(&m_orientation);
		const XMVECTOR T = XMLoadFloat3(&m_position);

		auto* self = const_cast<IKFESceneObject*>(this);
		self->m_transform = XMMatrixAffineTransformation(S, XMVectorZero(), R, T);

		ResetDirty();
	}

	return m_transform;
}

_Use_decl_annotations_
bool kfe::IKFESceneObject::IsDirty() const noexcept
{
	return m_bDirty;
}

void kfe::IKFESceneObject::SetDirty() const noexcept
{
	m_bDirty = true;
}

void kfe::IKFESceneObject::ResetDirty() const noexcept
{
	m_bDirty = false;
}

_Use_decl_annotations_
bool kfe::IKFESceneObject::IsInitialized() const noexcept
{
	return m_bInitialized;
}