#include "pch.h"
#include "knight_engine.h"

#include "windows_manager/windows_manager.h"
#include "utils/logger/logger.h"

#include "core/dependency/dependency_resolver.h"
#include "core/exception/base_exception.h"
#include "core/event_system/queue/event_queue.h"
#include "core/event_system/events/windows_events.h"
#include "core/time/timer.h"

#include <memory>

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

private:
	std::unique_ptr<KFEWindows> m_pWindowsManager{ nullptr };
	std::unique_ptr<KFETimer>	m_pTimer		 { nullptr };

	bool			   m_bEnginePaused{ false };
	DependencyResolver m_dependecyResolver{};
};

bool kfe::IKFEngine::Impl::CreateManagers(const KFE_ENGINE_CREATE_DESC& desc)
{
	m_pWindowsManager = std::make_unique<KFEWindows>(desc.WindowsDesc);
	return true;
}

void kfe::IKFEngine::Impl::CreateUtilities()
{
#ifdef _DEBUG
	KFE_LOGGER_CREATE_DESC logDesc{};
	logDesc.LogPrefix		= "SweetLog";
	logDesc.EnableTerminal	= true;
	logDesc.LogPath			= "logs";
	INIT_GLOBAL_LOGGER(&logDesc);
#endif

	m_pTimer = std::make_unique<KFETimer>();
}

void kfe::IKFEngine::Impl::SetManagerDependency()
{
	m_dependecyResolver.Register(m_pWindowsManager.get());
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
	gLogger->Close();
#endif
}

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

HRESULT kfe::IKFEngine::Execute()
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
			return S_OK;
		}
		m_impl->FrameBegin(dt);
		Tick(dt);
		m_impl->FrameEnd();

#if defined(DEBUG) || defined(_DEBUG)
		m_impl->DisplayFPS(dt);
#endif
		EventQueue::DispatchAll();
	}
	return S_OK;
}
