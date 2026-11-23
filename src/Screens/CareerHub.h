#ifndef CAREERHUB_H
#define CAREERHUB_H
#include "..\Screens.h"
#include "..\Career.h"
#include "..\Render\Renderer.h"
#include "..\Util.h"
#include <string>

namespace Game
{
	namespace Screens
	{
		class CareerHubScreen : public AppScreen
		{
			Career::CareerOrchestrator Orchestrator;
			bool PlayerCreationPending = false;
		public:
			GUI::IconButton RacerIcon, RaceIcon, TrainingIcon, RankingIcon;
			GUI::Button RacerDBButton, WorldMapButton, LeagueBrowserButton, SaveCareerButton;
			std::string StatusMessage;

			CareerHubScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data,
				bool _PlayerCreationPending = false);
			~CareerHubScreen();

			void Entry(), Exit();

			AppScreen* Update();
			Career::CareerData* GetCareerData() const;
		};
	}

	namespace Render
	{

		class CareerHubRenderer : public Render::BaseRenderer
		{
		public:
			Screens::CareerHubScreen* Screen;

			CareerHubRenderer(Game::Render::AppRenderContext* Context);
			CareerHubRenderer(Game::Render::AppRenderContext* Context, Screens::CareerHubScreen* State);

			unsigned int Render();

		};
	}
}
#endif