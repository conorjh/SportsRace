#include "MainMenu.h"
#include "RaceState.h"
#include "CareerHub.h"
#include "SDL2/SDL_mixer.h"

using namespace std;
using namespace Game;
using namespace Game::States;
using namespace Game::App;
using namespace Game::Audio;
using namespace Game::Race;

Game::States::MainMenuState::MainMenuState(AppStateMachine& _Machine, AppIO& _IO, AppData& _Data) : 
	AppState(_Machine, _IO, _Data),
	RaceButton(_IO, "Race", 100, 100, 210, 80),
	CareerButton(_IO, "Career", 100, 200, 290, 80),
	ExitButton(_IO, "Exit", 100, 400, 180, 80)
{
	Type = AppStateType::MainMenu;

	for (int t = 0; t < 6; ++t)
		RaceBuffer.Racers.push_back(Racers.Make(RacerNameMaker().Make()));
}

Game::States::MainMenuState::~MainMenuState()
{

}

AppState* Game::States::MainMenuState::Update()
{
	RaceButton.Update();
	if (RaceButton.HasMouseClicked())
	{
		RaceBuffer.Reset();

		//actually run our race
		Machine.Push(new InRaceState(Machine, IO, Data, RaceBuffer));
		return Machine.Top();
	}

	CareerButton.Update();
	if (CareerButton.HasMouseClicked())
	{
		//check if we have a saved profile
		
		Machine.Push(new CareerHubState(Machine, IO, Data));
		return Machine.Top();
	}

	ExitButton.Update();
	if (ExitButton.HasMouseClicked())
		Data.Halted = true;

	return this;
}
