#pragma once

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include <box2d/box2d.h>
#include <opencv2/opencv.hpp>
#include <SDL2/SDL.h>

#include "Ball.h"

class Application
{
private:
	static constexpr float s_PixelsPerMetre = 100.0f;

	cv::VideoCapture m_videoCapture;
	int m_projectorDisplayIndex = 0;
	SDL_Window* m_window = nullptr;
	SDL_Renderer* m_renderer = nullptr;

	std::unique_ptr<b2World> m_physicsWorld = nullptr;
	std::unique_ptr<Ball> m_ball = nullptr;
	std::vector<b2Body*> m_realtimeBodies;

	bool m_isRunning = true;
	std::uint32_t m_ticksCount = 0;

public:
	Application();
	~Application() noexcept;

	void Run();

	inline float GetPixelsPerMetre() const noexcept { return s_PixelsPerMetre; }
	inline SDL_Renderer* GetRenderer() const noexcept { return m_renderer; }
	inline const std::unique_ptr<b2World>& GetPhysicsWorld() const noexcept { return m_physicsWorld; }

private:
	void InitialiseSDL() const;
	void InitialiseVideoCapture();
	void InitialiseWindow();
	void InitialiseRenderer();
	void InitialisePhysics();

	void PollEvents(SDL_Event& event);
	void CaptureWebcamFrame(cv::Mat& cameraFrame);
	std::vector<cv::RotatedRect> GetContourRects(const cv::Mat& cameraFrame);
	
	void UpdateRealtimeBodies(const cv::Mat& cameraFrame, const std::vector<cv::RotatedRect>& contourRects);
	void UpdatePhysics();
	void Render() const;
};