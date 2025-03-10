#ifndef APPRENDERER_H
#define APPRENDERER_H
#include "..\Screens.h"
#include "..\App\App.h"
#include "..\Screens\MainMenu.h"
#include "..\Screens\RaceScreen.h"
#include "..\Screens\CareerHub.h"
#include "..\Screens\RacerScreen.h"
#include "..\Screens\RankingScreen.h"

namespace Game
{
	namespace App
	{
		namespace Renderer
		{
			class AppRenderer : public Render::BaseRenderer
			{
				Render::BaseRendererData BaseRenData;
				Game::Renderer::MainMenuRendererData MainMenuRenData;
				Game::Renderer::MainMenuRenderer MainMenuRen;
				Game::Renderer::RaceScreenRendererData InRaceRenData;
				Game::Renderer::RaceScreenRenderer InRaceRen;
				Game::Renderer::CareerHubRendererData CareerHubRenData;
				Game::Renderer::CareerHubRenderer CareerHubRen;
				Game::Renderer::RacerScreenRendererData RacerScreenRenData;
				Game::Renderer::RacerScreenRenderer RacerScreenRen;
				Game::Renderer::RankingScreenRendererData RankingScreenRenData;
				Game::Renderer::RankingScreenRenderer RankingScreenRen;

			public:
				Game::Screens::AppScreenStateMachine* StateMachine;

				AppRenderer(AppData* _RenderData, Screens::AppScreenStateMachine* _StateMachine);

				unsigned int Render();
			};
		}

	}

}
#endif