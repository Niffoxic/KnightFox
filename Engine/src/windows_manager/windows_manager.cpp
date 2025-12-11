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
#include "engine/windows_manager/windows_manager.h"

#include "engine/system/exception/win_exception.h"
#include "engine/system/event_system/event_queue.h"
#include "engine/system/event_system/windows_events.h"

#include "engine/utils/helpers.h"
#include "engine/utils/logger.h"

#if defined(DEBUG) || defined(_DEBUG)
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"
#endif

#pragma region IMPL

class kfe::KFEWindows::Impl
{
public:
    Impl(KFEWindows* windows)
        : m_pWindows(windows)
    {}
	Impl(KFEWindows* windows, _In_ const KFE_WINDOW_CREATE_DESC& desc);

	_NODISCARD _Check_return_ _Must_inspect_result_
	_Success_(return != 0)
	bool InitWindowScreen();

	_NODISCARD
	LRESULT MessageHandler(
		_In_ HWND   hwnd,
		_In_ UINT   msg,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam);

	_Function_class_(WINDOWS_CALLBACK)
	static LRESULT CALLBACK WindowProcSetup(
		_In_ HWND   hwnd,
		_In_ UINT   msg,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam);

	_Function_class_(WINDOWS_CALLBACK)
	static LRESULT CALLBACK WindowProcThunk(
		_In_ HWND   hwnd,
		_In_ UINT   msg,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam);

	void TransitionToFullScreen	   ();
	void TransitionToWindowedScreen() const;

	std::string		m_szWindowTitle	  { "Fox Knight The Game" };
	std::string		m_szClassName	  { "FoxKnightEngine" };
	UINT			m_nWindowsWidth   { 0u };
	UINT			m_nWindowsHeight  { 0u };
	HWND			m_pWindowsHandle  { nullptr };
	HINSTANCE		m_pWindowsInstance{ nullptr };
	EScreenState	m_eScreenState	  { EScreenState::Windowed };
	WINDOWPLACEMENT m_WindowPlacement { sizeof(m_WindowPlacement) };
	UINT			m_nIconID		  { 0u };
    KFEWindows*     m_pWindows        { nullptr };
};

#pragma endregion

kfe::KFEWindows::KFEWindows()
    : m_impl(std::make_unique<KFEWindows::Impl>(this))
{}

_Use_decl_annotations_
kfe::KFEWindows::KFEWindows(const KFE_WINDOW_CREATE_DESC& desc)
    : m_impl(std::make_unique<KFEWindows::Impl>(this, desc))
{}

kfe::KFEWindows::~KFEWindows()
{
    if (not Release())
    {
        LOG_ERROR("Failed to Release Windows Resources Successfully!");
    }
}

