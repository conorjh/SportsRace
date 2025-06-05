#include "AppRenderer.h"
#include "spdlog/spdlog.h"

using namespace Game;
using namespace Game::App;
using namespace Game::Render;
using namespace Game::Screens;
using namespace Game::Render;

Game::App::Render::AppRenderer::AppRenderer(Game::App::AppData* Data, Screens::AppScreenStateMachine* _StateMachine) :
	MainMenuRen(Data->RenderContext),
	InRaceRen(Data->RenderContext),
	RacerScreenRen(Data->RenderContext, Data->Profile),
	RankingScreenRen(Data->RenderContext),
	CareerHubRen(Data->RenderContext),
	BaseRenderer(Data->RenderContext),
	Data(Data),
	StateMachine(_StateMachine)
{
	Context->Store.RegisterAll(*this);
}

unsigned int Game::App::Render::AppRenderer::Render()
{
	if (StateMachine->Top() == nullptr)
		return 0;

	switch (StateMachine->Top()->Type)
	{
	case AppScreenType::RaceScreen:
	{
		RaceScreen* RaceState = reinterpret_cast<RaceScreen*>(StateMachine->Top());
		InRaceRen.Screen = RaceState;
		return InRaceRen.Render();
	}

	case AppScreenType::MainMenu:
	{
		MainMenuScreen* MenuState = reinterpret_cast<MainMenuScreen*>(StateMachine->Top());
		MainMenuRen.Screen = MenuState;
		return MainMenuRen.Render();
	}
	case AppScreenType::CareerHub:
	{
		CareerHubScreen* CareerHState = reinterpret_cast<CareerHubScreen*>(StateMachine->Top());
		CareerHubRen.Screen = CareerHState;
		return CareerHubRen.Render();
	}
	case AppScreenType::RacerScreen:
	{
		RacerScreen* RacerScreenRenSt = reinterpret_cast<RacerScreen*>(StateMachine->Top());
		RacerScreenRen.Screen = RacerScreenRenSt;
		return RacerScreenRen.Render();
	}
	case AppScreenType::RankingScreen:
	{
		RankingScreen* RankingScreenStt = reinterpret_cast<RankingScreen*>(StateMachine->Top());
		RankingScreenRen.Screen = RankingScreenStt;
		return RankingScreenRen.Render();
	}
	}

	return 0;
}