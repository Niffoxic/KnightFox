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
#include "engine/system/interface/interface_scene.h"

#include "imgui/imgui.h"
#include <DirectXMath.h>

using namespace DirectX;

void kfe::IKFESceneObject::SetVisible(bool visible)
{
    m_bVisible = visible;
}

_Use_decl_annotations_
bool kfe::IKFESceneObject::IsVisible() const
{
    return m_bVisible;
}

_Use_decl_annotations_
XMFLOAT3 kfe::IKFESceneObject::GetPosition() const
{
    return m_position;
}

_Use_decl_annotations_
void kfe::IKFESceneObject::SetPosition(const XMFLOAT3& position)
{
    m_position = position;
    SetDirty();
}

_Use_decl_annotations_
void kfe::IKFESceneObject::AddPosition(const XMFLOAT3& delta)
{
    m_position.x += delta.x;
    m_position.y += delta.y;
    m_position.z += delta.z;
    SetDirty();
}

void kfe::IKFESceneObject::AddPositionX(float x)
{
    m_position.x += x;
    SetDirty();
}

void kfe::IKFESceneObject::AddPositionY(float y)
{
    m_position.y += y;
    SetDirty();
}

void kfe::IKFESceneObject::AddPositionZ(float z)
{
    m_position.z += z;
    SetDirty();
}

_Use_decl_annotations_
XMFLOAT4 kfe::IKFESceneObject::GetOrientation() const
{
    return m_orientation;
}

_Use_decl_annotations_
void kfe::IKFESceneObject::SetOrientation(const XMFLOAT4& orientation)
{
    m_orientation = orientation;
    SetDirty();
}

void kfe::IKFESceneObject::SetOrientationFromEuler(float pitch, float yaw, float roll)
{
    const XMVECTOR q = XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);
    XMStoreFloat4(&m_orientation, q);
    SetDirty();
}

void kfe::IKFESceneObject::AddPitch(float pitch)
{
    const XMVECTOR current = XMLoadFloat4(&m_orientation);
    const XMVECTOR delta = XMQuaternionRotationRollPitchYaw(pitch, 0.0f, 0.0f);

    XMVECTOR result = XMQuaternionMultiply(delta, current);
    result = XMQuaternionNormalize(result);

    XMStoreFloat4(&m_orientation, result);
    SetDirty();
}

void kfe::IKFESceneObject::AddYaw(float yaw)
{
    const XMVECTOR current = XMLoadFloat4(&m_orientation);
    const XMVECTOR delta = XMQuaternionRotationRollPitchYaw(0.0f, yaw, 0.0f);

    XMVECTOR result = XMQuaternionMultiply(delta, current);
    result = XMQuaternionNormalize(result);

    XMStoreFloat4(&m_orientation, result);
    SetDirty();
}

void kfe::IKFESceneObject::AddRoll(float roll)
{
    const XMVECTOR current = XMLoadFloat4(&m_orientation);
    const XMVECTOR delta = XMQuaternionRotationRollPitchYaw(0.0f, 0.0f, roll);

    XMVECTOR result = XMQuaternionMultiply(delta, current);
    result = XMQuaternionNormalize(result);

    XMStoreFloat4(&m_orientation, result);
    SetDirty();
}

_Use_decl_annotations_
XMFLOAT3 kfe::IKFESceneObject::GetScale() const
{
    return m_scale;
}

_Use_decl_annotations_
void kfe::IKFESceneObject::SetScale(const XMFLOAT3& scale)
{
    m_scale = scale;
    SetDirty();
}

void kfe::IKFESceneObject::SetUniformScale(float s)
{
    m_scale = XMFLOAT3(s, s, s);
    SetDirty();
}

_Use_decl_annotations_
void kfe::IKFESceneObject::AddScale(const XMFLOAT3& delta)
{
    m_scale.x += delta.x;
    m_scale.y += delta.y;
    m_scale.z += delta.z;
    SetDirty();
}

void kfe::IKFESceneObject::AddScaleX(float x)
{
    m_scale.x += x;
    SetDirty();
}

void kfe::IKFESceneObject::AddScaleY(float y)
{
    m_scale.y += y;
    SetDirty();
}

