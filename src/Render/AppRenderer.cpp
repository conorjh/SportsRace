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
	using namespace std;

	spdlog::debug("Starting AppRenderer");
	StateMachine = _StateMachine;

	//Load datas -  constructing strings to allow for one line concatenation because C++ still 
	//				makes you jump through hoops to do the most common tasks
	spdlog::debug("Loading renderer data...");
	spdlog::debug(string("Loading BaseData...")				+ string(BaseData->Load(*this) ? "success" : "failed"));
	spdlog::debug(string("Loading MainMenuRenData...")		+ string(MainMenuRenData.Load(*this) ? "success" : "failed"));
	spdlog::debug(string("Loading InRaceRenData...")		+ string(InRaceRenData.Load(*this) ? "success" : "failed"));
	spdlog::debug(string("Loading RacerScreenRenData...")	+ string(RacerScreenRenData.Load(*this) ? "success" : "failed"));
	spdlog::debug(string("Loading CareerHubRenData...")		+ string(CareerHubRenData.Load(*this) ? "success" : "failed"));
	spdlog::debug(string("Loading RankingScreenRenData...") + string(RankingScreenRenData.Load(*this) ? "success" : "failed"));
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