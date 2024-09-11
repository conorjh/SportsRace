#include "AppRenderer.h"
#include "spdlog/spdlog.h"

using namespace Game;
using namespace Game::App;
using namespace Game::Renderer;
using namespace Game::Screens;
using namespace Game::Screens;

Game::App::Renderer::AppRenderer::AppRenderer(AppData* _Data, Screens::AppScreenStateMachine* _StateMachine) :
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
	spdlog::debug("Loaded renderer data");

}

unsigned int Game::App::Renderer::AppRenderer::Render()
{
	if (StateMachine->Top() == nullptr)
		return 0;

	switch (StateMachine->Top()->Type)
	{
	case AppScreenType::RaceScreen:
	{
		RaceScreen* RaceState = reinterpret_cast<RaceScreen*>(StateMachine->Top());
		InRaceRen.State = RaceState;
		return InRaceRen.Render();
	}

	case AppScreenType::MainMenu:
	{
		MainMenuScreen* MenuState = reinterpret_cast<MainMenuScreen*>(StateMachine->Top());
		MainMenuRen.State = MenuState;
		return MainMenuRen.Render();
	}
	case AppScreenType::CareerHub:
	{
		CareerHubScreen* CareerHState = reinterpret_cast<CareerHubScreen*>(StateMachine->Top());
		CareerHubRen.State = CareerHState;
		return CareerHubRen.Render();
	}
	case AppScreenType::RacerScreen:
	{
		RacerScreen* RacerScreenRenSt = reinterpret_cast<RacerScreen*>(StateMachine->Top());
		RacerScreenRen.State = RacerScreenRenSt;
		return RacerScreenRen.Render();
	}
	case AppScreenType::RankingScreen:
	{
		RankingScreen* RankingScreenStt = reinterpret_cast<RankingScreen*>(StateMachine->Top());
		RankingScreenRen.State = RankingScreenStt;
		return RankingScreenRen.Render();
	}
	}

	return 0;
}