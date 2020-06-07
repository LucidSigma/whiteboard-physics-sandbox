#pragma once

#include <memory>
#include <string>

#include <box2d/box2d.h>
#include <SDL2/SDL.h>

class Ball
{
private:
	static constexpr float s_Radius = 0.25f;

	const class Application& m_application;

	b2Vec2 m_initialPosition;
	b2Body* m_physicsBody = nullptr;

	SDL_Texture* m_texture = nullptr;

public:
	Ball(const class Application& application, const std::string& textureFilepath, const unsigned int windowWidth);
	~Ball() noexcept = default;

	void Draw();
	bool IsOffscreen(const unsigned int windowHeight);
	void ResetPosition();

	inline float GetRadius() const noexcept { return s_Radius; }

private:
	void CreatePhysicsBody();
	void InitialiseTexture(const std::string& filepath);

	SDL_Rect GetWorldRect() const;
};