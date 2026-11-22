#ifndef BOOKIESSCREEN_H
#define BOOKIESSCREEN_H
#include "BuildingScreen.h"
#include <vector>

namespace Game
{
	namespace Screens
	{
		class BookiesScreen final : public BuildingScreen
		{
			friend class Game::Render::BuildingScreenRenderer;

			std::vector<Race::Racer*> PredictionEntrants;
			std::vector<double> BookieOdds;
			std::vector<double> BookieWinProbabilities;
			unsigned int OddsRaceSeed = 0;
			unsigned int OddsTrackLength = 0;
			unsigned int BetStake = 25;
			std::size_t SelectedRaceEntrant = 0;

			void RefreshBookmakerOdds();
			void UpdateActivity() override;

		public:
			BookiesScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data,
				const Career::CityPlot& _Plot);
		};
	}
}
#endif
