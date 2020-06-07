#pragma once

#include <memory>
#include <string>

#include <box2d/box2d.h>
#include <SDL2/SDL.h>

class Ball
{
private:
	static constexpr float s_Radius = 0.25f;

	b2Vec2 m_initialPosition;
	b2Body* m_physicsBody = nullptr;

	SDL_Texture* m_texture = nullptr;

public:
	Ball(const std::unique_ptr<b2World>& physicsWorld, SDL_Renderer* renderer, const std::string& textureFilepath, const unsigned int windowWidth, const float pixelsPerMetre);
	~Ball() noexcept = default;

	void Draw(SDL_Renderer* renderer, const float pixelsPerMetre);
	bool IsOffscreen(const unsigned int windowHeight, const float pixelsPerMetre);
	void ResetPosition();

	inline float GetRadius() const noexcept { return s_Radius; }

private:
	void CreatePhysicsBody(const std::unique_ptr<b2World>& physicsWorld);
	void InitialiseTexture(SDL_Renderer* renderer, const std::string& filepath);

	SDL_Rect GetWorldRect(const float pixelsPerMetre) const;
};