#include "State.h"
#include "App.h"
#include "spdlog/spdlog.h"

using namespace Game;
using namespace Game::States;
using namespace Game::App;
using namespace Game::Audio;

std::string Game::States::AppStateTypeToString(AppStateType Type)
{
	switch (Type)
	{
	case AppStateType::Null:
		return "Null";
	case AppStateType::InRace:
		return "InRace";
	case AppStateType::MainMenu:
		return "MainMenu";
	case AppStateType::CareerHub:
		return "CareerHub";
	case AppStateType::RacerScreen:
		return "RacerScreen";
	}
	return "";
}

Game::States::AppState::AppState(AppStateMachine& _Machine, AppIO& _IO, AppData& _Data) : Machine(_Machine), IO(_IO), Data(_Data)
{

}

Game::States::AppState::~AppState()
{

}

void Game::States::AppState::Entry()
{
	spdlog::trace("AppState::Entry({})", AppStateTypeToString(Type));

}

void Game::States::AppState::Exit()
{
	spdlog::trace("AppState::Exit({})", AppStateTypeToString(Type));
}

AppState* Game::States::AppState::Update()
{
	return nullptr;
}


Game::States::AppStateMachine::AppStateMachine()
{

}

Game::States::AppStateMachine::AppStateMachine(AppState* StartingState)
{
	Push(StartingState);
}

void Game::States::AppStateMachine::Update()
{
	if (StateStack.size())
		StateStack.top()->Update();
}

void Game::States::AppStateMachine::Pop()
{
	StateStack.top()->Exit();
	StateStack.pop();
}

AppState* Game::States::AppStateMachine::Top()
{
	return StateStack.top();
}

void Game::States::AppStateMachine::Push(AppState* State)
{
	StateStack.push(State);
	State->Entry();
}

void Game::States::AppStateMachine::SwapTop(AppState* State)
{
	Pop();
	Push(State);
}
