#include "Screens.h"
#include "App\App.h"
#include "Screens\NodeViewScreen.h"
#include "Screens\PostRaceScreen.h"
#include "Screens\RacerScreen.h"
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
	case AppScreenType::PostRaceScreen:
		return "PostRaceScreen";
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

bool Game::Screens::AppScreen::IsPlayerOwnedRacer(const Game::Race::Racer* Runner) const
{
	return Data.IsPlayerOwnedRacer(Runner);
}

bool Game::Screens::AppScreen::HandleRacerInfoClick()
{
	if (Type == AppScreenType::RaceScreen || Type == AppScreenType::RacerScreen || Data.RenderContext == nullptr ||
		(IO.MouseButtonsPressed & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) == 0)
		return false;

	const SDL_Point MousePoint{ static_cast<int>(IO.MousePosition.x), static_cast<int>(IO.MousePosition.y) };
	const auto& Targets = Data.RenderContext->RacerInfoClickTargets;
	for (auto Target = Targets.rbegin(); Target != Targets.rend(); ++Target)
		if (Target->Runner != nullptr && SDL_PointInRect(&MousePoint, &Target->Bounds))
		{
			ScreenStack.Push(new RacerScreen(ScreenStack, IO, Data, Target->Runner,
				RacerScreenStateInitType::ViewOnly));
			return true;
		}
	return false;
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
	{
		AppScreen* CurrentScreen = ScreenStack.top();
		CurrentScreen->Update();
		if (ScreenStack.size() && ScreenStack.top() == CurrentScreen)
			CurrentScreen->HandleRacerInfoClick();
	}
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

bool Game::Screens::OpenCareerHistoricalRace(AppScreenStateMachine& ScreenStack, Game::App::AppIO& IO,
	Game::App::AppData& Data, std::uint64_t HistoryId, bool OpenNodeView)
{
	if (Data.Career == nullptr)
	{
		spdlog::error("Cannot open historic race {}: career data is unavailable.", HistoryId);
		return false;
	}
	const Game::Career::CareerHistoricalRace* HistoricalRace =
		Game::Career::FindCareerHistoricalRace(*Data.Career, HistoryId);
	if (HistoricalRace == nullptr)
	{
		spdlog::warn("Historic race {} is no longer available in the bounded race archive.", HistoryId);
		return false;
	}

	std::string Error;
	std::shared_ptr<Game::Career::CareerRaceHistoryReplay> Replay =
		Game::Career::CreateCareerRaceHistoryReplay(*HistoricalRace, Error);
	if (Replay == nullptr)
	{
		spdlog::error("Could not restore historic race {}: {}", HistoryId, Error);
		return false;
	}

	if (OpenNodeView)
		ScreenStack.Push(new NodeViewScreen(ScreenStack, IO, Data, Replay->CompletedRace, Replay, false));
	else
		ScreenStack.Push(new PostRaceScreen(ScreenStack, IO, Data, Replay->CompletedRace, {},
			Game::Race::RacerGUID{}, false, HistoricalRace->EventName, Replay, false));
	return true;
}
