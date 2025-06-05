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
	case AppScreenType::RacerScreen:
		return "RacerScreen";
	case AppScreenType::RankingScreen:
		return "RankingScreen";
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
	if (ScreenStack.size())
		ScreenStack.top()->Update();
}

void Game::Screens::AppScreenStateMachine::Pop()
{
	ScreenStack.top()->Exit();
	ScreenStack.pop();
	if(ScreenStack.size())
		ScreenStack.top()->Entry();
}

Game::Screens::AppScreen* Game::Screens::AppScreenStateMachine::Top()
{
	return ScreenStack.top();
}

void Game::Screens::AppScreenStateMachine::Push(AppScreen* State)
{
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
