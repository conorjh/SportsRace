#ifndef APPRENDERER_H
#define APPRENDERER_H
#include "Renderer.h"
#include "..\App\App.h"
#include "..\Screens.h"
#include "..\Screens\MainMenu.h"
#include "..\Screens\RaceScreen.h"
#include "..\Screens\CareerHub.h"
#include "..\Screens\RacerScreen.h"
#include "..\Screens\RankingScreen.h"

namespace Game
{
	namespace App
	{
		namespace Render
		{
			class AppRenderer : public Game::Render::BaseRenderer
			{
				Game::Render::MainMenuRenderer MainMenuRen;
				Game::Render::RaceScreenRenderer InRaceRen;
				Game::Render::CareerHubRenderer CareerHubRen;
				Game::Render::RacerScreenRenderer RacerScreenRen;
				Game::Render::RankingScreenRenderer RankingScreenRen;

				Game::App::AppData* Data;
			public:
				Game::Screens::AppScreenStateMachine* StateMachine;

				AppRenderer(Game::App::AppData* Data, Screens::AppScreenStateMachine* _StateMachine);

				unsigned int Render();
			};
		}

	}

}
#endif