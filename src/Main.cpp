#include "App\App.h"
#include "Render\AppRenderer.h"
#include "SDL3/SDL_main.h"

using namespace Game;
using namespace Game::App;
using namespace Game::App::Renderer;

int main(int argc, char* argv[])
{
	Application App(argc, argv);

	if (!App.Init())
		return App.ReturnCode;	

	AppRenderer Renderer(&App.Data, &App.ScreenStack);
	while (!App.Ended())
	{
		App.Update();

		Renderer.Render();
	}
		
	return App.ReturnCode;
}