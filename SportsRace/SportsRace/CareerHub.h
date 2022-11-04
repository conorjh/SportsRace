#ifndef CAREERHUB_H
#define CAREERHUB_H
#include "Screens.h"
#include "App.h"
#include "Career.h"
#include "Renderer.h"

namespace Game
{
	namespace Screens
	{
		class CareerHubScreen : public AppScreen
		{
			Career::CareerOrchestrator Orchestrator;
		public:
			GUI::IconButton RacerIcon, RaceIcon, TrainingIcon, RankingIcon;

			CareerHubScreen(AppStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data);
			~CareerHubScreen();

			void Entry(), Exit();

			AppScreen* Update();
		};
	}

	namespace Renderer
	{
		struct CareerHubRendererData
		{
			bool Load(Render::BaseRenderer& Renderer);


			Render::Image RacerIconGraphic, RaceIconGraphic, TrainingIconGraphic, RankingIconGraphic;
		};

		class CareerHubRenderer : public Render::BaseRenderer
		{
			CareerHubRendererData* RendererData;

		public:
			Screens::CareerHubScreen* State;

			CareerHubRenderer(App::AppData*, CareerHubRendererData* _Data, Render::BaseRendererData* _BaseData);
			CareerHubRenderer(App::AppData*, Screens::CareerHubScreen* State, CareerHubRendererData* _Data, Render::BaseRendererData* _BaseData);

			unsigned int Render();

		};
	}
}
#endif