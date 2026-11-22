#include "StableScreen.h"
#include "..\App\App.h"

Game::Screens::StableScreen::StableScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO,
	App::AppData& _Data, const Career::CityPlot& _Plot)
	: BuildingScreen(_Machine, _IO, _Data, AppScreenType::StableScreen, _Plot)
{
}

void Game::Screens::StableScreen::UpdateActivity()
{
	for (std::size_t Index = 0; Index < Data.Career->OwnedRacers.size(); ++Index)
		if (Clicked(42.0f, 160.0f + static_cast<float>(Index) * 46.0f, 430.0f, 40.0f))
			SelectedOwnedRacerIndex = Index;
}
