#include "Ball.h"

#include <iostream>

#include <SDL2/SDL_image.h>

#include "Application.h"

Ball::Ball(const Application& application, const std::string& textureFilepath, const unsigned int windowWidth)
	: m_application(application), m_initialPosition({ windowWidth / m_application.GetPixelsPerMetre() / 2.0f, 0.0f })
{
	CreatePhysicsBody();
	InitialiseTexture(textureFilepath);
}

void Ball::Draw()
{
	SDL_SetRenderDrawColor(m_application.GetRenderer(), 0xFF, 0x00, 0x00, SDL_ALPHA_OPAQUE);

	const SDL_Rect destinationRect = GetWorldRect();
	SDL_RenderCopy(m_application.GetRenderer(), m_texture, nullptr, &destinationRect);
}

bool Ball::IsOffscreen(const unsigned int windowHeight)
{
	constexpr float Leeway = 2.0f;

	return m_physicsBody->GetPosition().y * m_application.GetPixelsPerMetre() > windowHeight + Leeway;
}

void Ball::ResetPosition()
{
	m_physicsBody->SetTransform(m_initialPosition, 0.0f);
	m_physicsBody->SetLinearVelocity({ 0.0f, 0.0f });
}

void Ball::CreatePhysicsBody()
{
	b2BodyDef bodyDef;
	bodyDef.type = b2_dynamicBody;
	bodyDef.position.Set(m_initialPosition.x, m_initialPosition.y);
	m_physicsBody = m_application.GetPhysicsWorld()->CreateBody(&bodyDef);

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

void Ball::InitialiseTexture(const std::string& filepath)
{
	SDL_Surface* loadedTextureSurface = IMG_Load(filepath.c_str());
	
	if (loadedTextureSurface == nullptr)
	{
		std::cerr << "Failed to load ball texture from file " << filepath << ".";

		return;
	}
	
	m_texture = SDL_CreateTextureFromSurface(m_application.GetRenderer(), loadedTextureSurface);
	SDL_FreeSurface(loadedTextureSurface);
}

SDL_Rect Ball::GetWorldRect() const
{
	return SDL_Rect{
		static_cast<int>((m_physicsBody->GetPosition().x - s_Radius) * m_application.GetPixelsPerMetre()),
		static_cast<int>((m_physicsBody->GetPosition().y - s_Radius) * m_application.GetPixelsPerMetre()),
		static_cast<int>(s_Radius * m_application.GetPixelsPerMetre() * 2.0f),
		static_cast<int>(s_Radius * m_application.GetPixelsPerMetre() * 2.0f)
	};
}