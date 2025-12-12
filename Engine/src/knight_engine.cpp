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
#include "engine/knight_engine.h"

#include "engine/windows_manager/windows_manager.h"
#include "engine/render_manager/render_manager.h"
#include "engine/editor/editor.h"
#include "engine/utils/logger.h"

#include "engine/system/dependency_resolver.h"
#include "engine/system/exception/base_exception.h"
#include "engine/system/event_system/event_queue.h"
#include "engine/system/event_system/windows_events.h"
#include "engine/system/timer.h"

#if defined(DEBUG) || defined(_DEBUG)

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_dx12.h"
#include "imgui/imgui_impl_win32.h"
#endif

#include "engine/map/world.h"
#include "engine/render_manager/scene/cube_scene.h"

//~ test
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <filesystem>
#include <string>

#pragma region IMPL

class kfe::IKFEngine::Impl
{
public:
	 Impl() = default;
	~Impl() = default;

	_NODISCARD _Check_return_
	bool CreateManagers(_In_ const KFE_ENGINE_CREATE_DESC& desc);

	void CreateUtilities	 ();
	void SetManagerDependency();
	void SubscribeToEvents	 ();

	bool IsEnginePaused() const noexcept { return m_bEnginePaused; }

	bool Init	 ();
	bool Shutdown();

	//~ frames
	void FrameBegin(float dt);
	void FrameEnd  ();

	EProcessedMessage ProcessMessage();
	void DisplayFPS(float dt) const;

	KFETimer* GetTimer() const;
	KFEWorld* GetWorld() const noexcept;

private:
	std::unique_ptr<KFEWindows>		  m_pWindowsManager{ nullptr };
	std::unique_ptr<KFERenderManager> m_pRendeManager  { nullptr };
	std::unique_ptr<KFEWorld>		  m_pWorld		   { nullptr };
	
#if defined(_DEBUG) || defined(DEBUG)
	std::unique_ptr<KFEEditor> m_pEditor{ nullptr };
#endif
	//~ Tools and Utilities
	std::unique_ptr<KFETimer>	m_pTimer		 { nullptr };

	//~ configurations
	bool			   m_bEnginePaused{ false };
	DependencyResolver m_dependecyResolver{};
};

#pragma endregion

_Use_decl_annotations_
kfe::IKFEngine::IKFEngine(const KFE_ENGINE_CREATE_DESC& desc)
	: m_impl(std::make_shared<kfe::IKFEngine::Impl>())
{
	if (!m_impl->CreateManagers(desc))
	{
		LOG_ERROR("Failed to Create Managers!");
	}

	m_impl->CreateUtilities		();
	m_impl->SetManagerDependency();
	m_impl->SubscribeToEvents	();
}

kfe::IKFEngine::~IKFEngine()
{
	if (!m_impl->Shutdown())
	{
		LOG_ERROR("Failed to shutdown smoothly!");
	}
#if defined(DEBUG) || defined(_DEBUG)
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif
}

_Use_decl_annotations_
bool kfe::IKFEngine::Init()
{
	if (!m_impl->Init())
	{
		LOG_ERROR("Failed to initialize manager!");
		THROW_MSG("Failed to initialize manager!");
		return false;
	}

	if (!InitApplication())
	{
		LOG_ERROR("Failed to initialize application!");
		THROW_MSG("Failed to initialize application!");
		return false;
	}

	return true;
}

_Use_decl_annotations_
int kfe::IKFEngine::Execute()
{
	LOG_INFO("Starting Game Loop!");
	BeginPlay();

	while (true)
	{
		float dt = m_impl->GetTimer()->Tick();
		
		if (m_impl->IsEnginePaused())
		{
			dt = 0.0f;
		}

		auto msg = m_impl->ProcessMessage();

		if (msg == EProcessedMessage::QuitInvoked)
		{
			LOG_INFO("Closing Application!");
			m_impl->Shutdown();
			return 0;
		}
		m_impl->FrameBegin(dt);
		Tick(dt);
		m_impl->FrameEnd();

#if defined(DEBUG) || defined(_DEBUG)
		m_impl->DisplayFPS(dt);
#endif
		EventQueue::DispatchAll();
	}
	return 0;
}

kfe::KFEWorld* kfe::IKFEngine::GetWorld() const
{
	return m_impl->GetWorld();
}

