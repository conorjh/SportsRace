#ifndef MAINMENU_H
#define MAINMENU_H
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
}
#endif