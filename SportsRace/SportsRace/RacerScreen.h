#ifndef CAREER_H
#define CAREER_H
#include "State.h"
#include "App.h"
#include "SDL2/SDL_ttf.h"
#include "League.h"
#include "Race.h"

namespace Game
{
	namespace States
	{
		class RacerScreenState : public AppState
		{
		protected:
			Race::Racer* RacerToDisplay;
			GUI::Button ExitButton;

		public:
			RacerScreenState(AppStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* _RacerToDisplay);
			~RacerScreenState();

			AppState* Update();
		};

	}
}
#endif