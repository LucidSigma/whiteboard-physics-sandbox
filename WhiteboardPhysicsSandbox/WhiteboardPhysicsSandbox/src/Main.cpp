#include <cstdlib>
#include <iostream>
#include <vector>

#include <box2d/box2d.h>
#include <opencv2/opencv.hpp>
#include <SDL2/SDL.h>

int main(const int argc, char* argv[])
{
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

	bool isRunning = true;
	SDL_Window* physicsWindow = SDL_CreateWindow(
		"physics",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		static_cast<int>(videoCapture.get(cv::CAP_PROP_FRAME_WIDTH)),
		static_cast<int>(videoCapture.get(cv::CAP_PROP_FRAME_HEIGHT)),
		SDL_WINDOW_SHOWN
	);
	SDL_Renderer* renderer = SDL_CreateRenderer(physicsWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

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

		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer);
		SDL_RenderPresent(renderer);

		videoCapture >> cameraFrame;
		cv::imshow("webcam", cameraFrame);

		cv::cvtColor(cameraFrame, cameraFrame, cv::COLOR_BGR2GRAY);
		cv::threshold(cameraFrame, cameraFrame, 140.0, 255.0f, cv::THRESH_BINARY_INV);
		cv::imshow("webcam_threshold", cameraFrame);


		cv::waitKey(16);
	}

	cv::destroyAllWindows();
	SDL_DestroyWindow(physicsWindow);
	SDL_Quit();

	return EXIT_SUCCESS;
}