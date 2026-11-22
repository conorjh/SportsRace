#include "RaceTrackScreen.h"
#include "RaceScreen.h"
#include "..\App\App.h"
#include "..\Audio.h"
#include "SDL3\SDL.h"
#include <algorithm>
#include <limits>

Game::Screens::RaceTrackScreen::RaceTrackScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO,
	App::AppData& _Data, const Career::CityPlot& _Plot)
	: BuildingScreen(_Machine, _IO, _Data, AppScreenType::RaceTrackScreen, _Plot)
{
	if (Data.Career != nullptr)
	{
		const auto PlotIt = std::find_if(Data.Career->CityPlots.begin(), Data.Career->CityPlots.end(),
			[this](const Career::CityPlot& Candidate)
			{
				return Candidate.GridX == this->Plot.GridX && Candidate.GridY == this->Plot.GridY &&
					Candidate.Type == Career::CityPlotType::RaceTrack;
			});
		if (PlotIt != Data.Career->CityPlots.end())
			TrackPlotIndex = static_cast<std::size_t>(PlotIt - Data.Career->CityPlots.begin());
		else
		{
			TrackPlotIndex = std::numeric_limits<std::size_t>::max();
			StatusMessage = "Could not locate this track in the career world.";
		}
	}
}

std::vector<std::shared_ptr<Game::Career::CareerWorldRace>>
Game::Screens::RaceTrackScreen::GetActiveWorldRaces() const
{
	std::vector<std::shared_ptr<Career::CareerWorldRace>> ActiveRaces;
	if (Data.Career == nullptr || TrackPlotIndex >= Data.Career->CityPlots.size())
		return ActiveRaces;
	for (const std::shared_ptr<Career::CareerWorldRace>& WorldRace : Data.Career->WorldRaces)
		if (WorldRace != nullptr && !WorldRace->IsComplete && WorldRace->TrackPlotIndex == TrackPlotIndex)
			ActiveRaces.push_back(WorldRace);
	std::sort(ActiveRaces.begin(), ActiveRaces.end(), [](const auto& Left, const auto& Right)
		{
			return Left->StartedAtMs < Right->StartedAtMs;
		});
	return ActiveRaces;
}

void Game::Screens::RaceTrackScreen::UpdateActivity()
{
	if (Data.Career == nullptr)
		return;
	const std::vector<std::shared_ptr<Career::CareerWorldRace>> ActiveRaces = GetActiveWorldRaces();
	const std::size_t MaximumScrollOffset = ActiveRaces.size() > VisibleRaceCount
		? ActiveRaces.size() - VisibleRaceCount : 0;
	RaceScrollOffset = std::min(RaceScrollOffset, MaximumScrollOffset);
	if (IO.MouseWheelY != 0.0f && IO.MousePosition.x >= 62.0f && IO.MousePosition.x <= 962.0f &&
		IO.MousePosition.y >= 166.0f && IO.MousePosition.y < 566.0f)
	{
		RaceScrollRemainder -= IO.MouseWheelY * 3.0f;
		const int RowDelta = static_cast<int>(RaceScrollRemainder);
		if (RowDelta != 0)
		{
			const long long NewOffset = static_cast<long long>(RaceScrollOffset) + RowDelta;
			RaceScrollOffset = static_cast<std::size_t>(std::clamp<long long>(NewOffset, 0,
				static_cast<long long>(MaximumScrollOffset)));
			RaceScrollRemainder -= static_cast<float>(RowDelta);
		}
	}

	for (std::size_t Row = 0; Row < VisibleRaceCount; ++Row)
	{
		const std::size_t Index = RaceScrollOffset + Row;
		if (Index >= ActiveRaces.size())
			break;
		if (Clicked(62.0f, 166.0f + static_cast<float>(Row) * 50.0f, 900.0f, 42.0f))
		{
			if (ActiveRaces[Index]->StateMachine == nullptr)
				StatusMessage = "The selected world race has no state machine.";
			else
			{
				IO.Player.Play(Audio::BuiltInSounds::Click);
				ScreenStack.Push(new RaceScreen(ScreenStack, IO, Data, ActiveRaces[Index]));
			}
			return;
		}
	}

	if (Data.Career->UpcomingRaceEntrants.empty())
		Orchestrator.PrepareUpcomingRace(SDL_GetTicks(), Plot.TrackLength);
	else if (!Data.Career->PendingBet.Active)
		Orchestrator.PrepareUpcomingRace(Data.Career->UpcomingRaceSeed, Plot.TrackLength);

	if (Clicked(590.0f, 596.0f, 300.0f, 56.0f))
	{
		if (Data.Profile == nullptr)
			StatusMessage = "Player profile data is unavailable.";
		else if (!Orchestrator.PrepareUpcomingRace(SDL_GetTicks(), Plot.TrackLength))
			StatusMessage = "Not enough runners are available to start a race.";
		else
		{
			Race::Race RaceToRun = Orchestrator.CreateUpcomingRace();
			if (RaceToRun.Racers.size() < 2)
				StatusMessage = "Not enough runners are available to start a race.";
			else
			{
				RaceToRun.Financials = Race::RaceFinancials(500, 0);
				IO.Player.Play(Audio::BuiltInSounds::Click);
				ScreenStack.Push(new RaceScreen(ScreenStack, IO, Data, RaceToRun, Data.Profile->MainFella.GUID));
				return;
			}
		}
	}
}
