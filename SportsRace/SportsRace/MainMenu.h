#ifndef MAINMENU_H
#define MAINMENU_H
#include "Renderer.h"
#include "Screens.h"
#include "App.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "League.h"

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

	namespace Renderer
	{
		struct MainMenuRendererData
		{
			bool Load(Render::BaseRenderer& Renderer);

			Render::Image Screen;

			TTF_Font* MainFont;
		};

		class MainMenuRenderer : public Render::BaseRenderer
		{
			MainMenuRendererData* RendererData;

			void DrawLeague();

		public:
			Screens::MainMenuScreen* State;
			MainMenuRenderer(App::AppData*, MainMenuRendererData* _Data, Render::BaseRendererData* _BaseData);
			MainMenuRenderer(App::AppData*, Screens::MainMenuScreen* _State, MainMenuRendererData* _Data, Render::BaseRendererData* _BaseData);


			unsigned int Render();
		};
	}
}
#endif