#include "game.h"

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
}

void GameApplication::Release()
{
}

void GameApplication::Tick(float deltaTime)
{
}
