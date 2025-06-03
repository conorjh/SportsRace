#ifndef MAINMENU_H
#define MAINMENU_H
#include "..\Render/Renderer.h"
#include "..\Screens.h"
#include "..\Util.h"
#include "..\Race.h"
#include "..\Racer.h"

namespace Game
{
	namespace Screens
	{
		class MainMenuScreen : public AppScreen
		{
			Race::Race RaceBuffer;
			Race::RacerDB Racers;

		public:
			GUI::Button RaceButton, CareerButton, ExitButton;

			MainMenuScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data);
			~MainMenuScreen();

			AppScreen* Update();
		};

	}

	namespace Render
	{
		class MainMenuRenderer : public Render::BaseRenderer
		{
			void DrawLeague();

		public:
			Screens::MainMenuScreen* State;
			MainMenuRenderer(Game::Render::AppRenderContext* RenderContext);
			MainMenuRenderer(Game::Render::AppRenderContext* RenderContext, Screens::MainMenuScreen* _State);

			unsigned int Render();
		};
	}
}
#endif