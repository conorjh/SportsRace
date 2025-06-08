#include "App\App.h"
#include "Render\AppRenderer.h"
#include "SDL3/SDL_main.h"

int main(int argc, char* argv[])
{
	using namespace Game;
	using namespace Game::App;
	using namespace Game::App::Render;

	Application App(argc, argv);
	if (App.Ended())

	if (!App.Init())
		return App.ReturnCode;	

	AppRenderer Renderer(&App.Data, &App.Data.ScreenStack);
	while (!App.Ended())
	{
		App.Update();

		Renderer.Render();
	}
		
	return App.ReturnCode;
}