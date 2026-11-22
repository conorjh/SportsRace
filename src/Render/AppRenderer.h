#ifndef APPRENDERER_H
#define APPRENDERER_H
#include "Renderer.h"
#include "..\App\App.h"
#include "..\Screens.h"
#include "..\Screens\MainMenu.h"
#include "..\Screens\RaceScreen.h"
#include "..\Screens\NodeViewScreen.h"
#include "..\Screens\CareerHub.h"
#include "..\Screens\RacerScreen.h"
#include "..\Screens\RankingScreen.h"
#include "..\Screens\RacerDBScreen.h"
#include "..\Screens\OptionsScreen.h"
#include "..\Screens\WorldMapScreen.h"
#include "..\Screens\LeagueBrowserScreen.h"
#include "..\Screens\BuildingScreen.h"
#include "..\Screens\SlotMachineScreen.h"
#include "..\Screens\RouletteScreen.h"

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
				Game::Render::NodeViewRenderer NodeViewRen;
				Game::Render::CareerHubRenderer CareerHubRen;
				Game::Render::WorldMapRenderer WorldMapRen;
				Game::Render::BuildingScreenRenderer BuildingRen;
				Game::Render::SlotMachineScreenRenderer SlotMachineRen;
				Game::Render::RouletteScreenRenderer RouletteRen;
				Game::Render::RacerScreenRenderer RacerScreenRen;
				Game::Render::RankingScreenRenderer RankingScreenRen;
				Game::Render::RacerDBScreenRenderer RacerDBScreenRen;
				Game::Render::LeagueBrowserRenderer LeagueBrowserRen;
				Game::Render::OptionsScreenRenderer OptionsScreenRen;

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