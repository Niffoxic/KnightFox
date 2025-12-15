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
#include "engine/windows_manager/inputs/keyboard_inputs.h"
#include <cstring>

kfe::KFEKeyboardInput::KFEKeyboardInput() noexcept
{
    ClearAll();
}

_Use_decl_annotations_
bool kfe::KFEKeyboardInput::ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam) noexcept
{
    switch (message)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        const int key = static_cast<int>(wParam);
        if (!IsInside(key)) return false;

        if (!m_keyDown[key]) // key was pressed before 
        {
            if (!IsSetAutoRepeat(lParam))
                m_keyPressed[key] = true;
            m_keyDown[key] = true;
        }
        return true;
    }
    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        const int key = static_cast<int>(wParam);
        if (!IsInside(key)) return false;

        if (m_keyDown[key])
        {
            m_keyReleased[key] = true;
            m_keyDown[key] = false;
        }
        return true;
    }
    case WM_KILLFOCUS:
    case WM_SETFOCUS:
    {
        ClearAll();
        return false;
    }
    default:
        return false;
    }

    return false;
}

_Use_decl_annotations_
void kfe::KFEKeyboardInput::OnFrameBegin(float deltaTime) noexcept
{
}

void kfe::KFEKeyboardInput::OnFrameEnd() noexcept
{
    std::memset(m_keyPressed, 0, sizeof(m_keyPressed));
    std::memset(m_keyReleased, 0, sizeof(m_keyReleased));
}

_Use_decl_annotations_
bool kfe::KFEKeyboardInput::IsKeyPressed(int virtualKey) const noexcept
{
    return IsInside(virtualKey) ? m_keyDown[virtualKey] : false;
}

_Use_decl_annotations_
bool kfe::KFEKeyboardInput::WasKeyPressed(int virtualKey) const noexcept
{
    return IsInside(virtualKey) ? m_keyPressed[virtualKey] : false;
}

_Use_decl_annotations_
bool kfe::KFEKeyboardInput::WasKeyReleased(int virtualKey) const noexcept
{
    return IsInside(virtualKey) ? m_keyReleased[virtualKey] : false;
}

_Use_decl_annotations_
bool kfe::KFEKeyboardInput::WasChordPressed(int key, const kfe::KFEKeyboardMode& mode) const noexcept
{
    if (!WasKeyPressed(key)) return false;

    if ((mode & kfe::KFEKeyboardMode::Ctrl) && !IsCtrlPressed())  return false;
    if ((mode & kfe::KFEKeyboardMode::Shift) && !IsShiftPressed()) return false;
    if ((mode & kfe::KFEKeyboardMode::Alt) && !IsAltPressed())   return false;
    if ((mode & kfe::KFEKeyboardMode::Super) && !IsSuperPressed()) return false;

    return true;
}

_Use_decl_annotations_
bool kfe::KFEKeyboardInput::WasMultipleKeyPressed(std::initializer_list<int> keys) const noexcept
{
    bool anyPressed = false;
    for (int key : keys)
    {
        if (!IsKeyPressed(key)) return false;
        anyPressed = anyPressed || WasKeyPressed(key);
    }
    return anyPressed;
}

void kfe::KFEKeyboardInput::ClearAll() noexcept
{
    std::memset(m_keyDown, 0, sizeof(m_keyDown));
    std::memset(m_keyPressed, 0, sizeof(m_keyPressed));
    std::memset(m_keyReleased, 0, sizeof(m_keyReleased));
}

_Use_decl_annotations_
bool kfe::KFEKeyboardInput::IsSetAutoRepeat(LPARAM lParam) noexcept
{
    return (lParam & (1 << 30)) != 0;
}

_Use_decl_annotations_
bool kfe::KFEKeyboardInput::IsCtrlPressed() const noexcept
{
    return m_keyDown[VK_CONTROL] || m_keyDown[VK_LCONTROL] || m_keyDown[VK_RCONTROL];
}

_Use_decl_annotations_
bool kfe::KFEKeyboardInput::IsShiftPressed() const noexcept
{
    return m_keyDown[VK_SHIFT] || m_keyDown[VK_LSHIFT] || m_keyDown[VK_RSHIFT];
}

_Use_decl_annotations_
bool kfe::KFEKeyboardInput::IsAltPressed() const noexcept
{
    return m_keyDown[VK_MENU] || m_keyDown[VK_LMENU] || m_keyDown[VK_RMENU];
}

_Use_decl_annotations_
bool kfe::KFEKeyboardInput::IsSuperPressed() const noexcept
{
    return m_keyDown[VK_LWIN] || m_keyDown[VK_RWIN];
}

_Use_decl_annotations_
std::string kfe::KFEKeyboardInput::GetObjectName() const
{
    return "PixelEngineWindowsKeyboardInputs";
}

_Use_decl_annotations_
bool kfe::KFEKeyboardInput::Initialize()
{
    return true;
}

_Use_decl_annotations_
bool kfe::KFEKeyboardInput::Release()
{
    return true;
}
