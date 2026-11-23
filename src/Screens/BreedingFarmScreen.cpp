#include "BreedingFarmScreen.h"
#include "..\App\App.h"
#include "SDL3\SDL.h"

namespace BreedingFarmScreenDetail
{
	constexpr unsigned int BreedingCost = 1000;
}
using namespace BreedingFarmScreenDetail;

Game::Screens::BreedingFarmScreen::BreedingFarmScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO,
	App::AppData& _Data, const Career::CityPlot& _Plot)
	: BuildingScreen(_Machine, _IO, _Data, AppScreenType::BreedingFarmScreen, _Plot)
{
}

void Game::Screens::BreedingFarmScreen::UpdateActivity()
{
	const std::vector<Race::Racer*>& OwnedRacers = Data.Career->OwnedRacers;
	for (std::size_t Index = 0; Index < OwnedRacers.size(); ++Index)
	{
		const float Y = 160.0f + static_cast<float>(Index) * 46.0f;
		if (Clicked(42.0f, Y, 430.0f, 40.0f))
			FirstParentIndex = Index;
		else if (Clicked(522.0f, Y, 430.0f, 40.0f))
			SecondParentIndex = Index;
	}
	if (Clicked(680.0f, 596.0f, 230.0f, 56.0f))
	{
		if (!Data.Career->Licenses[1])
			StatusMessage = "A breeding license is required.";
		else if (Data.Profile == nullptr || OwnedRacers.size() < 2 || FirstParentIndex >= OwnedRacers.size() ||
			SecondParentIndex >= OwnedRacers.size() || FirstParentIndex == SecondParentIndex)
			StatusMessage = "Choose two different owned racers.";
		else if (Data.Profile->Financials.Cash < static_cast<int>(BreedingCost))
			StatusMessage = "Breeding costs $1,000; you do not have enough cash.";
		else
		{
			Race::Racer* Child = Orchestrator.BreedOwnedRacers(OwnedRacers[FirstParentIndex],
				OwnedRacers[SecondParentIndex], SDL_GetTicks());
			if (Child == nullptr)
				StatusMessage = "The selected racers cannot be bred.";
			else
			{
				Data.Profile->Financials.Cash -= static_cast<int>(BreedingCost);
				StatusMessage = Child->Name + " has joined your stable.";
			}
		}
	}
}
