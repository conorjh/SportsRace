#ifndef MAINMENU_H
#define MAINMENU_H
#include "Renderer.h"
#include "State.h"
#include "App.h"
#include "SDL2/SDL_ttf.h"
#include "League.h"

namespace Game
{
	namespace States
	{
		class MainMenuState : public AppState
		{
			Race::Race RaceBuffer;
			Race::RacerDB Racers;

		public:
			GUI::Button RaceButton, CareerButton, ExitButton;

			MainMenuState(AppStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data);
			~MainMenuState();

			AppState* Update();
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
			States::MainMenuState* State;
			MainMenuRenderer(App::AppData*, MainMenuRendererData* _Data, Render::BaseRendererData* _BaseData);
			MainMenuRenderer(App::AppData*, States::MainMenuState* _State, MainMenuRendererData* _Data, Render::BaseRendererData* _BaseData);


			unsigned int Render();
		};
	}
}
#endif