//~ IMP Implementation

_Use_decl_annotations_
bool kfe::IKFEngine::Impl::CreateManagers(const KFE_ENGINE_CREATE_DESC& desc)
{
	m_pWindowsManager = std::make_unique<KFEWindows>(desc.WindowsDesc);
	m_pRendeManager   = std::make_unique<KFERenderManager>(m_pWindowsManager.get());
	return true;
}

void kfe::IKFEngine::Impl::CreateUtilities()
{
#ifdef _DEBUG
	KFE_LOGGER_CREATE_DESC logDesc{};
	logDesc.LogPrefix = "GameLog";
	logDesc.EnableTerminal = true;
	logDesc.LogPath = "logs";
	INIT_GLOBAL_LOGGER(&logDesc);
#endif

#ifdef _DEBUG
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui::StyleColorsDark();

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}
#endif

	m_pWorld = std::make_unique<KFEWorld>();

#if defined(_DEBUG) || defined(DEBUG)
	m_pEditor = std::make_unique<KFEEditor>(m_pWorld.get());
#endif

	m_pTimer = std::make_unique<KFETimer>();
}

void kfe::IKFEngine::Impl::SetManagerDependency()
{
	//~ Core Managers
	m_dependecyResolver.Register(m_pWindowsManager.get());
	m_dependecyResolver.Register(m_pRendeManager.get());

	//~ Render Dependecy
	m_dependecyResolver.AddDependency(
		m_pRendeManager.get(),
		m_pWindowsManager.get());

#if defined(_DEBUG) || defined(DEBUG)
	//~ Editor Dependecies
	m_dependecyResolver.Register(m_pEditor.get());
	m_dependecyResolver.AddDependency(m_pEditor.get(),
		m_pRendeManager.get());
	m_dependecyResolver.AddDependency(m_pEditor.get(),
		m_pWindowsManager.get());
#endif

}

void kfe::IKFEngine::Impl::SubscribeToEvents()
{
	auto token = EventQueue::Subscribe<KFE_WINDOW_PAUSE_EVENT>(
		[&](const KFE_WINDOW_PAUSE_EVENT& event)
		{
			if (event.Paused) m_bEnginePaused = true;
			else
			{
				m_bEnginePaused = false;
				m_pTimer->ResetTime();
			}
			LOG_INFO("Window Drag Event Recevied with {}", event.Paused);
		});
}

bool kfe::IKFEngine::Impl::Init()
{
	if (not m_dependecyResolver.Init())
	{
		LOG_ERROR("Failed to initialize manager!");
		THROW_MSG("Failed to initialize manager!");
		return false;
	}

	return true;
}

bool kfe::IKFEngine::Impl::Shutdown()
{
	if (not m_dependecyResolver.Shutdown())
	{
		LOG_ERROR("Failure Detected at the time of deleting managers!");
		return false;
	}
	return true;
}

void kfe::IKFEngine::Impl::FrameBegin(float dt)
{
	m_dependecyResolver.UpdateLoopStart(dt);
}

void kfe::IKFEngine::Impl::FrameEnd()
{
	m_dependecyResolver.UpdateLoopEnd();
}

kfe::EProcessedMessage kfe::IKFEngine::Impl::ProcessMessage()
{
	return m_pWindowsManager->ProcessMessage();
}

void kfe::IKFEngine::Impl::DisplayFPS(float dt) const
{
	static float passed = 0.0f;
	static int   frame = 0;
	static float avg_frames = 0.0f;
	static float last_time_elapsed = 0.0f;

	frame++;
	passed += dt;

	if (passed >= 1.0f)
	{
		avg_frames += frame;
		last_time_elapsed = m_pTimer->TimeElapsed();

		std::string message =
			"Time Elapsed: " +
			std::to_string(last_time_elapsed) +
			" Frame Rate: " +
			std::to_string(frame) +
			" per second (Avg = " +
			std::to_string(avg_frames / last_time_elapsed) +
			")";

		m_pWindowsManager->SetWindowMessageOnTitle(message);

		passed = 0.0f;
		frame = 0;
	}
}

kfe::KFETimer* kfe::IKFEngine::Impl::GetTimer() const
{
	return m_pTimer.get();
}

kfe::KFEWorld* kfe::IKFEngine::Impl::GetWorld() const noexcept
{
	return m_pWorld.get();
}
