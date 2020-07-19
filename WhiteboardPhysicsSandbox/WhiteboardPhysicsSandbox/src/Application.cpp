#include "Application.h"

#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>
#include <SDL2/SDL_image.h>

Application::Application()
{
	InitialiseSDL();
	InitialiseVideoCapture();
	InitialiseWindow();
	InitialiseRenderer();
	InitialisePhysics();
}

Application::~Application() noexcept
{
	cv::destroyAllWindows();

	SDL_DestroyRenderer(m_renderer);
	SDL_DestroyWindow(m_window);

	m_videoCapture.release();

	IMG_Quit();
	SDL_Quit();
}

void Application::Run()
{
	m_ticksCount = SDL_GetTicks();

	cv::Mat cameraFrame;
	SDL_Event event{ };

	while (m_isRunning)
	{
		PollEvents(event);
		
		CaptureWebcamFrame(cameraFrame);
		UpdateRealtimeBodies(cameraFrame, GetContourRects(cameraFrame));

		UpdatePhysics();
		Render();

		cv::waitKey(16);
	}
}

void Application::InitialiseSDL() const
{
	std::clog << "INITIALISING SDL...\n";

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
	{
		std::cerr << "Failed to initialise SDL.\n";

		return;
	}

	if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
	{
		std::cerr << "Failed to initialise SDL_image.\n";
		
		return;
	}

	std::clog << "SDL INITIALISED.\n";
}

void Application::InitialiseVideoCapture()
{
	std::clog << "INITIALISING VIDEO...\n";
	m_videoCapture.open(0, cv::VideoCaptureAPIs::CAP_MSMF);

	if (!m_videoCapture.isOpened())
	{
		std::cerr << "Failed to open webcam for video capture.\n";

		return;
	}
	
	{
		std::ifstream projectorIndexFile("data/projectorIndex.json");

		if (!projectorIndexFile.is_open())
		{
			std::cerr << "Failed to open projector display index file.\n";

			return;
		}
		
		nlohmann::json json;
		projectorIndexFile >> json;
		projectorIndexFile.close();

		m_projectorDisplayIndex = json["projector-display-index"];
	}

	SDL_Rect projectorDisplayBounds{ };
	
	if (SDL_GetDisplayBounds(m_projectorDisplayIndex, &projectorDisplayBounds) != 0)
	{
		std::cerr << "Invalid projector display index.\n";

		return;
	}

	m_videoCapture.set(cv::CAP_PROP_FRAME_WIDTH, projectorDisplayBounds.w);
	m_videoCapture.set(cv::CAP_PROP_FRAME_WIDTH, projectorDisplayBounds.h);

	std::clog << "VIDEO INITIALISED.\n";
	std::clog << "PROJECTOR DISPLAY INDEX: " << m_projectorDisplayIndex << "\n";
	std::clog << "PROJECTOR DIMENSIONS: " << m_videoCapture.get(cv::CAP_PROP_FRAME_WIDTH) << " * " << m_videoCapture.get(cv::CAP_PROP_FRAME_HEIGHT) << "\n";
}

void Application::InitialiseWindow()
{
	m_window = SDL_CreateWindow(
		"physics",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		static_cast<int>(m_videoCapture.get(cv::CAP_PROP_FRAME_WIDTH)),
		static_cast<int>(m_videoCapture.get(cv::CAP_PROP_FRAME_HEIGHT)),
		SDL_WINDOW_SHOWN
	);
}

void Application::InitialiseRenderer()
{
	m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
}

void Application::InitialisePhysics()
{
	static const b2Vec2 gravity{ 0.0f, 5.0f };
	m_physicsWorld = std::make_unique<b2World>(gravity);
	m_ball = std::make_unique<Ball>(*this, "images/circle.png", static_cast<unsigned int>(m_videoCapture.get(cv::CAP_PROP_FRAME_WIDTH)));
}

