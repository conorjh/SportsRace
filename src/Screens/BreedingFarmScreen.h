#ifndef BREEDINGFARMSCREEN_H
#define BREEDINGFARMSCREEN_H
#include "BuildingScreen.h"

namespace Game
{
	namespace Screens
	{
		class BreedingFarmScreen final : public BuildingScreen
		{
			friend class Game::Render::BuildingScreenRenderer;

			std::size_t FirstParentIndex = 0;
			std::size_t SecondParentIndex = 1;

			void UpdateActivity() override;

		public:
			BreedingFarmScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data,
				const Career::CityPlot& _Plot);
		};
	}
}
#endif
