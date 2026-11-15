#include "App\App.h"
#include "Render\AppRenderer.h"
#include "SDL3/SDL_main.h"
#include <chrono>
#include <thread>

namespace
{
	constexpr std::chrono::nanoseconds RaceScreenFrameDuration(16666667);
}

int main(int argc, char* argv[])
{
	using namespace Game;
	using namespace Game::App;
	using namespace Game::App::Render;

	//read command line
	Application App(argc, argv);	
	if (App.Ended())
		return App.ReturnCode;	//could fail to parse command line etc

	if (!App.Init())
		return App.ReturnCode;	

	AppRenderer Renderer(&App.Data, &App.Data.ScreenStack);
	while (!App.Ended())
	{
		const auto FrameStart = std::chrono::steady_clock::now();
		App.Update();

		const Game::Screens::AppScreen* CurrentScreen = App.Data.ScreenStack.Top();
		Renderer.Render();
		if (CurrentScreen != nullptr && CurrentScreen->Type == Game::Screens::AppScreenType::RaceScreen)
			std::this_thread::sleep_until(FrameStart + RaceScreenFrameDuration);
	}
		
	return App.ReturnCode;
}