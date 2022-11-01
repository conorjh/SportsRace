#ifndef CAREERHUB_H
#define CAREERHUB_H
#include "State.h"
#include "App.h"
#include "Career.h"
#include "Renderer.h"

namespace Game
{
	namespace States
	{
		class CareerHubState : public AppState
		{
			Career::CareerOrchestrator Orchestrator;
		public:
			GUI::IconButton RacerIcon, RaceIcon, TrainingIcon, RankingIcon;
			CareerHubState(AppStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data);
			~CareerHubState();

			void Entry(), Exit();

			AppState* Update();
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
			States::CareerHubState* State;

			CareerHubRenderer(App::AppData*, CareerHubRendererData* _Data, Render::BaseRendererData* _BaseData);
			CareerHubRenderer(App::AppData*, States::CareerHubState* State, CareerHubRendererData* _Data, Render::BaseRendererData* _BaseData);

			unsigned int Render();

		};
	}
}
#endif