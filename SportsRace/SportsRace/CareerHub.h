#ifndef CAREERHUB_H
#define CAREERHUB_H
#include "State.h"
#include "App.h"
#include "Career.h"

namespace Game
{
	namespace States
	{
		class CareerHubState : public AppState
		{
			Career::CareerOrchestrator Orchestrator;
		public:
			GUI::IconButton RacerIcon, RaceIcon, TrainingIcon;
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


			Render::Image RacerIconGraphic, RaceIconGraphic, TrainingIconGraphic;
		};

		class CareerHubRenderer : public Render::BaseRenderer
		{
			CareerHubRendererData* RendererData;

		public:
			States::CareerHubState* State;

			CareerHubRenderer(AppData*, CareerHubRendererData* _Data, Render::BaseRendererData* _BaseData);
			CareerHubRenderer(AppData*, States::CareerHubState* State, CareerHubRendererData* _Data, Render::BaseRendererData* _BaseData);

			unsigned int Render();

		};
	}
}
#endif