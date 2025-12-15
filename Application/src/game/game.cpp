#include "game.h"

#include "engine/map/world.h"
#include "engine/render_manager/scene/cube_scene.h"

#include <filesystem>
#include <string>
#include <format>

#include "engine/utils/logger.h"

namespace 
{
	namespace fs = std::filesystem;

	static fs::path MakeUniqueSnapshotDir(const fs::path& rootDir)
	{
		fs::create_directories(rootDir);

		fs::path candidate = rootDir / "save";
		if (!fs::exists(candidate))
			return candidate;

		for (std::uint32_t i = 1; i < 10'000; ++i)
		{
			candidate = rootDir / std::format("save-{}", i);
			if (!fs::exists(candidate))
				return candidate;
		}

		return rootDir / "save-overflow";
	}

	static void SaveWorldSnapshot(kfe::KFEWorld* world)
	{
		if (!world)
			return;

		const fs::path root = "saved_map";
		const fs::path snapshotDir = MakeUniqueSnapshotDir(root);

		fs::create_directories(snapshotDir);

		{
			auto data = world->GetSceneData();
			data.Save((snapshotDir / "data.json").string());
		}

		{
			auto light = world->GetLightData();
			light.Save((snapshotDir / "light.json").string());
		}
	}

} // namespace none

_Use_decl_annotations_
GameApplication::GameApplication(const kfe::KFE_ENGINE_CREATE_DESC& desc)
: kfe::IKFEngine(desc)
{
}

GameApplication::~GameApplication()
{
	kfe::KFEWorld* world = GetWorld();
	auto data = world->GetSceneData();
	data.Save("world/data.json");
	auto lightData = world->GetLightData();
	lightData.Save("world/light.json");
}

bool GameApplication::InitApplication()
{
	return true;
}

void GameApplication::BeginPlay()
{
	kfe::KFEWorld* world = GetWorld();
	auto data = JsonLoader{};
	auto lightData = JsonLoader{};
	data.Load("world/data.json");

	if (data.IsValid()) world->LoadSceneData(data);

	lightData.Load("world/light.json");
	if (lightData.IsValid())
	{
		world->LoadLightData(lightData);
	}
}

void GameApplication::Release()
{
}

void GameApplication::Tick(float deltaTime)
{
	kfe::KFEWindows* windows = GetWindows();
	if (!windows) return;

	if (m_saveCooldown > 0.0f)
		m_saveCooldown -= deltaTime;

	auto& keyboard = windows->Keyboard;

	const bool doSave = keyboard.WasChordPressed('S', kfe::KFEKeyboardMode::Ctrl);

	if (doSave && m_saveCooldown <= 0.0f)
	{
		SaveWorldSnapshot(GetWorld());
		m_saveCooldown = m_saveMaxCooldown;
		LOG_SUCCESS("Took level snapshot!");
	}
}
