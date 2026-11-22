#ifndef CAREER_H
#define CAREER_H
#include "Core/Competition.h"
#include "Core/WorldClock.h"
#include "RaceState.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace Game
{
	namespace Career
	{
		struct CareerProfileFinancial
		{
			int Cash = 5000;
		};

		struct CareerProfile
		{
			CareerProfile();

			Race::Racer MainFella;
			CareerProfileFinancial Financials;
			std::uint64_t AppraisalIdentity = 0;
			double Accuracy = 0.0;
		};

		enum class CityPlotType
		{
			Scenery,
			VacantLot,
			Gym,
			RaceTrack,
			Bookies,
			LicenseOffice,
			BreedingFarm,
			Stable
		};

		struct CityPlot
		{
			unsigned int GridX = 0;
			unsigned int GridY = 0;
			CityPlotType Type = CityPlotType::Scenery;
			std::string Name;
			unsigned int TrackLength = 0;
			unsigned int SpriteIndex = 0;

			bool IsAvailable() const
			{
				return Type != CityPlotType::Scenery;
			}
		};

		struct CareerPendingBet
		{
			Race::Racer* SelectedRacer = nullptr;
			unsigned int Stake = 0;
			double DecimalOdds = 0.0;
			bool Active = false;
		};

		struct RacePrediction
		{
			Race::RaceError Error = Race::RaceError::None;
			std::vector<double> AverageFinishTimesMs;
			std::vector<double> WinProbabilities;
		};

		RacePrediction PredictRace(const std::vector<Race::Racer*>& Entrants, unsigned int TrackLength,
			unsigned int Seed, std::size_t SampleCount = 32);
		double EstimateRacerStrength(const Race::Racer& Runner, unsigned int TrackLength);

		enum class CareerRacerActivity
		{
			InBuilding,
			Travelling,
			Training,
			InRace,
			Resting,
			WaitingForLeague,
			Retired
		};

		enum class CareerLeagueDivision
		{
			Amateur,
			Division4,
			Division3,
			Division2,
			Division1
		};

		enum class CareerLeagueState
		{
			Filling,
			RegularSeason,
			Playoffs,
			Complete
		};

		enum class CareerFixtureState
		{
			Scheduled,
			InProgress,
			Complete,
			Cancelled
		};

		struct CareerRacerState
		{
			Race::Racer* Runner = nullptr;
			int Cash = 1000;
			int StartingCash = 1000;
			std::uint64_t LeagueId = 0;
			std::size_t LocationPlotIndex = 0;
			std::size_t DestinationPlotIndex = 0;
			std::uint64_t ActionEndsAtRealMs = 0;
			std::uint64_t ActiveRaceId = 0;
			std::uint64_t RetiredAtMs = 0;
			CareerRacerActivity Activity = CareerRacerActivity::InBuilding;
			bool HasRacingLicense = false;
			bool LicenseApplicationInProgress = false;
		};

		struct CareerLeagueFixture
		{
			SportRaceCore::LeagueFixture Fixture;
			CareerFixtureState State = CareerFixtureState::Scheduled;
			std::uint64_t ScheduledAtMs = 0;
			std::uint64_t RaceId = 0;
			std::uint64_t WinnerId = 0;
			std::size_t TrackPlotIndex = 0;
		};

		struct CareerLeagueStanding
		{
			Race::Racer* Runner = nullptr;
			unsigned int Wins = 0;
			unsigned int Draws = 0;
			unsigned int Losses = 0;
			unsigned int Points = 0;
		};

		struct CareerLeague
		{
			std::uint64_t Id = 0;
			std::string Name;
			CareerLeagueDivision Division = CareerLeagueDivision::Amateur;
			std::uint64_t PromotionTargetLeagueId = 0;
			bool IsAmateurCup = false;
			bool IsOpenLobby = false;
			CareerLeagueState State = CareerLeagueState::Filling;
			unsigned int Season = 0;
			std::vector<Race::Racer*> Entrants;
			std::vector<CareerLeagueFixture> Fixtures;
			std::vector<CareerLeagueStanding> Standings;
			SportRaceCore::PlayoffBracket Playoffs;
		};

		struct CareerRaceFixtureReference
		{
			std::uint64_t LeagueId = 0;
			std::size_t FixtureIndex = 0;
		};

		struct CareerPendingLeagueRace
		{
			std::uint64_t LeagueId = 0;
			std::size_t FixtureIndex = 0;
			std::size_t PlayoffRoundIndex = 0;
			std::size_t PlayoffMatchIndex = 0;
			std::size_t TrackPlotIndex = 0;
			Race::RaceFinancials Financials;
			std::vector<Race::Racer*> Entrants;
			std::vector<CareerRaceFixtureReference> LeagueFixtures;
			std::string EventName;
			bool IsPlayoff = false;
			bool Preparing = false;
			bool Active = false;
		};

		struct CareerWorldRace
		{
			std::uint64_t Id = 0;
			std::size_t TrackPlotIndex = 0;
			std::uint64_t LeagueId = 0;
			std::size_t PlayoffRoundIndex = 0;
			std::size_t PlayoffMatchIndex = 0;
			std::uint64_t StartedAtMs = 0;
			std::uint64_t FinishedAtMs = 0;
			std::uint64_t TickAccumulatorMs = 0;
			bool IsPlayoff = false;
			bool IsComplete = false;
			bool IsSettled = false;
			std::string EventName;
			std::vector<Race::Racer*> Entrants;
			std::vector<CareerRaceFixtureReference> LeagueFixtures;
			std::shared_ptr<Race::RaceStateMachine> StateMachine;
		};

		struct CareerData
		{
			CareerData();
			~CareerData();

			Race::RacerDB Racers;
			std::vector<CityPlot> CityPlots;
			std::vector<unsigned int> RaceTrackLengths;
			std::vector<Race::Racer*> OwnedRacers;
			Race::Racer* PlayerLeagueRacer = nullptr;
			std::uint64_t PlayerLeagueId = 0;
			CareerPendingLeagueRace PendingPlayerLeagueRace;
			std::vector<CareerRacerState> RacerStates;
			std::vector<CareerLeague> Leagues;
			std::vector<std::shared_ptr<CareerWorldRace>> WorldRaces;
			std::vector<Race::Racer*> UpcomingRaceEntrants;
			unsigned int UpcomingRaceSeed = 0;
			unsigned int UpcomingRaceTrackLength = 0;
			std::uint64_t NextWorldRaceId = 1;
			std::uint64_t NextLeagueId = 1;
			std::uint64_t NextOpenRaceAtMs = 0;
			std::uint64_t NextAmateurCupAtMs = 0;
			std::size_t TargetAiPopulation = 0;
			std::uint64_t RealTimeMs = 0;
			SportRaceCore::WorldClock WorldClock;
			std::mt19937 WorldRandom;
			bool WorldSimulationInitialized = false;
			bool InitialWorldMonthSimulated = false;
			bool InitialWorldMonthSimulationInProgress = false;
			bool InitialWorldMonthSimulationFailed = false;
			bool InitialWorldMonthClockResetPending = false;
			std::uint64_t InitialWorldMonthTotalRealMs = 0;
			std::uint64_t InitialWorldMonthElapsedRealMs = 0;
			std::uint64_t InitialWorldMonthStartingWorldMs = 0;
			std::uint64_t InitialWorldMonthStartingRealMs = 0;
			std::uint64_t InitialWorldMonthProcessingMs = 0;
			unsigned int BookmakerAccuracy = 98;
			std::array<bool, 4> Licenses{};
			CareerPendingBet PendingBet;
			CareerProfile Profile;
		};

		std::string CareerRacerActivityToString(CareerRacerActivity Activity);
		std::string CareerLeagueDivisionToString(CareerLeagueDivision Division);
		std::string CareerLeagueStateToString(CareerLeagueState State);

		class CareerOrchestrator
		{
		public:
			CareerData* Data;
			CareerProfile* Profile;

			CareerOrchestrator(CareerData*, CareerProfile*);

			void Init();
			void BeginInitialWorldMonthSimulation();
			bool AdvanceInitialWorldMonthSimulation(std::size_t MaximumUpdates);
			void SimulateInitialWorldMonth();
			bool CanJoinLeague(std::uint64_t LeagueId) const;
			bool JoinLeague(std::uint64_t LeagueId);
			bool CanRaceNextPlayerLeagueMatch() const;
			bool PreparePlayerLeagueRace(Race::Race& RaceBuffer);
			void CompletePlayerLeagueRace(const Race::RaceResult& Result);
			std::size_t SpawnRacers(std::size_t Count);
			std::vector<Race::Racer*> SelectMatchedOpponents(std::size_t Count, unsigned int Seed) const;
			bool PrepareUpcomingRace(unsigned int Seed, unsigned int TrackLength = 0);
			Race::Race CreateUpcomingRace() const;
			Race::Racer* BreedOwnedRacers(Race::Racer* FirstParent, Race::Racer* SecondParent,
				unsigned int Seed);
			void CompleteUpcomingRace(const Race::RaceResult& Result);
			void Update(std::uint64_t Ms);
			std::size_t GetActiveRacerCount() const;
			std::size_t GetActiveRaceCount() const;
		};
	}

	
	
}
#endif