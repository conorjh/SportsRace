#include "Screens.h"
#include "App\App.h"
#include "spdlog/spdlog.h"

std::string Game::Screens::AppStateTypeToString(AppScreenType Type)
{
	switch (Type)
	{
	case AppScreenType::Null:
		return "Null";
	case AppScreenType::RaceScreen:
		return "RaceScreen";
	case AppScreenType::MainMenu:
		return "MainMenu";
	case AppScreenType::CareerHub:
		return "CareerHub";
	case AppScreenType::WorldMapScreen:
		return "WorldMapScreen";
	case AppScreenType::BookiesScreen:
		return "BookiesScreen";
	case AppScreenType::GymScreen:
		return "GymScreen";
	case AppScreenType::LicenseOfficeScreen:
		return "LicenseOfficeScreen";
	case AppScreenType::BreedingFarmScreen:
		return "BreedingFarmScreen";
	case AppScreenType::RaceTrackScreen:
		return "RaceTrackScreen";
	case AppScreenType::StableScreen:
		return "StableScreen";
	case AppScreenType::RacerScreen:
		return "RacerScreen";
	case AppScreenType::RankingScreen:
		return "RankingScreen";
	case AppScreenType::NodeViewScreen:
		return "NodeViewScreen";
	case AppScreenType::RacerDBScreen:
		return "RacerDBScreen";
	case AppScreenType::OptionsScreen:
		return "OptionsScreen";
	case AppScreenType::SlotMachineScreen:
		return "SlotMachineScreen";
	case AppScreenType::RouletteScreen:
		return "RouletteScreen";
	case AppScreenType::LeagueBrowserScreen:
		return "LeagueBrowserScreen";
	}
	return "";
}

Game::Screens::AppScreen::AppScreen(AppScreenStateMachine& _Machine, Game::App::AppIO& _IO, Game::App::AppData& _Data)
	: ScreenStack(_Machine), 
	IO(_IO), 
	Data(_Data),
	Type()
{

}

Game::Screens::AppScreen::~AppScreen()
{

}

void Game::Screens::AppScreen::Entry()
{
	spdlog::trace("AppScreen::Entry({})", AppStateTypeToString(Type));

}

void Game::Screens::AppScreen::Exit()
{
	spdlog::trace("AppScreen::Exit({})", AppStateTypeToString(Type));
}

Game::Screens::AppScreen* Game::Screens::AppScreen::Update()
{
	return nullptr;
}


Game::Screens::AppScreenStateMachine::AppScreenStateMachine()
{

}

Game::Screens::AppScreenStateMachine::AppScreenStateMachine(AppScreen* StartingState)
{
	Push(StartingState);
}

void Game::Screens::AppScreenStateMachine::Update()
{
	// Only the top screen receives updates; screens below it remain suspended on the stack.
	if (ScreenStack.size())
		ScreenStack.top()->Update();
}

void Game::Screens::AppScreenStateMachine::Pop()
{
	ScreenStack.top()->Exit();
	ScreenStack.pop();
	// Resume the screen that was covered by the one just removed.
	if(ScreenStack.size())
		ScreenStack.top()->Entry();
}

Game::Screens::AppScreen* Game::Screens::AppScreenStateMachine::Top()
{
	return ScreenStack.top();
}

void Game::Screens::AppScreenStateMachine::Push(AppScreen* State)
{
	// Let the current screen release transient state while preserving it underneath the new screen.
	if (ScreenStack.size())
		ScreenStack.top()->Exit();
	ScreenStack.push(State);
	State->Entry();
}

void Game::Screens::AppScreenStateMachine::SwapTop(AppScreen* State)
{
	Pop();
	Push(State);
}
