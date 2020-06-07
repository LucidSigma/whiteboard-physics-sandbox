#include "Ball.h"

#include <iostream>

#include <SDL2/SDL_image.h>

Ball::Ball(const std::unique_ptr<b2World>& physicsWorld, SDL_Renderer* renderer, const std::string& textureFilepath, const unsigned int windowWidth, const float pixelsPerMetre)
	: m_initialPosition({ windowWidth / pixelsPerMetre / 2.0f, 0.0f })
{
	CreatePhysicsBody(physicsWorld);
	InitialiseTexture(renderer, textureFilepath);
}

void Ball::Draw(SDL_Renderer* renderer, const float pixelsPerMetre)
{
	SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, SDL_ALPHA_OPAQUE);

	const SDL_Rect destinationRect = GetWorldRect(pixelsPerMetre);
	SDL_RenderCopy(renderer, m_texture, nullptr, &destinationRect);
}

bool Ball::IsOffscreen(const unsigned int windowHeight, const float pixelsPerMetre)
{
	constexpr float Leeway = 2.0f;

	return m_physicsBody->GetPosition().y * pixelsPerMetre > windowHeight + Leeway;
}

void Ball::ResetPosition()
{
	m_physicsBody->SetTransform(m_initialPosition, 0.0f);
	m_physicsBody->SetLinearVelocity({ 0.0f, 0.0f });
}

void Ball::CreatePhysicsBody(const std::unique_ptr<b2World>& physicsWorld)
{
	b2BodyDef bodyDef;
	bodyDef.type = b2_dynamicBody;
	bodyDef.position.Set(m_initialPosition.x, m_initialPosition.y);
	m_physicsBody = physicsWorld->CreateBody(&bodyDef);

	b2CircleShape circleCollider;
	circleCollider.m_p.Set(0.0f, 0.0f);
	circleCollider.m_radius = s_Radius;

	b2FixtureDef fixtureDef;
	fixtureDef.shape = &circleCollider;
	fixtureDef.density = 0.1f;
	fixtureDef.friction = 0.1f;
	fixtureDef.restitution = 0.2f;
	m_physicsBody->CreateFixture(&fixtureDef);
}

void Ball::InitialiseTexture(SDL_Renderer* renderer, const std::string& filepath)
{
	SDL_Surface* loadedTextureSurface = IMG_Load(filepath.c_str());
	
	if (loadedTextureSurface == nullptr)
	{
		std::cerr << "Failed to load ball texture from file " << filepath << ".";

		return;
	}
	
	m_texture = SDL_CreateTextureFromSurface(renderer, loadedTextureSurface);
	SDL_FreeSurface(loadedTextureSurface);
}

SDL_Rect Ball::GetWorldRect(const float pixelsPerMetre) const
{
	return SDL_Rect{
		static_cast<int>(m_physicsBody->GetPosition().x * pixelsPerMetre - (pixelsPerMetre * s_Radius)),
		static_cast<int>(m_physicsBody->GetPosition().y * pixelsPerMetre - (pixelsPerMetre * s_Radius)),
		static_cast<int>(s_Radius * pixelsPerMetre * 2.0f),
		static_cast<int>(s_Radius * pixelsPerMetre * 2.0f)
	};
}