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

}

bool GameApplication::InitApplication()
{
	return true;
}

void GameApplication::BeginPlay()
{

	kfe::KFEWorld* world = GetWorld();

	auto cube = std::make_unique<kfe::KEFCubeSceneObject>();
	cube->SetPosition({ -5.f, 0.f, 0.f });
	world->AddSceneObject(std::move(cube));
}

void GameApplication::Release()
{
}

void GameApplication::Tick(float deltaTime)
{
}
