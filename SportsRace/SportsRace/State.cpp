#include "State.h"
#include "App.h"
#include "spdlog/spdlog.h"

using namespace Game;
using namespace Game::Screens;
using namespace Game::App;
using namespace Game::Audio;

std::string Game::Screens::AppStateTypeToString(AppStateType Type)
{
	switch (Type)
	{
	case AppStateType::Null:
		return "Null";
	case AppStateType::RaceScreen:
		return "RaceScreen";
	case AppStateType::MainMenu:
		return "MainMenu";
	case AppStateType::CareerHub:
		return "CareerHub";
	case AppStateType::RacerScreen:
		return "RacerScreen";
	case AppStateType::RankingScreen:
		return "RankingScreen";
	}
	return "";
}

Game::Screens::AppScreen::AppScreen(AppStateMachine& _Machine, AppIO& _IO, AppData& _Data) : Machine(_Machine), IO(_IO), Data(_Data)
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

AppScreen* Game::Screens::AppScreen::Update()
{
	return nullptr;
}


Game::Screens::AppStateMachine::AppStateMachine()
{

}

Game::Screens::AppStateMachine::AppStateMachine(AppScreen* StartingState)
{
	Push(StartingState);
}

void Game::Screens::AppStateMachine::Update()
{
	if (StateStack.size())
		StateStack.top()->Update();
}

void Game::Screens::AppStateMachine::Pop()
{
	StateStack.top()->Exit();
	StateStack.pop();
	if(StateStack.size())
		StateStack.top()->Entry();
}

AppScreen* Game::Screens::AppStateMachine::Top()
{
	return StateStack.top();
}

void Game::Screens::AppStateMachine::Push(AppScreen* State)
{
	if (StateStack.size())
		StateStack.top()->Exit();
	StateStack.push(State);
	State->Entry();
}

void Game::Screens::AppStateMachine::SwapTop(AppScreen* State)
{
	Pop();
	Push(State);
}
