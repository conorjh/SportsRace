#include "RacerScreen.h"
#include "Audio.h"

using namespace std;
using namespace Game::App;
using namespace Game::Audio;
using namespace Game::States;
using namespace Game::Race;

Game::States::RacerScreenState::RacerScreenState(AppStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* _RacerToDisplay, RacerScreenStateInitType _InitType) :
	AppState(_Machine, _IO, _Data),
	ExitButton(_IO, "Exit", 800, 600, 210, 80)
{
	Type = AppStateType::RacerScreen;
	InitType = _InitType;
	RacerToDisplay = _RacerToDisplay;
}

Game::States::RacerScreenState::~RacerScreenState()
{
}

AppState* Game::States::RacerScreenState::Update()
{
	ExitButton.Update();
	if (ExitButton.HasMouseClicked() || IO.Esc)
	{
		IO.Player.Play(BuiltInSounds::Click);

		//go back to last screen
		Machine.Pop();
		return Machine.Top();
	}

	return this;
}
