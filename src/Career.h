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

		struct CareerWorldCity
		{
			std::uint64_t Id = 0;
			std::uint64_t RegionId = 0;
			std::string Name;
			std::uint64_t Population = 0;
			float MapX = 0.0f;
			float MapY = 0.0f;
			bool IsCapital = false;
			std::vector<CityPlot> Plots;
		};

		struct CareerWorldRegion
		{
			std::uint64_t Id = 0;
			std::uint64_t CountryId = 0;
			std::uint64_t CapitalCityId = 0;
			std::string Name;
			std::uint64_t Population = 0;
			float MapX = 0.0f;
			float MapY = 0.0f;
			std::vector<std::uint64_t> CityIds;
		};

		struct CareerWorldCountry
		{
			std::uint64_t Id = 0;
			std::uint64_t CapitalRegionId = 0;
			std::string Name;
			std::uint64_t Population = 0;
			std::vector<std::uint64_t> RegionIds;
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

		enum class CareerCompetitionLevel
		{
			City,
			Regional,
			National
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
			std::uint64_t CityId = 0;
			int Cash = 1000;
			int StartingCash = 1000;
			std::uint64_t LeagueId = 0;
			std::size_t LocationPlotIndex = 0;
			std::size_t DestinationPlotIndex = 0;
			std::uint64_t ActionEndsAtRealMs = 0;
			std::uint64_t RaceCooldownEndsAtRealMs = 0;
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
			std::uint64_t HistoricalRaceId = 0;
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
			std::uint64_t CountryId = 0;
			std::uint64_t CityId = 0;
			std::uint64_t RegionId = 0;
			std::string Name;
			CareerCompetitionLevel Level = CareerCompetitionLevel::City;
			CareerLeagueDivision Division = CareerLeagueDivision::Amateur;
			std::uint64_t PromotionTargetLeagueId = 0;
			std::size_t PromotionSlots = 2;
			std::size_t IncomingPromotionLimit = 2;
			bool IsCup = false;
			bool IsAmateurCup = false;
			bool IsOpenLobby = false;
			CareerLeagueState State = CareerLeagueState::Filling;
			unsigned int Season = 0;
			std::vector<Race::Racer*> Entrants;
			std::vector<CareerLeagueFixture> Fixtures;
			std::vector<CareerLeagueStanding> Standings;
			SportRaceCore::PlayoffBracket Playoffs;
		};

		struct CareerHistoricalRace
		{
			std::uint64_t Id = 0;
			std::uint64_t WorldTimeMs = 0;
			std::uint64_t WorldRaceId = 0;
			std::uint64_t LeagueId = 0;
			std::size_t PlayoffRoundIndex = 0;
			std::size_t PlayoffMatchIndex = 0;
			std::string EventName;
			bool IsPlayoff = false;
			Race::RaceHistorySnapshot Snapshot;
		};

		struct CareerRaceHistoryReplay
		{
			explicit CareerRaceHistoryReplay(unsigned int Seed)
				: CompletedRace(Seed)
			{
			}

			std::vector<std::unique_ptr<Race::Racer>> Entrants;
			Race::Race CompletedRace;
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

		struct CareerPendingOpenRace
		{
			std::size_t TrackPlotIndex = 0;
			std::vector<Race::Racer*> Entrants;
		};

		struct CareerPendingWorldLeagueRace
		{
			std::size_t TrackPlotIndex = 0;
			std::uint64_t LeagueId = 0;
			std::string EventName;
			std::vector<Race::Racer*> Entrants;
			std::vector<CareerRaceFixtureReference> LeagueFixtures;
		};

		struct CareerPendingWorldPlayoffRace
		{
			std::size_t TrackPlotIndex = 0;
			std::uint64_t LeagueId = 0;
			std::size_t PlayoffRoundIndex = 0;
			std::size_t PlayoffMatchIndex = 0;
			std::string EventName;
			std::vector<Race::Racer*> Entrants;
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
			std::vector<CareerWorldCity> Cities;
			std::vector<CareerWorldRegion> Regions;
			std::vector<CareerWorldCountry> Countries;
			std::vector<CareerHistoricalRace> HistoricalRaces;
			std::uint64_t HomeCityId = 0;
			std::vector<unsigned int> RaceTrackLengths;
			std::vector<Race::Racer*> OwnedRacers;
			Race::Racer* PlayerLeagueRacer = nullptr;
			std::uint64_t PlayerLeagueId = 0;
			CareerPendingLeagueRace PendingPlayerLeagueRace;
			std::vector<CareerRacerState> RacerStates;
			std::vector<CareerLeague> Leagues;
			std::vector<CareerPendingOpenRace> PendingOpenRaces;
			std::vector<CareerPendingWorldLeagueRace> PendingWorldLeagueRaces;
			std::vector<CareerPendingWorldPlayoffRace> PendingWorldPlayoffRaces;
			std::vector<std::shared_ptr<CareerWorldRace>> WorldRaces;
			std::vector<Race::Racer*> UpcomingRaceEntrants;
			unsigned int UpcomingRaceSeed = 0;
			unsigned int UpcomingRaceTrackLength = 0;
			std::uint64_t NextWorldRaceId = 1;
			std::uint64_t NextCareerRaceHistoryId = 1;
			std::uint64_t NextLeagueId = 1;
			std::uint64_t NextOpenRaceAtMs = 0;
			std::size_t NextOpenRaceTrackIndex = 0;
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

		inline constexpr std::size_t MaximumCareerHistoricalRaces = 128;
		inline constexpr std::uint32_t CurrentCareerSaveFormatVersion = 3; // Bump whenever the serialized layout changes.
		inline constexpr char CareerSaveFilePath[] = "saves/career.srsave";

		bool IsLeagueAtStableLocation(const CareerData& Data, const CareerLeague& League);
		CareerHistoricalRace* FindCareerHistoricalRace(CareerData& Data, std::uint64_t HistoryId);
		const CareerHistoricalRace* FindCareerHistoricalRace(const CareerData& Data, std::uint64_t HistoryId);
		bool RecordCareerRaceHistory(CareerData& Data, const Race::Race& CompletedRace, const std::string& EventName,
			std::uint64_t WorldTimeMs, std::uint64_t& HistoryId, std::string& Error);
		std::shared_ptr<CareerRaceHistoryReplay> CreateCareerRaceHistoryReplay(
			const CareerHistoricalRace& HistoricalRace, std::string& Error);

		bool SaveCareer(const CareerData& Data, const CareerProfile& Profile, const std::string& FilePath,
			std::string& Error);
		bool LoadCareer(const std::string& FilePath, std::unique_ptr<CareerData>& LoadedData,
			std::unique_ptr<CareerProfile>& LoadedProfile, std::string& Error);
		void RebindCareerProfileRacerReferences(CareerData& Data, const CareerProfile& OldProfile,
			CareerProfile& NewProfile);

		std::string CareerRacerActivityToString(CareerRacerActivity Activity);
		std::string CareerLeagueDivisionToString(CareerLeagueDivision Division);
		std::string CareerCompetitionLevelToString(CareerCompetitionLevel Level);
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
			void CompletePlayerLeagueRace(const Race::RaceResult& Result, std::uint64_t HistoricalRaceId = 0);
			std::size_t SpawnRacers(std::size_t Count);
			std::vector<Race::Racer*> SelectMatchedOpponents(std::size_t Count, unsigned int Seed) const;
			bool PrepareUpcomingRace(unsigned int Seed, unsigned int TrackLength = 0);
			Race::Race CreateUpcomingRace() const;
			Race::Racer* BreedOwnedRacers(Race::Racer* FirstParent, Race::Racer* SecondParent,
				unsigned int Seed);
			void CompleteUpcomingRace(const Race::RaceResult& Result, std::uint64_t HistoricalRaceId = 0);
			void Update(std::uint64_t Ms);
			std::size_t GetActiveRacerCount() const;
			std::size_t GetActiveRaceCount() const;
		};
	}

	
	
}
#endif