#ifndef CAREERHUB_H
#define CAREERHUB_H
#include "..\Screens.h"
#include "..\Career.h"
#include "..\Render\Renderer.h"
#include "..\Util.h"

namespace Game
{
	namespace Screens
	{
		class CareerHubScreen : public AppScreen
		{
			Career::CareerOrchestrator Orchestrator;
		public:
			GUI::IconButton RacerIcon, RaceIcon, TrainingIcon, RankingIcon;

			CareerHubScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data);
			~CareerHubScreen();

			void Entry(), Exit();

			AppScreen* Update();
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