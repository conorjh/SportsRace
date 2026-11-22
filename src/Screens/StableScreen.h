#ifndef STABLESCREEN_H
#define STABLESCREEN_H
#include "BuildingScreen.h"

namespace Game
{
	namespace Screens
	{
		class StableScreen final : public BuildingScreen
		{
			friend class Game::Render::BuildingScreenRenderer;

			std::size_t SelectedOwnedRacerIndex = 0;

			void UpdateActivity() override;

		public:
			StableScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data,
				const Career::CityPlot& _Plot);
		};
	}
}
#endif
