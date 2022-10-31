#ifndef RACERSCREEN_H
#define RACERSCREEN_H
#include "State.h"
#include "App.h"
#include "Race.h"

namespace Game
{
	namespace States
	{
		enum class RacerScreenStateInitType
		{
			RacerCreation, ViewOnly
		};

		class RacerScreenState : public AppState
		{
		protected:
			Race::Racer* RacerToDisplay;

		public:
			RacerScreenStateInitType InitType;

			GUI::Button RegenButton, ApplyButton, ExitButton;
			RacerScreenState(AppStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* _RacerToDisplay, RacerScreenStateInitType _Type);

			~RacerScreenState();

			AppState* Update();
		};

	}
}
#endif