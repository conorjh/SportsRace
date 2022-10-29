#include "RacerScreen.h"

using namespace std;
using namespace Game::App;
using namespace Game::States;
using namespace Game::Race;

Game::States::RacerScreenState::RacerScreenState(AppStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* _RacerToDisplay) : 
	AppState(_Machine, _IO, _Data),
	ExitButton(_IO, "Exit", 100, 200, 210, 80)
{
	Type = AppStateType::RacerScreen;

	RacerToDisplay = _RacerToDisplay;

}

Game::States::RacerScreenState::~RacerScreenState()
{
}

AppState* Game::States::RacerScreenState::Update()
{
	ExitButton.Update();
	if (ExitButton.HasMouseClicked())
	{
		//go back to last screen
		Machine.Pop();
		return Machine.Top();
	}

	return this;
}
