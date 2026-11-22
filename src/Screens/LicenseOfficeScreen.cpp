#include "LicenseOfficeScreen.h"
#include "..\App\App.h"

Game::Screens::LicenseOfficeScreen::LicenseOfficeScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO,
	App::AppData& _Data, const Career::CityPlot& _Plot)
	: BuildingScreen(_Machine, _IO, _Data, AppScreenType::LicenseOfficeScreen, _Plot)
{
}

void Game::Screens::LicenseOfficeScreen::UpdateActivity()
{
	if (Data.Profile == nullptr)
		return;
	for (std::size_t Index = 0; Index < Data.Career->Licenses.size(); ++Index)
		if (Clicked(100.0f, 168.0f + static_cast<float>(Index) * 92.0f, 824.0f, 72.0f))
		{
			if (Data.Career->Licenses[Index])
				StatusMessage = std::string(LicenseName(Index)) + " license already owned.";
			else if (Data.Profile->Financials.Cash < static_cast<int>(LicensePrice(Index)))
				StatusMessage = "Not enough cash for the " + std::string(LicenseName(Index)) + " license.";
			else
			{
				Data.Profile->Financials.Cash -= static_cast<int>(LicensePrice(Index));
				Data.Career->Licenses[Index] = true;
				StatusMessage = std::string(LicenseName(Index)) + " license purchased.";
			}
			break;
		}
}
