#include "AppRenderer.h"
#include "spdlog/spdlog.h"

using namespace Game;
using namespace Game::App;
using namespace Game::Renderer;
using namespace Game::States;
using namespace Game::States;

Game::App::Renderer::AppRenderer::AppRenderer(AppData* _Data, States::AppStateMachine* _StateMachine) :
	MainMenuRen(_Data, &MainMenuRenData, &BaseRenData),
	InRaceRen(_Data, &InRaceRenData, &BaseRenData),
	RacerScreenRen(_Data, &RacerScreenRenData, &BaseRenData),
	RankingScreenRen(_Data, &RankingScreenRenData, &BaseRenData),
	CareerHubRen(_Data, &CareerHubRenData, &BaseRenData),

	BaseRenderer(_Data, &BaseRenData)
{
	spdlog::debug("Starting AppRenderer");
	StateMachine = _StateMachine;

	//Load datas
	spdlog::debug("Loading renderer data...");
	BaseData->Load(*this);
	MainMenuRenData.Load(*this);
	InRaceRenData.Load(*this);
	RacerScreenRenData.Load(*this);
	CareerHubRenData.Load(*this);
	RankingScreenRenData.Load(*this);

}

unsigned int Game::App::Renderer::AppRenderer::Render()
{
	if (StateMachine->Top() == nullptr)
		return 0;

	switch (StateMachine->Top()->Type)
	{
	case AppStateType::RaceScreen:
	{
		RaceScreenState* RaceState = reinterpret_cast<RaceScreenState*>(StateMachine->Top());
		InRaceRen.State = RaceState;
		return InRaceRen.Render();
	}

	case AppStateType::MainMenu:
	{
		MainMenuState* MenuState = reinterpret_cast<MainMenuState*>(StateMachine->Top());
		MainMenuRen.State = MenuState;
		return MainMenuRen.Render();
	}
	case AppStateType::CareerHub:
	{
		CareerHubState* CareerHState = reinterpret_cast<CareerHubState*>(StateMachine->Top());
		CareerHubRen.State = CareerHState;
		return CareerHubRen.Render();
	}
	case AppStateType::RacerScreen:
	{
		RacerScreenState* RacerScreenRenSt = reinterpret_cast<RacerScreenState*>(StateMachine->Top());
		RacerScreenRen.State = RacerScreenRenSt;
		return RacerScreenRen.Render();
	}
	case AppStateType::RankingScreen:
	{
		RankingScreenState* RankingScreenStt = reinterpret_cast<RankingScreenState*>(StateMachine->Top());
		RankingScreenRen.State = RankingScreenStt;
		return RankingScreenRen.Render();
	}
	}

	return 0;
}