#pragma once

#include <engine/knight_engine.h>

class GameApplication final : public kfe::IKFEngine
{
public:
	GameApplication(_In_ const kfe::KFE_ENGINE_CREATE_DESC& desc);
	~GameApplication() override;

	bool InitApplication() override;
	void BeginPlay		() override;
	void Release		() override;

	void Tick(float deltaTime) override;

private:
	float m_saveCooldown = 0.0f;
	const float m_saveMaxCooldown = 0.75f;
};
