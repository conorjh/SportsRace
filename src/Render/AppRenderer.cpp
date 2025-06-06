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
		RaceScreen* Race_Screen = reinterpret_cast<RaceScreen*>(StateMachine->Top());
		InRaceRen.Screen = Race_Screen;
		return InRaceRen.Render();
	}

	case AppScreenType::MainMenu:
	{
		MainMenuScreen* Menu_Screen = reinterpret_cast<MainMenuScreen*>(StateMachine->Top());
		MainMenuRen.Screen = Menu_Screen;
		return MainMenuRen.Render();
	}
	case AppScreenType::CareerHub:
	{
		CareerHubScreen* CareerHub_Screen = reinterpret_cast<CareerHubScreen*>(StateMachine->Top());
		CareerHubRen.Screen = CareerHub_Screen;
		return CareerHubRen.Render();
	}
	case AppScreenType::RacerScreen:
	{
		RacerScreen* Racer_Screen = reinterpret_cast<RacerScreen*>(StateMachine->Top());
		RacerScreenRen.Screen = Racer_Screen;
		return RacerScreenRen.Render();
	}
	case AppScreenType::RankingScreen:
	{
		RankingScreen* Ranking_Screen = reinterpret_cast<RankingScreen*>(StateMachine->Top());
		RankingScreenRen.Screen = Ranking_Screen;
		return RankingScreenRen.Render();
	}
	}

	return 0;
}