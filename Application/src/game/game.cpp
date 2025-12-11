#include "game.h"

#include "engine/map/world.h"
#include "engine/render_manager/scene/cube_scene.h"

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
}

bool GameApplication::InitApplication()
{
	return true;
}

void GameApplication::BeginPlay()
{
	kfe::KFEWorld* world = GetWorld();
	auto data = JsonLoader{};
	data.Load("world/data.json");
	world->LoadSceneData(data);
}

void GameApplication::Release()
{
}

void GameApplication::Tick(float deltaTime)
{
}
