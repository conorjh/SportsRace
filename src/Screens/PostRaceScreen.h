#ifndef POSTRACESCREEN_H
#define POSTRACESCREEN_H
#include "..\Race.h"
#include "..\Render\Renderer.h"
#include "..\Screens.h"
#include "..\Util.h"
#include <cstdint>
#include <string>
#include <vector>

namespace Game
{
	namespace Render
	{
		class PostRaceRenderer;
	}

	namespace Screens
	{
		struct RacerProgressSnapshot
		{
			Race::Racer* Runner = nullptr;
			std::uint64_t Xp = 0;
			std::uint64_t Wins = 0;
			std::uint64_t Losses = 0;
			std::uint64_t RacesEntered = 0;
			std::uint64_t PositionPoints = 0;
			unsigned int BestTimeMs = 0;
			double Reflex = 0.0;
			double Composure = 0.0;
			double Acceleration = 0.0;
			double Endurance = 0.0;
			double Speed = 0.0;
		};

		class PostRaceScreen final : public AppScreen
		{
			friend class Game::Render::PostRaceRenderer;

			const Race::Race* CompletedRace;
			std::vector<RacerProgressSnapshot> PreRaceProgress;
			std::vector<const Race::RacerRaceResult*> OrderedResults;
			std::size_t SelectedResultIndex = 0;
			std::string EventName;
			Race::RacerGUID PlayerGUID;
			bool PlayerRacerSupplied = false;

		public:
			GUI::Button PreviousRacerButton;
			GUI::Button NextRacerButton;
			GUI::Button ContinueButton;

			PostRaceScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data,
				const Race::Race& RaceToReview, std::vector<RacerProgressSnapshot> _PreRaceProgress,
				Race::RacerGUID _PlayerGUID, bool _PlayerRacerSupplied, std::string _EventName);

			AppScreen* Update() override;
			const Race::Race& GetCompletedRace() const;
			const Race::RacerRaceResult* GetPlayerResult() const;
			const Race::RacerRaceResult* GetSelectedResult() const;
			const Race::RacerRaceResult* GetResultAt(std::size_t Index) const;
			const RacerProgressSnapshot* GetProgressSnapshot(const Race::Racer* Runner) const;
			std::size_t GetResultCount() const;
			std::size_t GetSelectedResultIndex() const;
			const std::string& GetEventName() const;
			bool HasPlayerRacer() const;
		};
	}

	namespace Render
	{
		class PostRaceRenderer final : public BaseRenderer
		{
		public:
			Screens::PostRaceScreen* Screen = nullptr;

			PostRaceRenderer(AppRenderContext* Context);
			unsigned int Render();
		};
	}
}

#endif