#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

#include <box2d/box2d.h>
#include <opencv2/opencv.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

int main(const int argc, char* argv[])
{
	std::clog << "INITIALISING SDL...\n";
	
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL Error", "Failed to initialise SDL.", nullptr);
		std::cin.get();

		return EXIT_FAILURE;
	}

	if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL_Image Error", "Failed to initialise SDL_image.", nullptr);
		std::cin.get();

		return EXIT_FAILURE;
	}

	std::clog << "SDL INITIALISED.\n";

	std::clog << "INITIALISING VIDEO...\n";
	cv::VideoCapture videoCapture(0);

	if (!videoCapture.isOpened())
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Video Capture Error", "Failed to open webcam.", nullptr);
		std::cin.get();

		return EXIT_FAILURE;
	}

	std::clog << "VIDEO INITIALISED.\n";

	cv::Mat cameraFrame;

	SDL_Window* physicsWindow = SDL_CreateWindow(
		"physics",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		static_cast<int>(videoCapture.get(cv::CAP_PROP_FRAME_WIDTH)),
		static_cast<int>(videoCapture.get(cv::CAP_PROP_FRAME_HEIGHT)),
		SDL_WINDOW_SHOWN
	);

	SDL_SetWindowOpacity(physicsWindow, 0.5f);

	SDL_Renderer* renderer = SDL_CreateRenderer(physicsWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	b2World physicsWorld({ 0.0f, 5.0f });
	std::vector<b2Body*> whiteboardBodies;
	constexpr float PixelsPerMetre = 100.0f;

	constexpr float BallRadius = 0.25f;
	b2BodyDef bodyDef;
	bodyDef.type = b2_dynamicBody;
	bodyDef.position.Set(static_cast<int>(videoCapture.get(cv::CAP_PROP_FRAME_WIDTH)) / 2.0f / PixelsPerMetre, 0.0f);
	b2Body* ballBody = physicsWorld.CreateBody(&bodyDef);

	b2CircleShape dynamicCircle;
	dynamicCircle.m_p.Set(0.0f, 0.0f);
	dynamicCircle.m_radius = BallRadius;

	b2FixtureDef fixtureDef;
	fixtureDef.shape = &dynamicCircle;
	fixtureDef.density = 0.1f;
	fixtureDef.friction = 0.1f;
	fixtureDef.restitution = 0.2f;
	ballBody->CreateFixture(&fixtureDef);

	SDL_Surface* loadedTexture = IMG_Load("images/circle.png");
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, loadedTexture);
	SDL_FreeSurface(loadedTexture);

	bool isRunning = true;
	std::uint32_t ticksCount = SDL_GetTicks();

	while (isRunning)
	{
		SDL_Event event{ };

		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_KEYDOWN:
			case SDL_QUIT:
				isRunning = false;

				break;

			default:
				break;
			}
		}

		videoCapture >> cameraFrame;
		cv::imshow("webcam", cameraFrame);

		cv::cvtColor(cameraFrame, cameraFrame, cv::COLOR_BGR2GRAY);
		cv::threshold(cameraFrame, cameraFrame, 128.0, 255.0f, cv::THRESH_BINARY_INV);
		cv::imshow("webcam_threshold", cameraFrame);

		std::vector<std::vector<cv::Point>> contours;
		std::vector<cv::Vec4i> hierarchy;

		cv::findContours(cameraFrame, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

		std::vector<cv::RotatedRect> minRects(contours.size());

		for (int i = 0; i < contours.size(); ++i)
		{
			minRects[i] = cv::minAreaRect(cv::Mat(contours[i]));
		}

		cv::Mat frameContours = cv::Mat::zeros(cameraFrame.size(), CV_8UC3);

		for (auto& body : whiteboardBodies)
		{
			physicsWorld.DestroyBody(body);
		}
		
		whiteboardBodies.clear();
		whiteboardBodies.reserve(contours.size());

		for (std::size_t i = 0; i < contours.size(); i++)
		{
			if (minRects[i].size.area() < 100.0f)
			{
				continue;
			}

			std::array<cv::Point2f, 4u> rectPoints;
			minRects[i].points(rectPoints.data());

			b2BodyDef bodyDef;
			bodyDef.type = b2_staticBody;
			bodyDef.position.Set(rectPoints[0].x / PixelsPerMetre, rectPoints[0].y / PixelsPerMetre);

			b2Vec2 vertices[4];

			for (std::size_t j = 0; j < 4; ++j)
			{
				vertices[j].x = (rectPoints[j].x - rectPoints[0].x) / PixelsPerMetre;
				vertices[j].y = (rectPoints[j].y - rectPoints[0].y) / PixelsPerMetre;
			}

			b2PolygonShape collider;
			collider.Set(vertices, 4);

			b2FixtureDef fixtureDef;
			fixtureDef.shape = &collider;
			fixtureDef.density = 0.0f;
			fixtureDef.friction = 0.1f;
			fixtureDef.restitution = 0.1f;

			whiteboardBodies.push_back(physicsWorld.CreateBody(&bodyDef));
			whiteboardBodies.back()->CreateFixture(&fixtureDef);

			for (std::size_t j = 0; j < 4; j++)
			{
				cv::line(frameContours, rectPoints[j], rectPoints[(j + 1) % 4], cv::Scalar(0, 255, 0), 1, cv::LINE_8);
			}
		}

		cv::imshow("webcam_contours", frameContours);

		physicsWorld.Step((SDL_GetTicks() - ticksCount) / 1000.0f, 6, 2);
		ticksCount = SDL_GetTicks();

		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer);

		const SDL_Rect destination{
			static_cast<int>(ballBody->GetPosition().x * PixelsPerMetre - (PixelsPerMetre * BallRadius)),
			static_cast<int>(ballBody->GetPosition().y * PixelsPerMetre - (PixelsPerMetre * BallRadius)),
			static_cast<int>(BallRadius * PixelsPerMetre * 2.0f),
			static_cast<int>(BallRadius * PixelsPerMetre * 2.0f)
		};

		SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, SDL_ALPHA_OPAQUE);
		SDL_RenderCopy(renderer, texture, nullptr, &destination);

		SDL_RenderPresent(renderer);

		if (ballBody->GetPosition().y * PixelsPerMetre > static_cast<int>(videoCapture.get(cv::CAP_PROP_FRAME_HEIGHT)) + 2.0f)
		{
			ballBody->SetTransform({ static_cast<int>(videoCapture.get(cv::CAP_PROP_FRAME_WIDTH)) / 2.0f / PixelsPerMetre, 0.0f }, 0.0f);
			ballBody->SetLinearVelocity({ 0.0f, 0.0f });
		}

		cv::waitKey(16);
	}

	cv::destroyAllWindows();
	SDL_DestroyWindow(physicsWindow);
	IMG_Quit();
	SDL_Quit();

	return EXIT_SUCCESS;
}