void kfe::IKFESceneObject::AddScaleZ(float z)
{
    m_scale.z += z;
    SetDirty();
}

_Use_decl_annotations_
const XMMATRIX& kfe::IKFESceneObject::GetWorldMatrix() const
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

JsonLoader kfe::IKFESceneObject::GetTransformJsonData() const noexcept
{
    JsonLoader root{};

    // Position
    {
        JsonLoader& posNode = root["Position"];
        posNode["x"] = std::to_string(m_position.x);
        posNode["y"] = std::to_string(m_position.y);
        posNode["z"] = std::to_string(m_position.z);
    }

    // Orientation (quaternion: x, y, z, w)
    {
        JsonLoader& rotNode = root["Orientation"];
        rotNode["x"] = std::to_string(m_orientation.x);
        rotNode["y"] = std::to_string(m_orientation.y);
        rotNode["z"] = std::to_string(m_orientation.z);
        rotNode["w"] = std::to_string(m_orientation.w);
    }

    // Scale
    {
        JsonLoader& scaleNode = root["Scale"];
        scaleNode["x"] = std::to_string(m_scale.x);
        scaleNode["y"] = std::to_string(m_scale.y);
        scaleNode["z"] = std::to_string(m_scale.z);
    }

    return root;
}

void kfe::IKFESceneObject::LoadTransformFromJson(const JsonLoader& loader) noexcept
{
    DirectX::XMFLOAT3 position = m_position;
    DirectX::XMFLOAT4 orientation = m_orientation;
    DirectX::XMFLOAT3 scale = m_scale;

    if (loader.Contains("Position"))
    {
        const JsonLoader& posNode = loader["Position"];

        if (posNode.Contains("x"))
            position.x = posNode["x"].AsFloat(position.x);
        if (posNode.Contains("y"))
            position.y = posNode["y"].AsFloat(position.y);
        if (posNode.Contains("z"))
            position.z = posNode["z"].AsFloat(position.z);
    }

    if (loader.Contains("Orientation"))
    {
        const JsonLoader& rotNode = loader["Orientation"];

        if (rotNode.Contains("x"))
            orientation.x = rotNode["x"].AsFloat(orientation.x);
        if (rotNode.Contains("y"))
            orientation.y = rotNode["y"].AsFloat(orientation.y);
        if (rotNode.Contains("z"))
            orientation.z = rotNode["z"].AsFloat(orientation.z);
        if (rotNode.Contains("w"))
            orientation.w = rotNode["w"].AsFloat(orientation.w);
    }

    if (loader.Contains("Scale"))
    {
        const JsonLoader& scaleNode = loader["Scale"];

        if (scaleNode.Contains("x"))
            scale.x = scaleNode["x"].AsFloat(scale.x);
        if (scaleNode.Contains("y"))
            scale.y = scaleNode["y"].AsFloat(scale.y);
        if (scaleNode.Contains("z"))
            scale.z = scaleNode["z"].AsFloat(scale.z);
    }

    SetPosition(position);
    SetOrientation(orientation);
    SetScale(scale);
}

void kfe::IKFESceneObject::ImguiTransformView(float)
{
    if (!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    DirectX::XMFLOAT3 pos = GetPosition();
    if (ImGui::DragFloat3("Position", &pos.x, 0.1f))
    {
        SetPosition(pos);
    }

    DirectX::XMFLOAT4 ori = GetOrientation();
    float q[4] = { ori.x, ori.y, ori.z, ori.w };

    if (ImGui::DragFloat4("Rotation (quat)", q, 0.01f))
    {
        DirectX::XMVECTOR qv = DirectX::XMVectorSet(q[0], q[1], q[2], q[3]);
        qv = DirectX::XMQuaternionNormalize(qv);
        DirectX::XMStoreFloat4(&ori, qv);
        SetOrientation(ori);
    }

    DirectX::XMFLOAT3 scl = GetScale();
    if (ImGui::DragFloat3("Scale", &scl.x, 0.01f))
    {
        SetScale(scl);
    }

    ImGui::TextDisabled("Tip: Rotation is edited as a normalized quaternion.");
}
