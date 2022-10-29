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
}
#endif