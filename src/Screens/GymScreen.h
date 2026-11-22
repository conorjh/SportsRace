#ifndef GYMSCREEN_H
#define GYMSCREEN_H
#include "BuildingScreen.h"
#include <array>

namespace Game
{
	namespace Screens
	{
		class GymScreen final : public BuildingScreen
		{
			friend class Game::Render::BuildingScreenRenderer;

			enum class GymActivity
			{
				None,
				Treadmill,
				Weights,
				Dance
			};

			static constexpr unsigned int WorkoutAttemptCount = 5;
			static constexpr unsigned int DanceMoveCount = 5;
			std::size_t SelectedOwnedRacerIndex = 0;
			GymActivity ActiveGymActivity = GymActivity::None;
			std::array<unsigned int, DanceMoveCount> DanceSequence{};
			std::size_t DanceProgress = 0;
			unsigned int WorkoutAttempts = 0;
			unsigned int WorkoutHits = 0;
			unsigned int WorkoutStartedAt = 0;

			void StartGymActivity(GymActivity Activity);
			void FinishWorkout();
			void UpdateActivity() override;

		public:
			GymScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data,
				const Career::CityPlot& _Plot);
		};
	}
}
#endif