void Application::PollEvents(SDL_Event& event)
{
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym)
			{
			case SDLK_RETURN:
			{
				static bool isFullscreen = false;

				SDL_SetWindowFullscreen(m_window, isFullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
				isFullscreen = !isFullscreen;
			}

				break;

			case SDLK_ESCAPE:
				m_isRunning = false;

			default:
				break;
			}

			break;

		case SDL_QUIT:
			m_isRunning = false;

			break;

		default:
			break;
		}
	}
}

void Application::CaptureWebcamFrame(cv::Mat& cameraFrame)
{
	m_videoCapture >> cameraFrame;
	cv::imshow("webcam", cameraFrame);

	cv::cvtColor(cameraFrame, cameraFrame, cv::COLOR_BGR2GRAY);
	cv::imshow("webcam_greyscale", cameraFrame);
	cv::threshold(cameraFrame, cameraFrame, 140.0, 255.0f, cv::THRESH_BINARY_INV);
	cv::imshow("webcam_threshold", cameraFrame);
}

std::vector<cv::RotatedRect> Application::GetContourRects(const cv::Mat& cameraFrame)
{
	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;

	cv::findContours(cameraFrame, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

	std::vector<cv::RotatedRect> minRects(contours.size());

	for (int i = 0; i < contours.size(); ++i)
	{
		minRects[i] = cv::minAreaRect(cv::Mat(contours[i]));
	}

	for (auto& body : m_realtimeBodies)
	{
		m_physicsWorld->DestroyBody(body);
	}

	m_realtimeBodies.clear();
	m_realtimeBodies.reserve(contours.size());

	return minRects;
}

void Application::UpdateRealtimeBodies(const cv::Mat& cameraFrame, const std::vector<cv::RotatedRect>& contourRects)
{
	cv::Mat frameContours = cv::Mat::zeros(cameraFrame.size(), CV_8UC3);

	for (std::size_t i = 0; i < contourRects.size(); i++)
	{
		if (contourRects[i].size.area() < 100.0f)
		{
			continue;
		}

		std::array<cv::Point2f, 4u> rectPoints;
		contourRects[i].points(rectPoints.data());

		b2BodyDef bodyDef;
		bodyDef.type = b2_staticBody;
		bodyDef.position.Set(rectPoints[0].x / s_PixelsPerMetre, rectPoints[0].y / s_PixelsPerMetre);

		b2Vec2 vertices[4];

		for (std::size_t j = 0; j < 4; ++j)
		{
			vertices[j].x = (rectPoints[j].x - rectPoints[0].x) / s_PixelsPerMetre;
			vertices[j].y = (rectPoints[j].y - rectPoints[0].y) / s_PixelsPerMetre;
		}

		b2PolygonShape collider;
		collider.Set(vertices, 4);

		b2FixtureDef fixtureDef;
		fixtureDef.shape = &collider;
		fixtureDef.density = 0.0f;
		fixtureDef.friction = 0.1f;
		fixtureDef.restitution = 0.1f;

		m_realtimeBodies.push_back(m_physicsWorld->CreateBody(&bodyDef));
		m_realtimeBodies.back()->CreateFixture(&fixtureDef);

		for (std::size_t j = 0; j < 4; j++)
		{
			cv::line(frameContours, rectPoints[j], rectPoints[(j + 1) % 4], cv::Scalar(0, 255, 0), 1, cv::LINE_8);
		}
	}

	cv::imshow("webcam_contours", frameContours);
}

void Application::UpdatePhysics()
{
	m_physicsWorld->Step((SDL_GetTicks() - m_ticksCount) / 1000.0f, 6, 2);
	m_ticksCount = SDL_GetTicks();

	if (m_ball->IsOffscreen(static_cast<unsigned int>(m_videoCapture.get(cv::CAP_PROP_FRAME_HEIGHT))))
	{
		m_ball->ResetPosition();
	}
}

void Application::Render() const
{
	SDL_SetRenderDrawColor(m_renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(m_renderer);

	m_ball->Draw();

	SDL_RenderPresent(m_renderer);
}