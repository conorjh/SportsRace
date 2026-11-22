#include "GymScreen.h"
#include "..\App\App.h"
#include "SDL3\SDL.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <random>

Game::Screens::GymScreen::GymScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO,
	App::AppData& _Data, const Career::CityPlot& _Plot)
	: BuildingScreen(_Machine, _IO, _Data, AppScreenType::GymScreen, _Plot)
{
}

void Game::Screens::GymScreen::UpdateActivity()
{
	const std::vector<Race::Racer*>& OwnedRacers = Data.Career->OwnedRacers;
	for (std::size_t Index = 0; Index < OwnedRacers.size(); ++Index)
		if (Clicked(38.0f, 160.0f + static_cast<float>(Index) * 46.0f, 300.0f, 40.0f))
			SelectedOwnedRacerIndex = Index;

	if (Clicked(382.0f, 160.0f, 180.0f, 60.0f))
		StartGymActivity(GymActivity::Treadmill);
	else if (Clicked(576.0f, 160.0f, 180.0f, 60.0f))
		StartGymActivity(GymActivity::Weights);
	else if (Clicked(770.0f, 160.0f, 180.0f, 60.0f))
		StartGymActivity(GymActivity::Dance);

	if ((ActiveGymActivity == GymActivity::Treadmill || ActiveGymActivity == GymActivity::Weights) &&
		Clicked(622.0f, 510.0f, 210.0f, 54.0f))
	{
		const unsigned int Elapsed = SDL_GetTicks() - WorkoutStartedAt;
		const double Position = std::fmod(static_cast<double>(Elapsed), 1200.0) / 12.0;
		const bool Hit = ActiveGymActivity == GymActivity::Treadmill
			? Position >= 35.0 && Position <= 65.0 : Position >= 46.0 && Position <= 56.0;
		++WorkoutAttempts;
		if (Hit)
			++WorkoutHits;
		StatusMessage = Hit ? "Good timing." : "Missed the training zone.";
		if (WorkoutAttempts >= WorkoutAttemptCount)
			FinishWorkout();
	}
	else if (ActiveGymActivity == GymActivity::Dance)
	{
		constexpr std::array<SDL_FRect, 4> DirectionButtons{{
			{ 520.0f, 478.0f, 78.0f, 52.0f }, { 606.0f, 478.0f, 78.0f, 52.0f },
			{ 692.0f, 478.0f, 78.0f, 52.0f }, { 778.0f, 478.0f, 78.0f, 52.0f }
		}};
		for (unsigned int Direction = 0; Direction < DirectionButtons.size(); ++Direction)
			if (Clicked(DirectionButtons[Direction].x, DirectionButtons[Direction].y,
				DirectionButtons[Direction].w, DirectionButtons[Direction].h))
			{
				if (Direction == DanceSequence[DanceProgress])
				{
					++DanceProgress;
					if (DanceProgress >= DanceMoveCount)
					{
						if (SelectedOwnedRacerIndex < OwnedRacers.size() &&
							OwnedRacers[SelectedOwnedRacerIndex] != nullptr)
						{
							Race::Racer& Runner = *OwnedRacers[SelectedOwnedRacerIndex];
							Runner.Skills.Reflex = std::min(100.0, Runner.Skills.Reflex + 1.0);
							Runner.Skills.Composure = std::min(100.0, Runner.Skills.Composure + 1.0);
							Runner.Skills.Acceleration = std::min(100.0, Runner.Skills.Acceleration + 1.0);
							Runner.Skills.Endurance = std::min(100.0, Runner.Skills.Endurance + 1.0);
							Runner.Skills.Speed = std::min(100.0, Runner.Skills.Speed + 1.0);
							Runner.Conditions.Fatigue = std::clamp(Runner.Conditions.Fatigue + 0.05, 0.0, 1.0);
							StatusMessage = "Perfect routine. All five skills improved by 1.";
						}
						ActiveGymActivity = GymActivity::None;
					}
				}
				else
				{
					DanceProgress = 0;
					StatusMessage = "Wrong step; the dance sequence has reset.";
				}
				break;
	}
}
}

void Game::Screens::GymScreen::StartGymActivity(GymActivity Activity)
{
	if (Data.Career == nullptr || Data.Career->OwnedRacers.empty())
	{
		StatusMessage = "You need an owned runner before training.";
		return;
	}
	ActiveGymActivity = Activity;
	WorkoutAttempts = 0;
	WorkoutHits = 0;
	WorkoutStartedAt = SDL_GetTicks();
	DanceProgress = 0;
	if (Activity == GymActivity::Dance)
	{
		std::mt19937 Random(WorkoutStartedAt ^ 0x27d4eb2du);
		std::uniform_int_distribution<unsigned int> DirectionDistribution(0, 3);
		for (unsigned int& Direction : DanceSequence)
			Direction = DirectionDistribution(Random);
	}
	StatusMessage = Activity == GymActivity::Treadmill ? "Tap Run when the marker reaches the green zone."
		: Activity == GymActivity::Weights ? "Tap Lift when the marker reaches the narrow green zone."
			: "Repeat the five-step dance sequence using the direction buttons.";
}

void Game::Screens::GymScreen::FinishWorkout()
{
	if (Data.Career == nullptr || SelectedOwnedRacerIndex >= Data.Career->OwnedRacers.size() ||
		Data.Career->OwnedRacers[SelectedOwnedRacerIndex] == nullptr)
	{
		StatusMessage = "Select a runner before training.";
		ActiveGymActivity = GymActivity::None;
		return;
	}

	Race::Racer& Runner = *Data.Career->OwnedRacers[SelectedOwnedRacerIndex];
	const double Gain = static_cast<double>(WorkoutHits) * 0.2;
	if (ActiveGymActivity == GymActivity::Treadmill)
		Runner.Skills.Endurance = std::min(100.0, Runner.Skills.Endurance + Gain);
	else if (ActiveGymActivity == GymActivity::Weights)
		Runner.Skills.Acceleration = std::min(100.0, Runner.Skills.Acceleration + Gain);
	Runner.Conditions.Fatigue = std::clamp(Runner.Conditions.Fatigue + WorkoutHits * 0.02, 0.0, 1.0);
	StatusMessage = "Workout complete: " + std::to_string(WorkoutHits) + " / " +
		std::to_string(WorkoutAttemptCount) + " good reps; skill gain " + FormatOdds(Gain) + ".";
	ActiveGymActivity = GymActivity::None;
}
