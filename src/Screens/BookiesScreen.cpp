#include "BookiesScreen.h"
#include "SlotMachineScreen.h"
#include "RouletteScreen.h"
#include "..\App\App.h"
#include "..\Audio.h"
#include "SDL3\SDL.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace BookiesScreenDetail
{
	constexpr double BookmakerStrengthTemperature = 12.0;
}
using namespace BookiesScreenDetail;

Game::Screens::BookiesScreen::BookiesScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO,
	App::AppData& _Data, const Career::CityPlot& _Plot)
	: BuildingScreen(_Machine, _IO, _Data, AppScreenType::BookiesScreen, _Plot)
{
	RefreshBookmakerOdds();
}

void Game::Screens::BookiesScreen::RefreshBookmakerOdds()
{
	if (Data.Career == nullptr)
		return;
	if (Data.Career->UpcomingRaceEntrants.empty() &&
		!Orchestrator.PrepareUpcomingRace(SDL_GetTicks()))
	{
		StatusMessage = "The bookie cannot find enough runners for a race.";
		return;
	}
	if (OddsRaceSeed == Data.Career->UpcomingRaceSeed &&
		OddsTrackLength == Data.Career->UpcomingRaceTrackLength &&
		PredictionEntrants == Data.Career->UpcomingRaceEntrants)
		return;

	PredictionEntrants = Data.Career->UpcomingRaceEntrants;
	OddsRaceSeed = Data.Career->UpcomingRaceSeed;
	OddsTrackLength = Data.Career->UpcomingRaceTrackLength;
	Race::RaceError PredictionError = Race::RaceError::None;
	if (OddsTrackLength == 0)
		PredictionError = Race::RaceError::ZeroLengthTrack;
	else if (PredictionEntrants.size() < 2 ||
		std::find(PredictionEntrants.begin(), PredictionEntrants.end(), nullptr) != PredictionEntrants.end())
		PredictionError = Race::RaceError::SimulationFailed;
	if (PredictionError != Race::RaceError::None)
	{
		BookieOdds.clear();
		BookieWinProbabilities.clear();
		StatusMessage = "Odds unavailable: " + Race::RaceErrorToString(PredictionError);
		return;
	}

	std::vector<double> Strengths;
	Strengths.reserve(PredictionEntrants.size());
	for (const Race::Racer* Runner : PredictionEntrants)
		Strengths.push_back(Career::EstimateRacerStrength(*Runner, OddsTrackLength));
	const double StrongestRacer = *std::max_element(Strengths.begin(), Strengths.end());
	std::vector<double> StrengthProbabilities(Strengths.size(), 0.0);
	double StrengthTotal = 0.0;
	for (std::size_t Index = 0; Index < Strengths.size(); ++Index)
	{
		StrengthProbabilities[Index] = std::exp((Strengths[Index] - StrongestRacer) /
			BookmakerStrengthTemperature);
		StrengthTotal += StrengthProbabilities[Index];
	}
	for (double& Probability : StrengthProbabilities)
		Probability /= StrengthTotal;

	BookieWinProbabilities = StrengthProbabilities;
	std::mt19937 Random(OddsRaceSeed ^ Data.Career->BookmakerAccuracy);
	const double Accuracy = std::clamp(static_cast<double>(Data.Career->BookmakerAccuracy) / 100.0, 0.0, 1.0);
	std::normal_distribution<double> AppraisalNoise(0.0, 0.005 + (1.0 - Accuracy) * 0.08);
	std::vector<double> EstimatedProbabilities(StrengthProbabilities.size(), 0.0);
	const double PriorProbability = 1.0 / static_cast<double>(EstimatedProbabilities.size());
	double ProbabilityTotal = 0.0;
	for (std::size_t Index = 0; Index < EstimatedProbabilities.size(); ++Index)
	{
		EstimatedProbabilities[Index] = std::clamp(StrengthProbabilities[Index] * Accuracy +
			PriorProbability * (1.0 - Accuracy) + AppraisalNoise(Random), 0.01, 0.98);
		ProbabilityTotal += EstimatedProbabilities[Index];
	}

	BookieOdds.resize(EstimatedProbabilities.size());
	for (std::size_t Index = 0; Index < EstimatedProbabilities.size(); ++Index)
	{
		const double NormalizedProbability = EstimatedProbabilities[Index] / ProbabilityTotal;
		BookieOdds[Index] = std::clamp(0.90 / NormalizedProbability, 1.05, 25.0);
	}
	StatusMessage.clear();
}

void Game::Screens::BookiesScreen::UpdateActivity()
{
	RefreshBookmakerOdds();
	for (std::size_t Index = 0; Index < PredictionEntrants.size(); ++Index)
		if (Clicked(38.0f, 150.0f + static_cast<float>(Index) * 48.0f, 612.0f, 42.0f))
			SelectedRaceEntrant = Index;

	if (Clicked(720.0f, 190.0f, 100.0f, 44.0f))
		BetStake = 25;
	else if (Clicked(830.0f, 190.0f, 100.0f, 44.0f))
		BetStake = 100;
	else if (Clicked(720.0f, 244.0f, 210.0f, 44.0f))
		BetStake = 250;
	else if (Clicked(720.0f, 310.0f, 210.0f, 52.0f))
	{
		if (Data.Career->PendingBet.Active)
			StatusMessage = "A wager is already placed on this race.";
		else if (Data.Profile == nullptr || SelectedRaceEntrant >= PredictionEntrants.size() ||
			SelectedRaceEntrant >= BookieOdds.size())
			StatusMessage = "Select a runner with available odds first.";
		else if (BetStake > static_cast<unsigned int>(std::max(0, Data.Profile->Financials.Cash)))
			StatusMessage = "You do not have enough cash for that stake.";
		else
		{
			Race::Racer* SelectedRacer = PredictionEntrants[SelectedRaceEntrant];
			Data.Profile->Financials.Cash -= static_cast<int>(BetStake);
			Data.Career->PendingBet = Career::CareerPendingBet{
				SelectedRacer, BetStake, BookieOdds[SelectedRaceEntrant], true
			};
			StatusMessage = "Wager placed on " + SelectedRacer->Name + " at " +
				FormatOdds(BookieOdds[SelectedRaceEntrant]) + " odds.";
		}
	}

	if (Clicked(64.0f, 458.0f, 386.0f, 126.0f))
	{
		IO.Player.Play(Audio::BuiltInSounds::Click);
		ScreenStack.Push(new SlotMachineScreen(ScreenStack, IO, Data, Plot));
		return;
	}
	if (Clicked(482.0f, 458.0f, 482.0f, 126.0f))
	{
		IO.Player.Play(Audio::BuiltInSounds::Click);
		ScreenStack.Push(new RouletteScreen(ScreenStack, IO, Data, Plot));
	}
}

// The Visual C++ project compiles an explicit source list, so this dedicated screen joins an existing unit.
#include "SlotMachineScreen.cpp"
