#include "RacerScreen.h"
#include "Audio.h"

using namespace std;
using namespace Game::App;
using namespace Game::Audio;
using namespace Game::States;
using namespace Game::Race;

Game::States::RacerScreenState::RacerScreenState(AppStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* _RacerToDisplay, RacerScreenStateInitType _InitType) :
	AppState(_Machine, _IO, _Data),
	RegenButton(_IO, "Regen", 400, 500, 210, 80),
	ApplyButton(_IO, "Apply", 750, 650, 210, 80),
	ExitButton(_IO, "Exit", 750, 650, 210, 80)
{
	Type = AppStateType::RacerScreen;
	InitType = _InitType;
	RacerToDisplay = _RacerToDisplay;

	if (InitType == RacerScreenStateInitType::RacerCreation)
	{
		Data.Profile->MainFella.Name = RacerNameMaker().Make();
	}
}

Game::States::RacerScreenState::~RacerScreenState()
{
}

AppState* Game::States::RacerScreenState::Update()
{
	RegenButton.Update();
	if (RegenButton.HasMouseClicked())
	{
		IO.Player.Play(BuiltInSounds::Click);

		Data.Profile->MainFella.Name = RacerNameMaker().Make();
		Data.Profile->MainFella.Skills = RacerSkills();
	}

	if (InitType == RacerScreenStateInitType::RacerCreation)
		ApplyButton.Update();
	if (InitType == RacerScreenStateInitType::ViewOnly)
		ExitButton.Update();
	if ((ApplyButton.HasMouseClicked() || IO.Esc) || (ExitButton.HasMouseClicked() || IO.Esc))
	{
		IO.Player.Play(BuiltInSounds::Click);

		//go back to last screen
		Machine.Pop();
		return Machine.Top();
	}

	return this;
}
