#include "pch.h"
#include "render_manager.h"

#include "engine/windows_manager/windows_manager.h"
#include "engine/utils/logger/logger.h"

//~ Components
#include "components/factory/factory.h"
#include "components/adapter/adapter.h"
#include "components/monitor/monitor.h"
#include "components/device/device.h"

//~ Memory Managements
#include "memory_management/commands/command_queue/graphics_queue/graphics_queue.h"

#pragma region IMPL

class kfe::KFERenderManager::Impl
{
public:
	Impl(KFEWindows* windows);

	bool Initialize();
	bool Release   ();

	void FrameBegin(float dt);
	void FrameEnd  ();

private:
	bool InitializeComponents();
	bool InitializeQueues	 ();

private:
	KFEWindows* m_pWindows{ nullptr };

	//~ Components
	std::unique_ptr<KFEFactory> m_pFactory{ nullptr };
	std::unique_ptr<KFEAdapter> m_pAdapter{ nullptr };
	std::unique_ptr<KFEMonitor> m_pMonitor{ nullptr };
	std::unique_ptr<KFEDevice>  m_pDevice { nullptr };

	//~ Queues
	std::unique_ptr<KFEGraphicsCmdQ> m_pGraphicsQueue{ nullptr };
};

#pragma endregion

kfe::KFERenderManager::KFERenderManager(KFEWindows* windows)
	: m_impl(std::make_shared<kfe::KFERenderManager::Impl>(windows))
{}

kfe::KFERenderManager::~KFERenderManager()
{
	if (m_impl) 
	{
		m_impl->Release();
	}
}

bool kfe::KFERenderManager::Initialize()
{
	return m_impl->Initialize();
}

bool kfe::KFERenderManager::Release()
{
	return m_impl->Release();
}

void kfe::KFERenderManager::OnFrameBegin(float deltaTime)
{
	m_impl->FrameBegin(deltaTime);
}

void kfe::KFERenderManager::OnFrameEnd()
{
	m_impl->FrameEnd();
}

std::string kfe::KFERenderManager::GetName() const noexcept
{
	return "RenderManager";
}

//~ Impl 
kfe::KFERenderManager::Impl::Impl(KFEWindows* windows)
	: m_pWindows(windows)
{
	//~ Create Components
	m_pFactory = std::make_unique<KFEFactory>();
	m_pAdapter = std::make_unique<KFEAdapter>();
	m_pMonitor = std::make_unique<KFEMonitor>();
	m_pDevice  = std::make_unique<KFEDevice> ();

	//~ Create Dx Queues
	m_pGraphicsQueue = std::make_unique<KFEGraphicsCmdQ>();
}

bool kfe::KFERenderManager::Impl::Initialize()
{
	if (!InitializeComponents())
	{
		return false;
	}

	if (!InitializeQueues())
	{
		return false;
	}

	return true;
}

bool kfe::KFERenderManager::Impl::Release()
{
	return true;
}

void kfe::KFERenderManager::Impl::FrameBegin(float dt)
{

}

void kfe::KFERenderManager::Impl::FrameEnd()
{

}

bool kfe::KFERenderManager::Impl::InitializeComponents()
{
	if (!m_pFactory->Initialize())
	{
		LOG_ERROR("Failed to Initialize Factory");
		return false;
	}

	auto strategy = std::make_unique<AdapterStrategyBestVram>();
	if (!m_pAdapter->Initialize(m_pFactory.get(), strategy.get()))
	{
		LOG_ERROR("Failed to Initialize Factory");
		return false;
	}

	if (!m_pMonitor->Initialize(m_pAdapter.get()))
	{
		LOG_ERROR("Failed to Initialize Monitor");
		return false;
	}

	KFE_DEVICE_CREATE_DESC deviceDesc{};
	deviceDesc.Adapter = m_pAdapter.get();
	deviceDesc.debugName = "KnightDxDebugger";
	deviceDesc.Factory = m_pFactory.get();
	deviceDesc.Monitor = m_pMonitor.get();

#if defined(_DEBUG) || defined(DEBUG)
	deviceDesc.Flags = EDeviceCreateFlags::EnableDebugLayer |
		EDeviceCreateFlags::EnableGPUBasedValidation |
		EDeviceCreateFlags::EnableStablePowerState;
#endif

	if (!m_pDevice->Initialize(deviceDesc))
	{
		LOG_ERROR("Failed to Initialize Device");
		return false;
	}

#if defined(_DEBUG) || defined(DEBUG)
	m_pAdapter->LogAdapters();
	m_pMonitor->LogOutputs ();
	m_pDevice-> LogDevice  ();
#endif

	LOG_SUCCESS("RenderManager: All Components initialized!");
	return true;
}

bool kfe::KFERenderManager::Impl::InitializeQueues()
{
	if (!m_pGraphicsQueue->Initialize(m_pDevice.get()))
	{
		LOG_ERROR("Failed To Initialize Graphics Queue");
		return false;
	}

	LOG_SUCCESS("RenderManager: All Queues initialized!");
	return true;
}
