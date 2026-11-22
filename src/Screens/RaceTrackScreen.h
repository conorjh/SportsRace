#ifndef RACETRACKSCREEN_H
#define RACETRACKSCREEN_H
#include "BuildingScreen.h"
#include <cstddef>
#include <memory>
#include <vector>

namespace Game
{
	namespace Screens
	{
		class RaceTrackScreen final : public BuildingScreen
		{
			friend class Game::Render::BuildingScreenRenderer;

			std::size_t TrackPlotIndex = 0;
			std::size_t RaceScrollOffset = 0;
			float RaceScrollRemainder = 0.0f;
			void UpdateActivity() override;

		public:
			static constexpr std::size_t VisibleRaceCount = 8;

			RaceTrackScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data,
				const Career::CityPlot& _Plot);
			std::vector<std::shared_ptr<Career::CareerWorldRace>> GetActiveWorldRaces() const;
		};
	}
}
#endif
