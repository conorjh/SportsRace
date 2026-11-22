#ifndef LICENSEOFFICESCREEN_H
#define LICENSEOFFICESCREEN_H
#include "BuildingScreen.h"

namespace Game
{
	namespace Screens
	{
		class LicenseOfficeScreen final : public BuildingScreen
		{
			friend class Game::Render::BuildingScreenRenderer;

			void UpdateActivity() override;

		public:
			LicenseOfficeScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data,
				const Career::CityPlot& _Plot);
		};
	}
}
#endif
