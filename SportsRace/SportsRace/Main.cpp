#include "App.h"
#include "AppRenderer.h"

using namespace Game;
using namespace Game::App;
using namespace Game::App::Renderer;

int main(int argc, char* argv[])
{
	Application App(argc, argv);

	if (!App.Init())
		return 0;	

	AppRenderer Renderer(&App.Data, &App.StateMachine);
	while (!App.Ended())
	{
		App.Update();

		Renderer.Render();
	}
		
	return 0;
}
