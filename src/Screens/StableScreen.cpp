#include "StableScreen.h"
#include "..\App\App.h"

Game::Screens::StableScreen::StableScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO,
	App::AppData& _Data, const Career::CityPlot& _Plot, std::uint64_t _CityId, std::size_t _PlotIndex)
	: BuildingScreen(_Machine, _IO, _Data, AppScreenType::StableScreen, _Plot),
	CityId(_CityId),
	PlotIndex(_PlotIndex),
	RentStableButton(_IO, "Rent stable space", 652, 274, 270, 56)
{
}

void Game::Screens::StableScreen::UpdateActivity()
{
	if (IsCurrentStable())
	{
		for (std::size_t Index = 0; Index < Data.Career->OwnedRacers.size(); ++Index)
			if (Clicked(42.0f, 160.0f + static_cast<float>(Index) * 46.0f, 430.0f, 40.0f))
				SelectedOwnedRacerIndex = Index;
		return;
	}

	RentStableButton.SetBounds(MapX(652.0f), MapY(274.0f), 270, 56);
	RentStableButton.Update();
	if (RentStableButton.HasMouseClicked())
	{
		if (Orchestrator.RentStableSpace(CityId, PlotIndex))
			StatusMessage = "Stable space rented. Buy your first racer from the breeder.";
		else if (Data.Profile == nullptr || Data.Profile->Financials.Cash < Career::CareerStableLeasePrice)
			StatusMessage = "Renting stable space costs $" + std::to_string(Career::CareerStableLeasePrice) + ".";
		else
			StatusMessage = "You already have a stable lease or this location is unavailable.";
	}
}

bool Game::Screens::StableScreen::IsCurrentStable() const
{
	return Data.Career != nullptr && Data.Career->PlayerStableCityId == CityId &&
		Data.Career->PlayerStablePlotIndex == PlotIndex;
}

bool Game::Screens::StableScreen::CanRentSpace() const
{
	return Orchestrator.CanRentStableSpace(CityId, PlotIndex);
}

const Game::GUI::Button& Game::Screens::StableScreen::GetRentStableButton() const
{
	return RentStableButton;
}