_Use_decl_annotations_
kfe::EProcessedMessage kfe::KFEWindows::ProcessMessage() const noexcept
{
    MSG message{};

    while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
    {
        if (message.message == WM_QUIT) return EProcessedMessage::QuitInvoked;

        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return EProcessedMessage::Alive;
}

_Use_decl_annotations_
bool kfe::KFEWindows::Initialize()
{
    if (!m_impl->InitWindowScreen())
    {
        LOG_ERROR("Failed to Initialize Window!");
        return false;
    }
    if (auto handle = GetWindowsHandle())
        Mouse.AttachWindowHandle(handle);
    LOG_SUCCESS("Initialized Window!");

#if defined(DEBUG) || defined(_DEBUG)
    ImGui_ImplWin32_Init(GetWindowsHandle());
    LOG_SUCCESS("Initialzied Imgui For windows");
#endif

	return true;
}

_Use_decl_annotations_
bool kfe::KFEWindows::Release()
{
	return true;
}

_Use_decl_annotations_
void kfe::KFEWindows::OnFrameBegin(float deltaTime)
{
    Keyboard.OnFrameBegin(deltaTime);
    Mouse   .OnFrameBegin(deltaTime);
}

void kfe::KFEWindows::OnFrameEnd()
{
    Keyboard.OnFrameEnd();
    Mouse   .OnFrameEnd();
}

_Use_decl_annotations_
HWND kfe::KFEWindows::GetWindowsHandle() const
{
    auto handle = m_impl->m_pWindowsHandle;

    if (!handle) 
    {
        THROW_MSG("Indeed the error is here!");
    }

    return handle;
}

_Use_decl_annotations_
HINSTANCE kfe::KFEWindows::GetWindowsInstance() const
{
	return m_impl->m_pWindowsInstance;
}

_Use_decl_annotations_
bool kfe::KFEWindows::IsFullScreen() const
{
	return m_impl->m_eScreenState == EScreenState::FullScreen;
}

_Use_decl_annotations_
void kfe::KFEWindows::SetScreenState(EScreenState state)
{
    if (state == m_impl->m_eScreenState) return; // already done
    m_impl->m_eScreenState = state;

    if (state == EScreenState::FullScreen) m_impl->TransitionToFullScreen();
    else m_impl->TransitionToWindowedScreen();

    auto handle = m_impl->m_pWindowsHandle;
    if (!handle) return;
    UpdateWindow(handle);
    
    RECT rt{};
    GetClientRect(handle, &rt);
    UINT width  = rt.right  - rt.left;
    UINT height = rt.bottom - rt.top;

    if (state == EScreenState::FullScreen)
    {
        EventQueue::Post<KFE_FULL_SCREEN_EVENT>({ width, height });
    }
    else
    {
        EventQueue::Post<KFE_WINDOWED_SCREEN_EVENT>({ width, height });
    }
}

_Use_decl_annotations_
KFE_WinSizeU kfe::KFEWindows::GetWinSize() const
{
    return { m_impl->m_nWindowsWidth, m_impl->m_nWindowsHeight };
}

_Use_decl_annotations_
void kfe::KFEWindows::SetWindowTitle(const std::string& title)
{
    if (auto handle = GetWindowsHandle())
    {
        m_impl->m_szWindowTitle = title;
        auto wstr               = kfe_helpers::AnsiToWide(title);
        SetWindowText(handle, wstr.c_str());
    }
}

_Use_decl_annotations_
void kfe::KFEWindows::SetWindowMessageOnTitle(const std::string& message) const
{
    if (auto handle = GetWindowsHandle())
    {
        std::string convert = m_impl->m_szWindowTitle + " " + message;
        auto wstr = kfe_helpers::AnsiToWide(convert);
        SetWindowText(handle, wstr.c_str());
    }
}

//~ KFE IMPL
_Use_decl_annotations_
kfe::KFEWindows::Impl::Impl(KFEWindows* windows, const KFE_WINDOW_CREATE_DESC& desc)
    : m_pWindows(windows)
{
    m_nWindowsHeight = desc.Height;
    m_nWindowsWidth  = desc.Width;
    m_szWindowTitle  = desc.WindowTitle;
    m_nIconID        = desc.IconId;
    m_eScreenState   = desc.ScreenState;
}

_Use_decl_annotations_
bool kfe::KFEWindows::Impl::InitWindowScreen()
{
    m_pWindowsInstance = GetModuleHandle(nullptr);

    WNDCLASSEX wc{};
    wc.cbSize       = sizeof(WNDCLASSEX);
    wc.style        = CS_OWNDC;
    wc.lpfnWndProc  = WindowProcSetup;
    wc.cbClsExtra   = 0;
    wc.cbWndExtra   = sizeof(LONG_PTR);
    wc.hInstance    = m_pWindowsInstance;

    //~ Set Icon
    if (m_nIconID)
    {
        wc.hIcon   = LoadIcon(m_pWindowsInstance, MAKEINTRESOURCE(m_nIconID));
        wc.hIconSm = LoadIcon(m_pWindowsInstance, MAKEINTRESOURCE(m_nIconID));
    }
    else
    {
        wc.hIcon   = LoadIcon(nullptr, IDI_APPLICATION);
        wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    }
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName  = nullptr;
    auto wstr        = kfe_helpers::AnsiToWide(m_szWindowTitle);
    wc.lpszClassName = wstr.c_str();

    if (!RegisterClassEx(&wc))
    {
        THROW_WIN();
        return false;
    }

    DWORD style = WS_OVERLAPPEDWINDOW;

    RECT rect
    { 0, 0,
      static_cast<LONG>(m_nWindowsWidth),
      static_cast<LONG>(m_nWindowsHeight)
    };

    if (!AdjustWindowRect(&rect, style, FALSE))
    {
        THROW_WIN("Failed to adjust windows rect!");
        return false;
    }

    int adjustedWidth  = rect.right - rect.left;
    int adjustedHeight = rect.bottom - rect.top;

    m_pWindowsHandle = CreateWindowEx(
        0,
        wc.lpszClassName,
        wstr.c_str(),
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        adjustedWidth, adjustedHeight,
        nullptr,
        nullptr,
        m_pWindowsInstance,
        this);

    if (!m_pWindowsHandle)
    {
        THROW_WIN();
        return false;
    }

    ShowWindow  (m_pWindowsHandle, SW_SHOW);
    UpdateWindow(m_pWindowsHandle);

    return true;
}

#if defined(DEBUG) || defined(_DEBUG)
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

_Use_decl_annotations_
LRESULT kfe::KFEWindows::Impl::MessageHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
#if defined(DEBUG) || defined(_DEBUG)
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return S_OK;
#endif
    if (m_pWindows->Keyboard.ProcessMessage(msg, wParam, lParam)) return S_OK;
    if (m_pWindows->Mouse.ProcessMessage(msg, wParam, lParam))    return S_OK;

    switch (msg)
    {
    case WM_SIZE:
    {
        m_nWindowsWidth  = LOWORD(lParam);
        m_nWindowsHeight = HIWORD(lParam);
        EventQueue::Post<KFE_WINDOW_RESIZED_EVENT>({ m_nWindowsWidth, m_nWindowsHeight });
        return S_OK;
    }
    case WM_ENTERSIZEMOVE: // clicked mouse on title bar
    case WM_KILLFOCUS:
    {
        EventQueue::Post<KFE_WINDOW_PAUSE_EVENT>({ true });
        return S_OK;
    }
    case WM_EXITSIZEMOVE: // not clicking anymore
    case WM_SETFOCUS:
    {
        EventQueue::Post<KFE_WINDOW_PAUSE_EVENT>({ false });
        return S_OK;
    }
    case WM_CLOSE:
    {
        PostQuitMessage(0);
        return S_OK;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return S_OK;
}

_Use_decl_annotations_
LRESULT kfe::KFEWindows::Impl::WindowProcSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCT* create   = reinterpret_cast<CREATESTRUCT*>(lParam);
        KFEWindows::Impl* that = reinterpret_cast<KFEWindows::Impl*>(create->lpCreateParams);

        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(that));
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WindowProcThunk));

        return that->MessageHandler(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

_Use_decl_annotations_
LRESULT kfe::KFEWindows::Impl::WindowProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (auto that = reinterpret_cast<KFEWindows::Impl*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)))
    {
        return that->MessageHandler(hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void kfe::KFEWindows::Impl::TransitionToFullScreen()
{
    if (m_eScreenState != EScreenState::FullScreen) return;

    auto handle = m_pWindowsHandle;
    if (!handle) return;
    GetWindowPlacement(handle, &m_WindowPlacement);

    SetWindowLong(handle, GWL_STYLE, WS_POPUP);
    SetWindowPos(
        handle,
        HWND_TOP,
        0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        SWP_FRAMECHANGED | SWP_SHOWWINDOW
    );
}

void kfe::KFEWindows::Impl::TransitionToWindowedScreen() const
{
    if (m_eScreenState != EScreenState::Windowed) return;

    auto handle = m_pWindowsHandle;
    if (!handle) return;

    SetWindowLong(handle, GWL_STYLE, WS_OVERLAPPEDWINDOW);
    SetWindowPlacement(handle, &m_WindowPlacement);
    SetWindowPos
    (
        handle,
        nullptr,
        0, 0,
        0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW
    );
}
