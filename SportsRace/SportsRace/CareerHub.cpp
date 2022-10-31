#include "CareerHub.h"
#include "Audio.h"
#include "RaceState.h"
#include "RacerScreen.h"

using namespace std;
using namespace Game::App;
using namespace Game::Audio;
using namespace Game::States;
using namespace Game::GUI;
using namespace Game::Race;

Game::States::CareerHubState::CareerHubState(AppStateMachine& _Machine, AppIO& _IO, AppData& _Data) :
	AppState(_Machine, _IO, _Data),
	Orchestrator(_Data.Career, _Data.Profile),

	RaceIcon(_IO, IconButtonType::Race, 400, 500, 200, 200),
	TrainingIcon(_IO, IconButtonType::Race, 700, 100, 200, 200),
	RacerIcon(_IO, IconButtonType::Race, 100, 100, 200, 200)
{
	Type = AppStateType::CareerHub;

}

Game::States::CareerHubState::~CareerHubState()
{

}

void Game::States::CareerHubState::Entry()
{
	IO.Player.Play(Soundtrack::Catering);
}

void Game::States::CareerHubState::Exit()
{
	IO.Player.StopMusic();
}


AppState* Game::States::CareerHubState::Update()
{
	RaceIcon.Update();
	if (RaceIcon.HasMouseClicked())
	{
		Race::Race* RaceBuffer = new Race::Race();
		RaceBuffer->Racers.push_back(&Orchestrator.Profile->MainFella);
		for (int t = 0; t < 5; ++t)
			RaceBuffer->Racers.push_back(Orchestrator.Data->Racers.Make(RacerNameMaker().Make()));
		
		//actually run our race
		Machine.Push(new RaceScreenState(Machine, IO, Data, *RaceBuffer, Orchestrator.Profile->MainFella.GUID));
		return Machine.Top();
	}

	TrainingIcon.Update();
	if (TrainingIcon.HasMouseClicked())
	{
		RacerDB* DB = new RacerDB();

		//actually run our race
		Machine.Push(new RaceScreenState(Machine, IO, Data, &Orchestrator.Profile->MainFella));
		return Machine.Top();
	}

	RacerIcon.Update();
	if(RacerIcon.HasMouseClicked())
	{
		//actually run our race
		Machine.Push(new RacerScreenState(Machine, IO, Data, &Orchestrator.Profile->MainFella, RacerScreenStateInitType::ViewOnly));
		return Machine.Top();
	}

	if (IO.Esc)
	{
		IO.Player.Play(BuiltInSounds::Click);

		Machine.Pop();
		return Machine.Top();
	}

	return this;
}
