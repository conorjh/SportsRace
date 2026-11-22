#include "Career.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <execution>
#include <limits>
#include <numeric>
#include <string>
#include <unordered_set>
#include <utility>
#include "spdlog/spdlog.h"

namespace
{
	using Game::Career::CareerData;
	using Game::Career::CareerFixtureState;
	using Game::Career::CareerLeague;
	using Game::Career::CareerLeagueDivision;
	using Game::Career::CareerLeagueFixture;
	using Game::Career::CareerLeagueStanding;
	using Game::Career::CareerLeagueState;
	using Game::Career::CareerPendingLeagueRace;
	using Game::Career::CareerRacerActivity;
	using Game::Career::CareerRacerState;
	using Game::Career::CareerRaceFixtureReference;
	using Game::Career::CareerWorldRace;
	using Game::Race::Race;
	using Game::Race::RaceResult;
	using Game::Race::RacerCareerHistoryEntry;
	using Game::Race::Racer;

	constexpr unsigned int CareerCityGridWidth = 8;
	constexpr unsigned int CareerCityGridHeight = 5;
	constexpr unsigned int CareerCitySpriteCount = 12;
	constexpr std::size_t CareerStarterOwnedRacerCount = 2;
	constexpr std::size_t CareerTargetPopulation = 256;
	constexpr std::size_t CareerLeagueSize = 20;
	constexpr std::uint64_t CareerAmateurCupId = std::uint64_t{ 1 } << 63;
	constexpr std::size_t CareerMinimumRaceTrackCount = 5;
	constexpr std::size_t CareerRacersPerRaceTrack = 20;
	constexpr std::size_t CareerAmateurLeagueCount = 1;
	constexpr std::size_t CareerFixturesPerWorldRace = 3;
	constexpr std::size_t CareerWorldRaceEntrantCount = 6;
	constexpr std::size_t CareerWorldRaceHistoryLimit = 1024;
	constexpr int CareerStartingCash = 1000;
	constexpr int CareerLicenseSavingsTarget = CareerStartingCash * 2;
	constexpr int CareerRacingLicensePrice = CareerStartingCash;
	constexpr int CareerRaceEntryFee = 50;
	constexpr unsigned int CareerRaceFirstPrize = 100;
	constexpr int CareerTrainingFee = 10;
	constexpr std::uint64_t CareerLeagueFirstFixtureDelayMs = 1000;
	constexpr std::uint64_t CareerLeagueRoundIntervalMs = 120000;
	constexpr std::uint64_t CareerAmateurCupIntervalMs = 24ull * 60 * 60 * 1000;
	constexpr std::uint64_t CareerOpenRaceIntervalMs = 30000;
	constexpr std::uint64_t CareerTrainingDurationMs = 20000;
	constexpr std::uint64_t CareerRecoveryIntervalMs = 10000;
	constexpr std::uint64_t CareerRaceRetryDelayMs = 5000;
	constexpr std::uint64_t CareerTravelBaseMs = 1000;
	constexpr std::uint64_t CareerTravelPerGridStepMs = 1500;
	constexpr std::uint64_t CareerPostRaceRestMs = 5000;
	constexpr std::uint64_t CareerInitialWorldMonthMs = 30ull * 24 * 60 * 60 * 1000;
	constexpr std::uint64_t CareerInitialWorldSimulationStepMs = 1000;
	constexpr std::uint64_t CareerInitialWorldHistoryCatchupLimitMs = 30ull * 60 * 1000;
	constexpr double CareerTrainingFatigueThreshold = 0.35;
	constexpr double CareerTrainingInjuryThreshold = 0.15;
	constexpr double CareerTrainingFatigueRecovery = 0.35;
	constexpr double CareerTrainingInjuryRecovery = 0.05;
	constexpr double CareerPassiveFatigueRecovery = 0.02;
	constexpr double CareerPassiveInjuryRecovery = 0.01;

	constexpr std::array<unsigned int, 5> CareerTrackLengths{
		static_cast<unsigned int>(Game::Race::TrackLength::Standard100m),
		static_cast<unsigned int>(Game::Race::TrackLength::Standard200m),
		static_cast<unsigned int>(Game::Race::TrackLength::Standard250m),
		static_cast<unsigned int>(Game::Race::TrackLength::Standard500m),
		static_cast<unsigned int>(Game::Race::TrackLength::Standard1000m)
	};

	constexpr std::array<const char*, 8> AmateurLeagueNames{
		"Yorkshire Amateur League", "Lancashire Amateur League", "Northumbria Club League",
		"Merseyside Amateur League", "West Midlands Amateur League", "East Anglia Amateur League",
		"South Coast Amateur League", "Welsh Borders Amateur League"
	};

	constexpr std::size_t InvalidPlotIndex = std::numeric_limits<std::size_t>::max();

	void GenerateCity(CareerData& Data, std::mt19937& Random)
	{
		if (!Data.CityPlots.empty())
			return;

		Data.CityPlots.reserve(CareerCityGridWidth * CareerCityGridHeight);
		std::uniform_int_distribution<unsigned int> SpriteDistribution(0, CareerCitySpriteCount - 1);
		for (unsigned int GridY = 0; GridY < CareerCityGridHeight; ++GridY)
			for (unsigned int GridX = 0; GridX < CareerCityGridWidth; ++GridX)
			{
				Game::Career::CityPlot Plot;
				Plot.GridX = GridX;
				Plot.GridY = GridY;
				Plot.Name = "City scenery";
				Plot.SpriteIndex = SpriteDistribution(Random);
				Data.CityPlots.push_back(std::move(Plot));
			}

		std::vector<std::size_t> AvailablePlotIndices(Data.CityPlots.size());
		std::iota(AvailablePlotIndices.begin(), AvailablePlotIndices.end(), 0);
		std::shuffle(AvailablePlotIndices.begin(), AvailablePlotIndices.end(), Random);
		const std::array<Game::Career::CityPlotType, 8> BuildingTypes{
			Game::Career::CityPlotType::Gym,
			Game::Career::CityPlotType::RaceTrack,
			Game::Career::CityPlotType::Bookies,
			Game::Career::CityPlotType::LicenseOffice,
			Game::Career::CityPlotType::BreedingFarm,
			Game::Career::CityPlotType::Stable,
			Game::Career::CityPlotType::RaceTrack,
			Game::Career::CityPlotType::RaceTrack
		};
		std::uniform_int_distribution<std::size_t> TrackDistribution(0, CareerTrackLengths.size() - 1);
		std::size_t TrackNumber = 0;
		for (std::size_t BuildingIndex = 0; BuildingIndex < BuildingTypes.size(); ++BuildingIndex)
		{
			Game::Career::CityPlot& Plot = Data.CityPlots[AvailablePlotIndices[BuildingIndex]];
			Plot.Type = BuildingTypes[BuildingIndex];
			switch (Plot.Type)
			{
			case Game::Career::CityPlotType::Gym:
				Plot.Name = "Municipal Gym";
				break;
			case Game::Career::CityPlotType::RaceTrack:
				Plot.Name = "City Track " + std::to_string(++TrackNumber);
				Plot.TrackLength = CareerTrackLengths[TrackDistribution(Random)];
				Data.RaceTrackLengths.push_back(Plot.TrackLength);
				break;
			case Game::Career::CityPlotType::Bookies:
				Plot.Name = "Bookmaker's";
				break;
			case Game::Career::CityPlotType::LicenseOffice:
				Plot.Name = "Racing License Office";
				break;
			case Game::Career::CityPlotType::BreedingFarm:
				Plot.Name = "Breeding Clinic";
				break;
			case Game::Career::CityPlotType::Stable:
				Plot.Name = "Your Stable";
				break;
			default:
				break;
			}
		}

		for (std::size_t VacantIndex = BuildingTypes.size(); VacantIndex < BuildingTypes.size() + 5; ++VacantIndex)
		{
			Game::Career::CityPlot& Plot = Data.CityPlots[AvailablePlotIndices[VacantIndex]];
			Plot.Type = Game::Career::CityPlotType::VacantLot;
			Plot.Name = "Vacant Lot";
		}
	}

	std::vector<std::size_t> GetTrackPlotIndices(const CareerData& Data)
	{
		std::vector<std::size_t> Indices;
		for (std::size_t Index = 0; Index < Data.CityPlots.size(); ++Index)
			if (Data.CityPlots[Index].Type == Game::Career::CityPlotType::RaceTrack &&
				Data.CityPlots[Index].TrackLength != 0)
				Indices.push_back(Index);
		return Indices;
	}

	void EnsureRaceTrackCapacity(CareerData& Data)
	{
		const std::size_t PopulationTrackCount = Data.TargetAiPopulation / CareerRacersPerRaceTrack +
			(Data.TargetAiPopulation % CareerRacersPerRaceTrack == 0 ? 0 : 1);
		const std::size_t RequiredTrackCount = std::max(CareerMinimumRaceTrackCount, PopulationTrackCount);
		std::uniform_int_distribution<std::size_t> TrackDistribution(0, CareerTrackLengths.size() - 1);
		while (Data.RaceTrackLengths.size() < RequiredTrackCount)
		{
			auto Plot = std::find_if(Data.CityPlots.begin(), Data.CityPlots.end(), [](const auto& Candidate)
				{
					return Candidate.Type == Game::Career::CityPlotType::VacantLot;
				});
			if (Plot == Data.CityPlots.end())
				Plot = std::find_if(Data.CityPlots.begin(), Data.CityPlots.end(), [](const auto& Candidate)
					{
						return Candidate.Type == Game::Career::CityPlotType::Scenery;
					});

			if (Plot == Data.CityPlots.end())
			{
				Game::Career::CityPlot NewPlot;
				NewPlot.GridX = static_cast<unsigned int>(Data.CityPlots.size() % CareerCityGridWidth);
				NewPlot.GridY = static_cast<unsigned int>(Data.CityPlots.size() / CareerCityGridWidth);
				Data.CityPlots.push_back(std::move(NewPlot));
				Plot = Data.CityPlots.end() - 1;
			}

			Plot->Type = Game::Career::CityPlotType::RaceTrack;
			Plot->Name = "City Track " + std::to_string(Data.RaceTrackLengths.size() + 1);
			Plot->TrackLength = CareerTrackLengths[TrackDistribution(Data.WorldRandom)];
			Data.RaceTrackLengths.push_back(Plot->TrackLength);
		}
	}

	std::uint64_t GetWorldTimeAfterRealDelay(const CareerData& Data, std::uint64_t RealDelayMs)
	{
		const std::uint64_t WorldNowMs = Data.WorldClock.NowMs();
		const std::uint64_t TimeScale = Data.WorldClock.GetTimeScale();
		if (RealDelayMs > (std::numeric_limits<std::uint64_t>::max() - WorldNowMs) / TimeScale)
		{
			spdlog::error("Could not schedule career activity retry: world-time deadline overflowed.");
			return std::numeric_limits<std::uint64_t>::max();
		}
		return WorldNowMs + RealDelayMs * TimeScale;
	}

	std::uint64_t GetWorldTimeAtRealDeadline(const CareerData& Data, std::uint64_t RealDeadlineMs)
	{
		if (RealDeadlineMs <= Data.RealTimeMs)
			return Data.WorldClock.NowMs();
		return GetWorldTimeAfterRealDelay(Data, RealDeadlineMs - Data.RealTimeMs);
	}

	std::size_t FindPlotIndex(const CareerData& Data, Game::Career::CityPlotType Type)
	{
		const auto Plot = std::find_if(Data.CityPlots.begin(), Data.CityPlots.end(),
			[Type](const Game::Career::CityPlot& Candidate) { return Candidate.Type == Type; });
		return Plot == Data.CityPlots.end() ? InvalidPlotIndex :
			static_cast<std::size_t>(Plot - Data.CityPlots.begin());
	}

	CareerRacerState* FindRacerState(CareerData& Data, const Racer* Runner)
	{
		if (Runner == nullptr)
			return nullptr;
		const auto State = std::find_if(Data.RacerStates.begin(), Data.RacerStates.end(),
			[Runner](const CareerRacerState& Candidate) { return Candidate.Runner == Runner; });
		return State == Data.RacerStates.end() ? nullptr : &*State;
	}

	const CareerRacerState* FindRacerState(const CareerData& Data, const Racer* Runner)
	{
		if (Runner == nullptr)
			return nullptr;
		const auto State = std::find_if(Data.RacerStates.begin(), Data.RacerStates.end(),
			[Runner](const CareerRacerState& Candidate) { return Candidate.Runner == Runner; });
		return State == Data.RacerStates.end() ? nullptr : &*State;
	}

	CareerLeague* FindLeague(CareerData& Data, std::uint64_t LeagueId)
	{
		const auto League = std::find_if(Data.Leagues.begin(), Data.Leagues.end(),
			[LeagueId](const CareerLeague& Candidate) { return Candidate.Id == LeagueId; });
		return League == Data.Leagues.end() ? nullptr : &*League;
	}

	const CareerLeague* FindLeague(const CareerData& Data, std::uint64_t LeagueId)
	{
		const auto League = std::find_if(Data.Leagues.begin(), Data.Leagues.end(),
			[LeagueId](const CareerLeague& Candidate) { return Candidate.Id == LeagueId; });
		return League == Data.Leagues.end() ? nullptr : &*League;
	}

	CareerLeague* FindLeague(CareerData& Data, CareerLeagueDivision Division)
	{
		const auto League = std::find_if(Data.Leagues.begin(), Data.Leagues.end(),
			[Division](const CareerLeague& Candidate)
			{
				return !Candidate.IsAmateurCup && Candidate.Division == Division;
			});
		return League == Data.Leagues.end() ? nullptr : &*League;
	}

	std::uint64_t GetNextWorldDayBoundary(std::uint64_t WorldTimeMs)
	{
		const std::uint64_t DaysElapsed = WorldTimeMs / CareerAmateurCupIntervalMs;
		if (DaysElapsed >= std::numeric_limits<std::uint64_t>::max() / CareerAmateurCupIntervalMs)
			return std::numeric_limits<std::uint64_t>::max();
		return (DaysElapsed + 1) * CareerAmateurCupIntervalMs;
	}

	std::uint64_t GetRacerId(const Racer* Runner)
	{
		return Runner == nullptr ? 0 : Runner->GUID.GUID;
	}

	Racer* GetRacerById(CareerData& Data, std::uint64_t RacerId)
	{
		if (Data.PlayerLeagueRacer != nullptr && GetRacerId(Data.PlayerLeagueRacer) == RacerId)
			return Data.PlayerLeagueRacer;
		if (RacerId == 0 || RacerId > std::numeric_limits<unsigned int>::max())
			return nullptr;
		return Data.Racers.Get(static_cast<unsigned int>(RacerId));
	}

	CareerLeagueStanding* FindStanding(CareerLeague& League, const Racer* Runner)
	{
		const auto Standing = std::find_if(League.Standings.begin(), League.Standings.end(),
			[Runner](const CareerLeagueStanding& Candidate) { return Candidate.Runner == Runner; });
		return Standing == League.Standings.end() ? nullptr : &*Standing;
	}

	bool IsRacerActive(const CareerData& Data, const Racer* Runner)
	{
		if (Runner != nullptr && Runner == Data.PlayerLeagueRacer)
			return true;
		const CareerRacerState* State = FindRacerState(Data, Runner);
		return State != nullptr && State->Activity != CareerRacerActivity::Retired;
	}

	std::vector<CareerLeagueStanding*> GetSortedActiveStandings(CareerData& Data, CareerLeague& League)
	{
		std::vector<CareerLeagueStanding*> Sorted;
		for (CareerLeagueStanding& Standing : League.Standings)
			if (Standing.Runner != nullptr && IsRacerActive(Data, Standing.Runner))
				Sorted.push_back(&Standing);
		std::stable_sort(Sorted.begin(), Sorted.end(), [](const CareerLeagueStanding* Left,
			const CareerLeagueStanding* Right)
			{
				if (Left->Points != Right->Points)
					return Left->Points > Right->Points;
				if (Left->Wins != Right->Wins)
					return Left->Wins > Right->Wins;
				if (Left->Runner->CareerRecord.PositionPoints != Right->Runner->CareerRecord.PositionPoints)
					return Left->Runner->CareerRecord.PositionPoints > Right->Runner->CareerRecord.PositionPoints;
				return Left->Runner->Name < Right->Runner->Name;
			});
		return Sorted;
	}

	void RecordLeagueFixture(CareerLeague& League, CareerLeagueFixture& Fixture, std::uint64_t WinnerId)
	{
		Racer* First = nullptr;
		Racer* Second = nullptr;
		for (CareerLeagueStanding& Standing : League.Standings)
		{
			if (GetRacerId(Standing.Runner) == Fixture.Fixture.FirstCompetitorId)
				First = Standing.Runner;
			if (GetRacerId(Standing.Runner) == Fixture.Fixture.SecondCompetitorId)
				Second = Standing.Runner;
		}
		CareerLeagueStanding* FirstStanding = FindStanding(League, First);
		CareerLeagueStanding* SecondStanding = FindStanding(League, Second);
		if (FirstStanding == nullptr || SecondStanding == nullptr)
		{
			Fixture.State = CareerFixtureState::Cancelled;
			return;
		}

		Fixture.WinnerId = WinnerId;
		Fixture.State = CareerFixtureState::Complete;
		if (WinnerId == GetRacerId(First))
		{
			++FirstStanding->Wins;
			FirstStanding->Points += 3;
			++SecondStanding->Losses;
		}
		else if (WinnerId == GetRacerId(Second))
		{
			++SecondStanding->Wins;
			SecondStanding->Points += 3;
			++FirstStanding->Losses;
		}
		else
		{
			++FirstStanding->Draws;
			++SecondStanding->Draws;
			++FirstStanding->Points;
			++SecondStanding->Points;
		}
	}

	std::uint64_t GetFixtureWinner(const RaceResult& Result, Racer* First, Racer* Second)
	{
		unsigned int FirstPosition = Game::Race::PulledUpPosition;
		unsigned int SecondPosition = Game::Race::PulledUpPosition;
		for (const Game::Race::RacerRaceResult& RacerResult : Result.RacerResults)
		{
			if (RacerResult.Racer == First)
				FirstPosition = RacerResult.Position;
			else if (RacerResult.Racer == Second)
				SecondPosition = RacerResult.Position;
		}
		if (FirstPosition == Game::Race::PulledUpPosition && SecondPosition == Game::Race::PulledUpPosition)
			return 0;
		if (FirstPosition == Game::Race::PulledUpPosition)
			return GetRacerId(Second);
		if (SecondPosition == Game::Race::PulledUpPosition || FirstPosition < SecondPosition)
			return GetRacerId(First);
		if (SecondPosition < FirstPosition)
			return GetRacerId(Second);
		return 0;
	}

	unsigned int GetPrizeForPosition(const Game::Race::RaceFinancials& Financials, unsigned int Position)
	{
		if (Position == Game::Race::FirstPlaceRank)
			return Financials.FirstPlacePrize;
		if (Position == Game::Race::SecondPlaceRank)
			return Financials.SecondPlacePrize;
		if (Position == Game::Race::ThirdPlaceRank)
			return Financials.ThirdPlacePrize;
		return 0;
	}

	unsigned int GetPrizeForPosition(const Race& CurrentRace, unsigned int Position)
	{
		return GetPrizeForPosition(CurrentRace.Financials, Position);
	}

	unsigned int GetTravelDurationMs(const CareerData& Data, std::size_t FromPlot, std::size_t ToPlot)
	{
		if (FromPlot >= Data.CityPlots.size() || ToPlot >= Data.CityPlots.size())
			return static_cast<unsigned int>(CareerTravelBaseMs);
		const Game::Career::CityPlot& From = Data.CityPlots[FromPlot];
		const Game::Career::CityPlot& To = Data.CityPlots[ToPlot];
		const unsigned int Distance = static_cast<unsigned int>(std::abs(static_cast<int>(From.GridX) -
			static_cast<int>(To.GridX)) + std::abs(static_cast<int>(From.GridY) - static_cast<int>(To.GridY)));
		return static_cast<unsigned int>(CareerTravelBaseMs + Distance * CareerTravelPerGridStepMs);
	}

	void BeginTravel(CareerData& Data, CareerRacerState& State, std::size_t DestinationPlotIndex)
	{
		State.DestinationPlotIndex = DestinationPlotIndex;
		if (State.LocationPlotIndex == DestinationPlotIndex)
		{
			State.Activity = CareerRacerActivity::InBuilding;
			State.ActionEndsAtRealMs = Data.RealTimeMs;
			return;
		}
		State.Activity = CareerRacerActivity::Travelling;
		State.ActionEndsAtRealMs = Data.RealTimeMs +
			GetTravelDurationMs(Data, State.LocationPlotIndex, DestinationPlotIndex);
	}

	void StartLeagueSeason(CareerData& Data, CareerLeague& League)
	{
		if (League.Entrants.size() != CareerLeagueSize)
		{
			League.State = CareerLeagueState::Filling;
			return;
		}

		std::vector<std::uint64_t> CompetitorIds;
		CompetitorIds.reserve(League.Entrants.size());
		for (Racer* Runner : League.Entrants)
			if (Runner != nullptr)
				CompetitorIds.push_back(GetRacerId(Runner));
		if (CompetitorIds.size() != CareerLeagueSize)
		{
			spdlog::error("Could not start league {}: it contains an invalid entrant.", League.Name);
			League.State = CareerLeagueState::Filling;
			return;
		}

		const std::vector<std::size_t> Tracks = GetTrackPlotIndices(Data);
		if (Tracks.empty())
		{
			spdlog::error("Could not start league {}: the world has no usable race tracks.", League.Name);
			League.State = CareerLeagueState::Filling;
			return;
		}

		const std::vector<SportRaceCore::LeagueFixture> Fixtures =
			SportRaceCore::GenerateRoundRobinFixtures(CompetitorIds);
		if (Fixtures.size() != CareerLeagueSize * (CareerLeagueSize - 1) / 2)
		{
			spdlog::error("Could not generate the full fixture list for league {}.", League.Name);
			League.State = CareerLeagueState::Filling;
			return;
		}

		League.Fixtures.clear();
		League.Standings.clear();
		League.Playoffs = SportRaceCore::PlayoffBracket{};
		League.Fixtures.reserve(Fixtures.size());
		League.Standings.reserve(League.Entrants.size());
		for (Racer* Runner : League.Entrants)
			League.Standings.push_back({ Runner, 0, 0, 0, 0 });

		const std::uint64_t SeasonStartMs = Data.WorldClock.NowMs() + CareerLeagueFirstFixtureDelayMs;
		for (std::size_t Index = 0; Index < Fixtures.size(); ++Index)
		{
			const SportRaceCore::LeagueFixture& Fixture = Fixtures[Index];
			CareerLeagueFixture WorldFixture;
			WorldFixture.Fixture = Fixture;
			WorldFixture.ScheduledAtMs = SeasonStartMs +
				static_cast<std::uint64_t>(Fixture.Round) * CareerLeagueRoundIntervalMs;
			WorldFixture.TrackPlotIndex = Tracks[(Index / CareerFixturesPerWorldRace + Fixture.Round + League.Id) %
				Tracks.size()];
			League.Fixtures.push_back(WorldFixture);
		}
		++League.Season;
		League.State = CareerLeagueState::RegularSeason;
}

	void AddLeague(CareerData& Data, const std::string& Name, CareerLeagueDivision Division,
		const std::vector<Racer*>& Entrants)
	{
		CareerLeague League;
		League.Id = Data.NextLeagueId++;
		League.Name = Name;
		League.Division = Division;
		League.Entrants = Entrants;
		for (Racer* Runner : Entrants)
		{
			if (CareerRacerState* State = FindRacerState(Data, Runner))
			{
				State->LeagueId = League.Id;
				State->Activity = CareerRacerActivity::InBuilding;
				if (Division != CareerLeagueDivision::Amateur)
				{
					State->HasRacingLicense = true;
					State->Cash = CareerLicenseSavingsTarget;
				}
			}
		}
		Data.Leagues.push_back(std::move(League));
	}

	std::string MakeNextAmateurLeagueName(CareerData& Data)
	{
		std::vector<std::string> UnusedNames;
		for (const char* Name : AmateurLeagueNames)
			if (std::none_of(Data.Leagues.begin(), Data.Leagues.end(), [Name](const CareerLeague& League)
				{
					return League.Name == Name;
				}))
				UnusedNames.emplace_back(Name);
		if (!UnusedNames.empty())
		{
			std::shuffle(UnusedNames.begin(), UnusedNames.end(), Data.WorldRandom);
			return UnusedNames.front();
		}

		std::size_t AmateurLeagueNumber = 0;
		std::string LeagueName;
		do
		{
			LeagueName = "Amateur League " + std::to_string(++AmateurLeagueNumber);
		} while (std::any_of(Data.Leagues.begin(), Data.Leagues.end(), [&LeagueName](const CareerLeague& League)
			{
				return League.Name == LeagueName;
			}));
		return LeagueName;
	}

	void EnsureOpenAmateurLeague(CareerData& Data)
	{
		const bool HasOpenLeague = std::any_of(Data.Leagues.begin(), Data.Leagues.end(),
			[](const CareerLeague& League)
			{
				return !League.IsAmateurCup && League.Division == CareerLeagueDivision::Amateur &&
					League.State == CareerLeagueState::Filling && League.Entrants.size() < CareerLeagueSize;
			});
		if (!HasOpenLeague)
			AddLeague(Data, MakeNextAmateurLeagueName(Data), CareerLeagueDivision::Amateur, {});
	}

	bool IsRunnerInRace(const CareerData& Data, std::uint64_t LeagueId, std::size_t RoundIndex,
		std::size_t MatchIndex)
	{
		return std::any_of(Data.WorldRaces.begin(), Data.WorldRaces.end(),
			[LeagueId, RoundIndex, MatchIndex](const std::shared_ptr<CareerWorldRace>& WorldRace)
			{
				return WorldRace != nullptr && !WorldRace->IsComplete && WorldRace->IsPlayoff &&
					WorldRace->LeagueId == LeagueId && WorldRace->PlayoffRoundIndex == RoundIndex &&
					WorldRace->PlayoffMatchIndex == MatchIndex;
			});
	}

	std::shared_ptr<CareerWorldRace> StartWorldRace(CareerData& Data, std::size_t TrackPlotIndex,
		const std::vector<Racer*>& Entrants, const std::string& EventName,
		const std::vector<CareerRaceFixtureReference>& LeagueFixtures, std::uint64_t LeagueId = 0,
		bool IsPlayoff = false, std::size_t PlayoffRoundIndex = 0, std::size_t PlayoffMatchIndex = 0)
	{
		if (TrackPlotIndex >= Data.CityPlots.size() || Entrants.size() != CareerWorldRaceEntrantCount)
		{
			spdlog::error("Cannot start world race '{}' with {} entrants on track {} (expected {}).", EventName,
				Entrants.size(), TrackPlotIndex, CareerWorldRaceEntrantCount);
			return nullptr;
		}
		std::unordered_set<Racer*> UniqueEntrants;
		for (Racer* Runner : Entrants)
		{
			if (Runner == nullptr || !UniqueEntrants.insert(Runner).second)
			{
				spdlog::error("Cannot start world race '{}': entrants must be distinct valid racers.", EventName);
				return nullptr;
			}
			const CareerRacerState* State = FindRacerState(Data, Runner);
			if (State == nullptr || State->Activity == CareerRacerActivity::Retired ||
				State->Activity == CareerRacerActivity::InRace ||
				State->Cash < CareerRaceEntryFee)
				return nullptr;
		}

		Race CurrentRace(Data.WorldRandom());
		if (CurrentRace.ThisTrack == nullptr)
		{
			spdlog::error("Could not create world race '{}': the race has no track object.", EventName);
			return nullptr;
		}
		CurrentRace.Racers = Entrants;
		CurrentRace.ThisTrack->Length = static_cast<Game::Race::TrackLength>(
			Data.CityPlots[TrackPlotIndex].TrackLength);
		CurrentRace.Financials = Game::Race::RaceFinancials(CareerRaceFirstPrize, CareerRaceEntryFee);

		auto WorldRace = std::make_shared<CareerWorldRace>();
		WorldRace->Id = Data.NextWorldRaceId++;
		WorldRace->TrackPlotIndex = TrackPlotIndex;
		WorldRace->LeagueId = LeagueId;
		WorldRace->PlayoffRoundIndex = PlayoffRoundIndex;
		WorldRace->PlayoffMatchIndex = PlayoffMatchIndex;
		WorldRace->StartedAtMs = Data.WorldClock.NowMs();
		WorldRace->IsPlayoff = IsPlayoff;
		WorldRace->EventName = EventName;
		WorldRace->Entrants = Entrants;
		WorldRace->LeagueFixtures = LeagueFixtures;
		WorldRace->StateMachine = std::make_shared<Game::Race::RaceStateMachine>(std::move(CurrentRace));

		for (Racer* Runner : Entrants)
		{
			CareerRacerState* State = FindRacerState(Data, Runner);
			State->Cash -= CareerRaceEntryFee;
			State->Activity = CareerRacerActivity::InRace;
			State->ActiveRaceId = WorldRace->Id;
			State->LocationPlotIndex = TrackPlotIndex;
			State->DestinationPlotIndex = TrackPlotIndex;
		}
		for (const CareerRaceFixtureReference& Reference : LeagueFixtures)
			if (CareerLeague* League = FindLeague(Data, Reference.LeagueId);
				League != nullptr && Reference.FixtureIndex < League->Fixtures.size())
			{
				CareerLeagueFixture& Fixture = League->Fixtures[Reference.FixtureIndex];
				Fixture.State = CareerFixtureState::InProgress;
				Fixture.RaceId = WorldRace->Id;
			}

		Data.WorldRaces.push_back(WorldRace);
		return WorldRace;
	}

	void ResolveFixtureByForfeit(CareerData& Data, CareerLeague& League, CareerLeagueFixture& Fixture)
	{
		Racer* First = GetRacerById(Data, Fixture.Fixture.FirstCompetitorId);
		Racer* Second = GetRacerById(Data, Fixture.Fixture.SecondCompetitorId);
		const CareerRacerState* FirstState = FindRacerState(Data, First);
		const CareerRacerState* SecondState = FindRacerState(Data, Second);
		const bool FirstCanRace = First == Data.PlayerLeagueRacer ||
			(FirstState != nullptr && FirstState->Activity != CareerRacerActivity::Retired &&
				FirstState->Cash >= CareerRaceEntryFee);
		const bool SecondCanRace = Second == Data.PlayerLeagueRacer ||
			(SecondState != nullptr && SecondState->Activity != CareerRacerActivity::Retired &&
				SecondState->Cash >= CareerRaceEntryFee);
		if (FirstCanRace == SecondCanRace)
			RecordLeagueFixture(League, Fixture, 0);
		else
			RecordLeagueFixture(League, Fixture, FirstCanRace ? GetRacerId(First) : GetRacerId(Second));
	}

	void CancelRetiredRacerFixtures(CareerData& Data, CareerRacerState& RetiredState)
	{
		if (RetiredState.LeagueId == 0 || RetiredState.Runner == nullptr)
			return;
		CareerLeague* League = FindLeague(Data, RetiredState.LeagueId);
		if (League == nullptr)
			return;
		const std::uint64_t RetiredId = GetRacerId(RetiredState.Runner);
		for (CareerLeagueFixture& Fixture : League->Fixtures)
			if (Fixture.State == CareerFixtureState::Scheduled &&
				(Fixture.Fixture.FirstCompetitorId == RetiredId ||
					Fixture.Fixture.SecondCompetitorId == RetiredId))
				ResolveFixtureByForfeit(Data, *League, Fixture);
	}

	CareerRacerState MakeRacerWorldState(CareerData& Data, Racer* Runner)
	{
		CareerRacerState State;
		State.Runner = Runner;
		State.Cash = CareerStartingCash;
		State.StartingCash = CareerStartingCash;
		State.Activity = CareerRacerActivity::InBuilding;
		const std::vector<std::size_t> AvailablePlots = [&Data]()
		{
			std::vector<std::size_t> Indices;
			for (std::size_t Index = 0; Index < Data.CityPlots.size(); ++Index)
				if (Data.CityPlots[Index].Type != Game::Career::CityPlotType::Scenery &&
					Data.CityPlots[Index].Type != Game::Career::CityPlotType::VacantLot)
					Indices.push_back(Index);
			return Indices;
		}();
		if (!AvailablePlots.empty())
		{
			std::uniform_int_distribution<std::size_t> PlotDistribution(0, AvailablePlots.size() - 1);
			State.LocationPlotIndex = AvailablePlots[PlotDistribution(Data.WorldRandom)];
			State.DestinationPlotIndex = State.LocationPlotIndex;
		}
		return State;
	}

	void RetireAndReplaceRacer(CareerData& Data, Racer* Runner)
	{
		CareerRacerState* RetiredState = FindRacerState(Data, Runner);
		if (RetiredState == nullptr || RetiredState->Activity == CareerRacerActivity::Retired)
			return;
		RetiredState->Activity = CareerRacerActivity::Retired;
		RetiredState->RetiredAtMs = Data.WorldClock.NowMs();
		RetiredState->ActiveRaceId = 0;
		CancelRetiredRacerFixtures(Data, *RetiredState);
		if (CareerLeague* League = FindLeague(Data, RetiredState->LeagueId);
			League != nullptr && League->State == CareerLeagueState::Filling)
		{
			League->Entrants.erase(std::remove(League->Entrants.begin(), League->Entrants.end(), Runner),
				League->Entrants.end());
			RetiredState->LeagueId = 0;
		}

		Racer* Replacement = Data.Racers.Make();
		Data.RacerStates.push_back(MakeRacerWorldState(Data, Replacement));
	}

	void RecordRaceResults(CareerData& Data, CareerWorldRace& WorldRace)
	{
		if (WorldRace.StateMachine == nullptr)
			return;
		Race& CurrentRace = WorldRace.StateMachine->Data.ThisRace;
		const RaceResult& Result = CurrentRace.Result;
		const bool Successful = CurrentRace.IsComplete() && Result.Error == Game::Race::RaceError::None &&
			Result.RacerResults.size() == WorldRace.Entrants.size();
		std::vector<Racer*> BankruptRacers;
		if (Successful)
		{
			for (const Game::Race::RacerRaceResult& RacerResult : Result.RacerResults)
			{
				if (RacerResult.Racer == nullptr)
					continue;
				const bool Finished = RacerResult.Position != Game::Race::PulledUpPosition;
				RacerCareerHistoryEntry HistoryEntry;
				HistoryEntry.WorldTimeMs = Data.WorldClock.NowMs();
				HistoryEntry.EventName = WorldRace.EventName;
				HistoryEntry.TrackLength = Result.TrackLength;
				HistoryEntry.Position = RacerResult.Position;
				HistoryEntry.FinishTimeMs = Finished ? RacerResult.Ms : 0;
				RacerResult.Racer->RecordCareerResult(RacerResult.Position, HistoryEntry.FinishTimeMs, HistoryEntry);
				if (CareerRacerState* State = FindRacerState(Data, RacerResult.Racer))
				{
					State->Cash += static_cast<int>(GetPrizeForPosition(CurrentRace, RacerResult.Position));
					if (State->Cash <= 0)
						BankruptRacers.push_back(RacerResult.Racer);
				}
			}

			for (const CareerRaceFixtureReference& Reference : WorldRace.LeagueFixtures)
			{
				CareerLeague* League = FindLeague(Data, Reference.LeagueId);
				if (League == nullptr || Reference.FixtureIndex >= League->Fixtures.size())
					continue;
				CareerLeagueFixture& Fixture = League->Fixtures[Reference.FixtureIndex];
				Racer* First = GetRacerById(Data, Fixture.Fixture.FirstCompetitorId);
				Racer* Second = GetRacerById(Data, Fixture.Fixture.SecondCompetitorId);
				RecordLeagueFixture(*League, Fixture, GetFixtureWinner(Result, First, Second));
			}

			if (WorldRace.IsPlayoff)
			{
				CareerLeague* League = FindLeague(Data, WorldRace.LeagueId);
				if (League != nullptr)
				{
					if (WorldRace.PlayoffRoundIndex >= League->Playoffs.Rounds.size() ||
						WorldRace.PlayoffMatchIndex >=
						League->Playoffs.Rounds[WorldRace.PlayoffRoundIndex].Matches.size())
					{
						spdlog::error("Could not resolve playoff race {} in league {}: match no longer exists.",
							WorldRace.Id, League->Name);
					}
					else
					{
						const SportRaceCore::PlayoffMatch& Match =
							League->Playoffs.Rounds[WorldRace.PlayoffRoundIndex].Matches[WorldRace.PlayoffMatchIndex];
						const std::uint64_t CompetitorAId = Match.CompetitorAId.value_or(0);
						const std::uint64_t CompetitorBId = Match.CompetitorBId.value_or(0);
						std::uint64_t WinnerId = 0;
						unsigned int WinningPosition = Game::Race::PulledUpPosition;
						for (const Game::Race::RacerRaceResult& RacerResult : Result.RacerResults)
						{
							if (RacerResult.Racer == nullptr)
								continue;
							const std::uint64_t RacerId = GetRacerId(RacerResult.Racer);
							if ((RacerId == CompetitorAId || RacerId == CompetitorBId) &&
								RacerResult.Position < WinningPosition)
							{
								WinnerId = RacerId;
								WinningPosition = RacerResult.Position;
							}
						}
						if (WinnerId == 0)
							WinnerId = CompetitorAId != 0 ? CompetitorAId : CompetitorBId;
						if (!SportRaceCore::SetPlayoffMatchWinner(League->Playoffs, WorldRace.PlayoffRoundIndex,
							WorldRace.PlayoffMatchIndex, WinnerId))
							spdlog::error("Could not advance the playoff winner in league {}.", League->Name);
					}
				}
			}
		}
		else
		{
			spdlog::error("World race {} failed; refunding entry fees.", WorldRace.Id);
			for (Racer* Runner : WorldRace.Entrants)
				if (CareerRacerState* State = FindRacerState(Data, Runner))
				{
					State->Cash += CareerRaceEntryFee;
					if (State->Cash <= 0)
						BankruptRacers.push_back(Runner);
				}
			for (const CareerRaceFixtureReference& Reference : WorldRace.LeagueFixtures)
				if (CareerLeague* League = FindLeague(Data, Reference.LeagueId);
					League != nullptr && Reference.FixtureIndex < League->Fixtures.size())
					League->Fixtures[Reference.FixtureIndex].State = CareerFixtureState::Cancelled;
		}

		for (Racer* Runner : WorldRace.Entrants)
			if (CareerRacerState* State = FindRacerState(Data, Runner);
				State != nullptr && State->Activity != CareerRacerActivity::Retired)
			{
				State->Activity = CareerRacerActivity::Resting;
				State->ActiveRaceId = 0;
				State->ActionEndsAtRealMs = Data.RealTimeMs + CareerPostRaceRestMs;
				State->LocationPlotIndex = WorldRace.TrackPlotIndex;
				State->DestinationPlotIndex = WorldRace.TrackPlotIndex;
			}

		WorldRace.IsSettled = true;
		WorldRace.IsComplete = true;
		WorldRace.FinishedAtMs = Data.WorldClock.NowMs();
		for (Racer* Runner : BankruptRacers)
			RetireAndReplaceRacer(Data, Runner);
}

	void TickWorldRaces(CareerData& Data, std::uint64_t ElapsedRealMs)
	{
		std::vector<std::shared_ptr<CareerWorldRace>> ActiveRaces;
		for (const std::shared_ptr<CareerWorldRace>& WorldRace : Data.WorldRaces)
		{
			if (WorldRace == nullptr || WorldRace->IsComplete || WorldRace->StateMachine == nullptr)
				continue;
			ActiveRaces.push_back(WorldRace);
		}

		std::vector<std::shared_ptr<CareerWorldRace>> CompletedRaces(ActiveRaces.size());
		std::transform(std::execution::par, ActiveRaces.begin(), ActiveRaces.end(), CompletedRaces.begin(),
			[ElapsedRealMs](const std::shared_ptr<CareerWorldRace>& WorldRace)
			{
				WorldRace->TickAccumulatorMs += ElapsedRealMs;
				while (WorldRace->TickAccumulatorMs >= Game::Race::RaceTickIntervalMs && !WorldRace->IsComplete)
				{
					WorldRace->TickAccumulatorMs -= Game::Race::RaceTickIntervalMs;
					const Game::Race::RaceStateType State =
						WorldRace->StateMachine->Tick(Game::Race::RaceTickIntervalMs);
					if (State == Game::Race::RaceStateType::Finished || State == Game::Race::RaceStateType::Failed)
						return WorldRace;
				}
				return std::shared_ptr<CareerWorldRace>{};
			});

		for (const std::shared_ptr<CareerWorldRace>& WorldRace : CompletedRaces)
			if (WorldRace != nullptr)
				RecordRaceResults(Data, *WorldRace);
	}

	void FinishTravel(CareerData& Data, CareerRacerState& State)
	{
		State.LocationPlotIndex = State.DestinationPlotIndex;
		if (State.LicenseApplicationInProgress)
		{
			if (State.Cash >= CareerRacingLicensePrice)
			{
				State.Cash -= CareerRacingLicensePrice;
				State.HasRacingLicense = true;
				State.LicenseApplicationInProgress = false;
				State.Activity = State.LeagueId == 0 ? CareerRacerActivity::WaitingForLeague :
					CareerRacerActivity::InBuilding;
				State.ActionEndsAtRealMs = Data.RealTimeMs;
			}
			else
			{
				State.LicenseApplicationInProgress = false;
				State.Activity = CareerRacerActivity::Resting;
				State.ActionEndsAtRealMs = Data.RealTimeMs + CareerRecoveryIntervalMs;
			}
			return;
		}

		if (State.LocationPlotIndex == FindPlotIndex(Data, Game::Career::CityPlotType::Gym))
		{
			State.Activity = CareerRacerActivity::Training;
			State.ActionEndsAtRealMs = Data.RealTimeMs + CareerTrainingDurationMs;
		}
		else
		{
			State.Activity = State.LeagueId == 0 ? CareerRacerActivity::WaitingForLeague :
				CareerRacerActivity::InBuilding;
			State.ActionEndsAtRealMs = Data.RealTimeMs;
		}
	}

	void CompleteTraining(CareerData& Data, CareerRacerState& State)
	{
		if (State.Runner == nullptr)
			return;
		State.Cash -= CareerTrainingFee;
		State.Runner->Conditions.Fatigue = std::max(0.0,
			State.Runner->Conditions.Fatigue - CareerTrainingFatigueRecovery);
		State.Runner->Conditions.Injury = std::max(0.0,
			State.Runner->Conditions.Injury - CareerTrainingInjuryRecovery);
		double* WeakestSkill = &State.Runner->Skills.Speed;
		for (double* Skill : { &State.Runner->Skills.Endurance, &State.Runner->Skills.Acceleration,
			&State.Runner->Skills.Reflex })
			if (*Skill < *WeakestSkill)
				WeakestSkill = Skill;
		*WeakestSkill = std::min(100.0, *WeakestSkill +
			std::clamp(State.Runner->Skills.Trainability * 0.02, 0.2, 1.5));
		State.Activity = CareerRacerActivity::Resting;
		State.ActionEndsAtRealMs = Data.RealTimeMs + CareerRecoveryIntervalMs;
	}

	bool NeedsTraining(const Racer& Runner)
	{
		return Runner.Conditions.Fatigue >= CareerTrainingFatigueThreshold ||
			Runner.Conditions.Injury >= CareerTrainingInjuryThreshold;
	}

	bool AllActiveRacersHaveHistory(const CareerData& Data)
	{
		return std::all_of(Data.RacerStates.begin(), Data.RacerStates.end(), [](const CareerRacerState& State)
			{
				return State.Activity == CareerRacerActivity::Retired ||
					(State.Runner != nullptr && !State.Runner->CareerRecord.History.empty());
			});
	}

	void UpdateAgentActivities(CareerData& Data)
	{
		std::vector<Racer*> BankruptRacers(Data.RacerStates.size(), nullptr);
		const std::uint64_t Now = Data.RealTimeMs;
		const std::size_t GymIndex = FindPlotIndex(Data, Game::Career::CityPlotType::Gym);
		const std::size_t LicenseOfficeIndex = FindPlotIndex(Data, Game::Career::CityPlotType::LicenseOffice);
		std::transform(std::execution::par, Data.RacerStates.begin(), Data.RacerStates.end(),
			BankruptRacers.begin(), [&Data, Now, GymIndex, LicenseOfficeIndex](CareerRacerState& State) -> Racer*
		{
			if (State.Runner == nullptr || State.Activity == CareerRacerActivity::Retired)
				return nullptr;
			if (State.Cash <= 0 && State.Activity != CareerRacerActivity::InRace)
				return State.Runner;

			if (State.Activity == CareerRacerActivity::Travelling && State.ActionEndsAtRealMs <= Now)
				FinishTravel(Data, State);
			if (State.Activity == CareerRacerActivity::Training && State.ActionEndsAtRealMs <= Now)
				CompleteTraining(Data, State);

			if (State.Activity == CareerRacerActivity::InRace || State.Activity == CareerRacerActivity::Training ||
				State.Activity == CareerRacerActivity::Travelling)
				return nullptr;
			if (State.ActionEndsAtRealMs > Now)
				return nullptr;

			if (!State.HasRacingLicense && !State.LicenseApplicationInProgress &&
				State.Cash >= CareerLicenseSavingsTarget && LicenseOfficeIndex != InvalidPlotIndex)
			{
				State.LicenseApplicationInProgress = true;
				BeginTravel(Data, State, LicenseOfficeIndex);
				if (State.Activity != CareerRacerActivity::Travelling)
					FinishTravel(Data, State);
				return nullptr;
			}

			if (NeedsTraining(*State.Runner) && GymIndex != InvalidPlotIndex)
			{
				if (State.Cash > CareerTrainingFee)
				{
					BeginTravel(Data, State, GymIndex);
					if (State.Activity != CareerRacerActivity::Travelling)
					{
						State.Activity = CareerRacerActivity::Training;
						State.ActionEndsAtRealMs = Now + CareerTrainingDurationMs;
					}
				}
				else
				{
					State.Runner->Conditions.Fatigue = std::max(0.0,
						State.Runner->Conditions.Fatigue - CareerPassiveFatigueRecovery);
					State.Runner->Conditions.Injury = std::max(0.0,
						State.Runner->Conditions.Injury - CareerPassiveInjuryRecovery);
					State.Activity = CareerRacerActivity::Resting;
					State.ActionEndsAtRealMs = Now + CareerRecoveryIntervalMs;
				}
				return nullptr;
			}

			State.Runner->Conditions.Fatigue = std::max(0.0,
				State.Runner->Conditions.Fatigue - CareerPassiveFatigueRecovery);
			State.Runner->Conditions.Injury = std::max(0.0,
				State.Runner->Conditions.Injury - CareerPassiveInjuryRecovery);
			State.Activity = State.LeagueId == 0 ? CareerRacerActivity::WaitingForLeague :
				CareerRacerActivity::Resting;
			State.ActionEndsAtRealMs = Now + CareerRecoveryIntervalMs;
			return nullptr;
		});

		for (Racer* Runner : BankruptRacers)
			if (Runner != nullptr)
				RetireAndReplaceRacer(Data, Runner);
	}

	void AssignRacerToLeague(CareerData& Data, CareerRacerState& State, CareerLeague& League)
	{
		if (State.Runner == nullptr || State.Activity == CareerRacerActivity::Retired || State.LeagueId != 0 ||
			League.Entrants.size() >= CareerLeagueSize)
			return;
		State.LeagueId = League.Id;
		State.Activity = CareerRacerActivity::InBuilding;
		League.Entrants.push_back(State.Runner);
	}

	void FillLeagueVacancies(CareerData& Data)
	{
		for (CareerLeague& League : Data.Leagues)
		{
			if (League.IsAmateurCup || League.State != CareerLeagueState::Filling)
				continue;
			while (League.Entrants.size() < CareerLeagueSize)
			{
				CareerRacerState* Candidate = nullptr;
				for (CareerRacerState& State : Data.RacerStates)
				{
					const bool IsAvailable = State.Activity == CareerRacerActivity::InBuilding ||
						State.Activity == CareerRacerActivity::WaitingForLeague;
					if (State.Runner == nullptr || State.LeagueId != 0 || !IsAvailable ||
						(League.Division != CareerLeagueDivision::Amateur && !State.HasRacingLicense))
						continue;
					if (Candidate == nullptr ||
						Game::Career::EstimateRacerStrength(*State.Runner, 500) >
							Game::Career::EstimateRacerStrength(*Candidate->Runner, 500))
						Candidate = &State;
				}
				if (Candidate == nullptr)
					break;
				AssignRacerToLeague(Data, *Candidate, League);
			}
			if (League.Entrants.size() == CareerLeagueSize)
				StartLeagueSeason(Data, League);
		}
		EnsureOpenAmateurLeague(Data);
	}

	void UpdateLicenseStatus(CareerData& Data)
	{
		for (CareerRacerState& State : Data.RacerStates)
			if (State.Runner != nullptr && State.Activity != CareerRacerActivity::Retired &&
				State.HasRacingLicense && State.LeagueId == 0)
				State.Activity = CareerRacerActivity::WaitingForLeague;
	}

	bool IsLeagueSeasonResolved(const CareerLeague& League)
	{
		return std::all_of(League.Fixtures.begin(), League.Fixtures.end(),
			[](const CareerLeagueFixture& Fixture)
			{
				return Fixture.State == CareerFixtureState::Complete ||
					Fixture.State == CareerFixtureState::Cancelled;
			});
	}

	void UpdateLeagueStates(CareerData& Data)
	{
		for (CareerLeague& League : Data.Leagues)
		{
			if (League.State == CareerLeagueState::RegularSeason && IsLeagueSeasonResolved(League))
			{
				const std::vector<CareerLeagueStanding*> Sorted = GetSortedActiveStandings(Data, League);
				std::vector<std::uint64_t> PlayoffEntrants;
				const std::size_t EntrantCount = std::min<std::size_t>(4, Sorted.size());
				for (std::size_t Index = 0; Index < EntrantCount; ++Index)
					PlayoffEntrants.push_back(GetRacerId(Sorted[Index]->Runner));
				League.Playoffs = SportRaceCore::GeneratePlayoffBracket(PlayoffEntrants);
				League.State = League.Playoffs.IsComplete ? CareerLeagueState::Complete : CareerLeagueState::Playoffs;
			}
			else if (League.State == CareerLeagueState::Playoffs && League.Playoffs.IsComplete)
				League.State = CareerLeagueState::Complete;
		}
	}

	void RecordRaceStartTravel(CareerData& Data, const std::vector<Racer*>& Entrants,
		std::size_t TrackPlotIndex, std::uint64_t& RetryAtWorldMs, bool& IsReady)
	{
		IsReady = true;
		RetryAtWorldMs = Data.WorldClock.NowMs();
		for (Racer* Runner : Entrants)
		{
			CareerRacerState* State = FindRacerState(Data, Runner);
			if (State == nullptr || State->Activity == CareerRacerActivity::Retired)
			{
				IsReady = false;
				RetryAtWorldMs = GetWorldTimeAfterRealDelay(Data, CareerRaceRetryDelayMs);
				return;
			}
			if (State->Activity == CareerRacerActivity::InRace || State->Activity == CareerRacerActivity::Training)
			{
				IsReady = false;
				RetryAtWorldMs = std::max(RetryAtWorldMs,
					GetWorldTimeAfterRealDelay(Data, CareerRaceRetryDelayMs));
				continue;
			}
			if (State->Activity == CareerRacerActivity::Travelling)
			{
				IsReady = false;
				RetryAtWorldMs = std::max(RetryAtWorldMs, State->DestinationPlotIndex == TrackPlotIndex
					? GetWorldTimeAtRealDeadline(Data, State->ActionEndsAtRealMs)
					: GetWorldTimeAfterRealDelay(Data, CareerRaceRetryDelayMs));
				continue;
			}
			if (State->LocationPlotIndex != TrackPlotIndex)
			{
				BeginTravel(Data, *State, TrackPlotIndex);
				IsReady = false;
				RetryAtWorldMs = std::max(RetryAtWorldMs,
					GetWorldTimeAtRealDeadline(Data, State->ActionEndsAtRealMs));
			}
		}
	}

	bool IsOwnedRacer(const CareerData& Data, const Racer* Runner)
	{
		return std::find(Data.OwnedRacers.begin(), Data.OwnedRacers.end(), Runner) != Data.OwnedRacers.end();
	}

	std::unordered_set<std::uint64_t> GetNearLeagueFixtureRacerIds(const CareerData& Data)
	{
		std::unordered_set<std::uint64_t> RacerIds;
		const CareerPendingLeagueRace& PendingRace = Data.PendingPlayerLeagueRace;
		if (PendingRace.Preparing || PendingRace.Active)
			for (const Racer* Runner : PendingRace.Entrants)
				if (Runner != nullptr)
					RacerIds.insert(GetRacerId(Runner));

		const std::uint64_t WindowEnd = Data.WorldClock.NowMs() + CareerLeagueRoundIntervalMs;
		for (const CareerLeague& League : Data.Leagues)
			if (League.State == CareerLeagueState::RegularSeason)
				for (const CareerLeagueFixture& Fixture : League.Fixtures)
					if (Fixture.State == CareerFixtureState::Scheduled && Fixture.ScheduledAtMs <= WindowEnd)
					{
						RacerIds.insert(Fixture.Fixture.FirstCompetitorId);
						RacerIds.insert(Fixture.Fixture.SecondCompetitorId);
					}
		return RacerIds;
	}

	bool HasPendingPlayoffMatch(const CareerData& Data, const Racer* Runner)
	{
		const std::uint64_t RunnerId = GetRacerId(Runner);
		for (const CareerLeague& League : Data.Leagues)
			if (League.State == CareerLeagueState::Playoffs)
				for (const SportRaceCore::PlayoffRound& Round : League.Playoffs.Rounds)
					for (const SportRaceCore::PlayoffMatch& Match : Round.Matches)
						if (!Match.IsComplete &&
							((Match.CompetitorAId.has_value() && *Match.CompetitorAId == RunnerId) ||
								(Match.CompetitorBId.has_value() && *Match.CompetitorBId == RunnerId)))
							return true;
		return false;
	}

	bool FillWorldRaceToSix(CareerData& Data, std::vector<Racer*>& Entrants)
	{
		if (Entrants.size() >= CareerWorldRaceEntrantCount)
			return Entrants.size() == CareerWorldRaceEntrantCount;

		const std::unordered_set<Racer*> SelectedEntrants(Entrants.begin(), Entrants.end());
		const std::unordered_set<std::uint64_t> NearLeagueFixtureRacerIds =
			GetNearLeagueFixtureRacerIds(Data);
		std::vector<Racer*> GuestEntrants;
		std::vector<Racer*> NearFixtureGuestEntrants;
		for (const CareerRacerState& State : Data.RacerStates)
		{
			const bool IsAvailable = State.Activity == CareerRacerActivity::InBuilding ||
				State.Activity == CareerRacerActivity::Resting ||
				State.Activity == CareerRacerActivity::WaitingForLeague;
			if (State.Runner == nullptr || SelectedEntrants.contains(State.Runner) ||
				IsOwnedRacer(Data, State.Runner) || State.Cash < CareerRaceEntryFee || !IsAvailable ||
				HasPendingPlayoffMatch(Data, State.Runner))
				continue;
			if (NearLeagueFixtureRacerIds.contains(GetRacerId(State.Runner)))
				NearFixtureGuestEntrants.push_back(State.Runner);
			else
				GuestEntrants.push_back(State.Runner);
		}

		const std::size_t Needed = CareerWorldRaceEntrantCount - Entrants.size();
		if (GuestEntrants.size() < Needed)
		{
			const std::size_t AdditionalNeeded = Needed - GuestEntrants.size();
			if (NearFixtureGuestEntrants.size() < AdditionalNeeded)
				return false;
			std::shuffle(NearFixtureGuestEntrants.begin(), NearFixtureGuestEntrants.end(), Data.WorldRandom);
			GuestEntrants.insert(GuestEntrants.end(), NearFixtureGuestEntrants.begin(),
				NearFixtureGuestEntrants.begin() + AdditionalNeeded);
		}
		std::shuffle(GuestEntrants.begin(), GuestEntrants.end(), Data.WorldRandom);
		Entrants.insert(Entrants.end(), GuestEntrants.begin(), GuestEntrants.begin() + Needed);
		return true;
	}

	void StartDailyAmateurCup(CareerData& Data)
	{
		const std::uint64_t Now = Data.WorldClock.NowMs();
		if (Data.InitialWorldMonthSimulationInProgress || Now < Data.NextAmateurCupAtMs)
			return;

		Data.NextAmateurCupAtMs = GetNextWorldDayBoundary(Now);
		CareerLeague* Cup = nullptr;
		for (CareerLeague& League : Data.Leagues)
			if (League.IsAmateurCup)
			{
				Cup = &League;
				break;
			}
		if (Cup != nullptr && Cup->State != CareerLeagueState::Filling &&
			Cup->State != CareerLeagueState::Complete)
			return;

		std::vector<Racer*> Entrants;
		std::unordered_set<std::uint64_t> EntrantIds;
		for (const CareerRacerState& State : Data.RacerStates)
		{
			if (State.Runner == nullptr || State.Activity == CareerRacerActivity::Retired)
				continue;
			const CareerLeague* League = State.LeagueId == 0 ? nullptr : FindLeague(Data, State.LeagueId);
			const bool IsAmateur = League != nullptr && !League->IsAmateurCup &&
				League->Division == CareerLeagueDivision::Amateur;
			const bool IsUnlicensedUnassigned = State.LeagueId == 0 && !State.HasRacingLicense;
			const std::uint64_t RacerId = GetRacerId(State.Runner);
			if ((IsAmateur || IsUnlicensedUnassigned) && RacerId != 0 && EntrantIds.insert(RacerId).second)
				Entrants.push_back(State.Runner);
		}
		if (Entrants.size() < 2)
		{
			spdlog::warn("Skipping today's Amateur Cup: fewer than two active amateur racers are available.");
			return;
		}

		std::shuffle(Entrants.begin(), Entrants.end(), Data.WorldRandom);
		std::vector<std::uint64_t> CompetitorIds;
		CompetitorIds.reserve(Entrants.size());
		for (const Racer* Runner : Entrants)
			CompetitorIds.push_back(GetRacerId(Runner));

		if (Cup == nullptr)
		{
			CareerLeague NewCup;
			NewCup.Id = CareerAmateurCupId;
			NewCup.Name = "Amateur Cup";
			NewCup.Division = CareerLeagueDivision::Amateur;
			NewCup.IsAmateurCup = true;
			Data.Leagues.push_back(std::move(NewCup));
			Cup = &Data.Leagues.back();
		}
		Cup->Season++;
		Cup->Entrants = std::move(Entrants);
		Cup->Fixtures.clear();
		Cup->Standings.clear();
		Cup->Playoffs = SportRaceCore::GeneratePlayoffBracket(CompetitorIds);
		Cup->State = CareerLeagueState::Playoffs;
		spdlog::info("Amateur Cup edition {} started with {} entrants on world day {}.", Cup->Season,
			Cup->Entrants.size(), Now / CareerAmateurCupIntervalMs + 1);
	}

	void TryStartLeagueRaces(CareerData& Data)
	{
		for (CareerLeague& League : Data.Leagues)
		{
			if (League.State != CareerLeagueState::RegularSeason)
				continue;
		std::uint32_t EarliestUnresolvedRound = std::numeric_limits<std::uint32_t>::max();
		for (const CareerLeagueFixture& Fixture : League.Fixtures)
			if (Fixture.State == CareerFixtureState::Scheduled || Fixture.State == CareerFixtureState::InProgress)
				EarliestUnresolvedRound = std::min(EarliestUnresolvedRound, Fixture.Fixture.Round);
		if (EarliestUnresolvedRound == std::numeric_limits<std::uint32_t>::max())
			continue;

			for (std::size_t Index = 0; Index < League.Fixtures.size(); ++Index)
			{
				CareerLeagueFixture& FirstFixture = League.Fixtures[Index];
				if (FirstFixture.State != CareerFixtureState::Scheduled ||
					FirstFixture.Fixture.Round != EarliestUnresolvedRound ||
					FirstFixture.ScheduledAtMs > Data.WorldClock.NowMs())
					continue;

				std::vector<CareerRaceFixtureReference> References;
				std::vector<Racer*> Entrants;
				std::unordered_set<std::uint64_t> EntrantIds;
				for (std::size_t CandidateIndex = Index; CandidateIndex < League.Fixtures.size() &&
					References.size() < CareerFixturesPerWorldRace; ++CandidateIndex)
				{
					CareerLeagueFixture& Fixture = League.Fixtures[CandidateIndex];
					if (Fixture.State != CareerFixtureState::Scheduled || Fixture.ScheduledAtMs > Data.WorldClock.NowMs() ||
						Fixture.TrackPlotIndex != FirstFixture.TrackPlotIndex ||
						Fixture.Fixture.Round != FirstFixture.Fixture.Round)
						continue;

					Racer* First = GetRacerById(Data, Fixture.Fixture.FirstCompetitorId);
					Racer* Second = GetRacerById(Data, Fixture.Fixture.SecondCompetitorId);
					CareerRacerState* FirstState = FindRacerState(Data, First);
					CareerRacerState* SecondState = FindRacerState(Data, Second);
					const bool FirstCanRace = First == Data.PlayerLeagueRacer ||
						(FirstState != nullptr && FirstState->Activity != CareerRacerActivity::Retired &&
							FirstState->Cash >= CareerRaceEntryFee);
					const bool SecondCanRace = Second == Data.PlayerLeagueRacer ||
						(SecondState != nullptr && SecondState->Activity != CareerRacerActivity::Retired &&
							SecondState->Cash >= CareerRaceEntryFee);
					if (!FirstCanRace || !SecondCanRace)
					{
						ResolveFixtureByForfeit(Data, League, Fixture);
						continue;
					}
					if (First == Data.PlayerLeagueRacer || Second == Data.PlayerLeagueRacer)
					{
						Racer* Opponent = First == Data.PlayerLeagueRacer ? Second : First;
						CareerRacerState* OpponentState = FindRacerState(Data, Opponent);
						if (OpponentState != nullptr &&
							OpponentState->Activity != CareerRacerActivity::InRace &&
							OpponentState->Activity != CareerRacerActivity::Training &&
							OpponentState->Activity != CareerRacerActivity::Travelling &&
							OpponentState->LocationPlotIndex != FirstFixture.TrackPlotIndex)
							BeginTravel(Data, *OpponentState, FirstFixture.TrackPlotIndex);
						continue;
					}
					if (EntrantIds.contains(GetRacerId(First)) || EntrantIds.contains(GetRacerId(Second)) ||
						Entrants.size() + 2 > CareerWorldRaceEntrantCount)
						continue;
					EntrantIds.insert(GetRacerId(First));
					EntrantIds.insert(GetRacerId(Second));
					Entrants.push_back(First);
					Entrants.push_back(Second);
					References.push_back({ League.Id, CandidateIndex });
				}

				if (Entrants.empty())
					continue;
				if (!FillWorldRaceToSix(Data, Entrants))
				{
					for (const CareerRaceFixtureReference& Reference : References)
						League.Fixtures[Reference.FixtureIndex].ScheduledAtMs =
							GetWorldTimeAfterRealDelay(Data, CareerRaceRetryDelayMs);
					continue;
				}
				bool Ready = false;
				std::uint64_t RetryAtWorldMs = 0;
				RecordRaceStartTravel(Data, Entrants, FirstFixture.TrackPlotIndex, RetryAtWorldMs, Ready);
				if (!Ready)
				{
					for (const CareerRaceFixtureReference& Reference : References)
						League.Fixtures[Reference.FixtureIndex].ScheduledAtMs =
							std::max(RetryAtWorldMs, Data.WorldClock.NowMs() + 1);
					continue;
				}

				std::string EventName = League.Name + " - round " +
					std::to_string(FirstFixture.Fixture.Round + 1);
				if (StartWorldRace(Data, FirstFixture.TrackPlotIndex, Entrants, EventName, References, League.Id) == nullptr)
					for (const CareerRaceFixtureReference& Reference : References)
						League.Fixtures[Reference.FixtureIndex].ScheduledAtMs =
							GetWorldTimeAfterRealDelay(Data, CareerRaceRetryDelayMs);
			}
		}
	}

	void TryStartPlayoffRaces(CareerData& Data)
	{
		const std::vector<std::size_t> Tracks = GetTrackPlotIndices(Data);
		if (Tracks.empty())
			return;
		for (CareerLeague& League : Data.Leagues)
		{
			if (League.State != CareerLeagueState::Playoffs || League.Playoffs.IsComplete)
				continue;
			for (std::size_t RoundIndex = 0; RoundIndex < League.Playoffs.Rounds.size(); ++RoundIndex)
				for (std::size_t MatchIndex = 0; MatchIndex < League.Playoffs.Rounds[RoundIndex].Matches.size(); ++MatchIndex)
				{
					const SportRaceCore::PlayoffMatch& Match = League.Playoffs.Rounds[RoundIndex].Matches[MatchIndex];
					if (Match.IsComplete || !Match.CompetitorAId.has_value() || !Match.CompetitorBId.has_value() ||
						*Match.CompetitorAId == 0 || *Match.CompetitorBId == 0 ||
						IsRunnerInRace(Data, League.Id, RoundIndex, MatchIndex))
						continue;
					if (Data.PendingPlayerLeagueRace.Active && Data.PendingPlayerLeagueRace.IsPlayoff &&
						Data.PendingPlayerLeagueRace.LeagueId == League.Id &&
						Data.PendingPlayerLeagueRace.PlayoffRoundIndex == RoundIndex &&
						Data.PendingPlayerLeagueRace.PlayoffMatchIndex == MatchIndex)
						continue;

					Racer* First = GetRacerById(Data, *Match.CompetitorAId);
					Racer* Second = GetRacerById(Data, *Match.CompetitorBId);
					CareerRacerState* FirstState = FindRacerState(Data, First);
					CareerRacerState* SecondState = FindRacerState(Data, Second);
					const bool FirstCanRace = First == Data.PlayerLeagueRacer ||
						(FirstState != nullptr && FirstState->Activity != CareerRacerActivity::Retired &&
							FirstState->Cash >= CareerRaceEntryFee);
					const bool SecondCanRace = Second == Data.PlayerLeagueRacer ||
						(SecondState != nullptr && SecondState->Activity != CareerRacerActivity::Retired &&
							SecondState->Cash >= CareerRaceEntryFee);
					if (FirstCanRace != SecondCanRace)
					{
						const std::uint64_t WinnerId = FirstCanRace ? GetRacerId(First) : GetRacerId(Second);
						if (!SportRaceCore::SetPlayoffMatchWinner(League.Playoffs, RoundIndex, MatchIndex, WinnerId))
							spdlog::error("Could not award the playoff match in league {}.", League.Name);
						continue;
					}
					if (!FirstCanRace)
					{
						const std::uint64_t SeededWinnerId = Match.CompetitorAId.value();
						spdlog::warn("Both competitors are unavailable for playoff match {} in {}; advancing the higher seed by walkover.",
							MatchIndex + 1, League.Name);
						if (!SportRaceCore::SetPlayoffMatchWinner(League.Playoffs, RoundIndex, MatchIndex,
							SeededWinnerId))
							spdlog::error("Could not resolve unavailable playoff match {} in {}.", MatchIndex + 1,
								League.Name);
						continue;
					}

					const std::size_t TrackIndex = Tracks[(RoundIndex + MatchIndex + League.Id) % Tracks.size()];
					if (First == Data.PlayerLeagueRacer || Second == Data.PlayerLeagueRacer)
					{
						Racer* Opponent = First == Data.PlayerLeagueRacer ? Second : First;
						CareerRacerState* OpponentState = FindRacerState(Data, Opponent);
						if (OpponentState != nullptr &&
							OpponentState->Activity != CareerRacerActivity::InRace &&
							OpponentState->Activity != CareerRacerActivity::Training &&
							OpponentState->Activity != CareerRacerActivity::Travelling &&
							OpponentState->LocationPlotIndex != TrackIndex)
							BeginTravel(Data, *OpponentState, TrackIndex);
						continue;
					}
					std::vector<Racer*> Entrants{ First, Second };
					if (!FillWorldRaceToSix(Data, Entrants))
						continue;
					bool Ready = false;
					std::uint64_t RetryAtWorldMs = 0;
					RecordRaceStartTravel(Data, Entrants, TrackIndex, RetryAtWorldMs, Ready);
					if (!Ready)
						continue;

					const std::string EventName = League.Name + " playoff round " +
						std::to_string(RoundIndex + 1);
					StartWorldRace(Data, TrackIndex, Entrants, EventName, {}, League.Id, true,
						RoundIndex, MatchIndex);
			}
		}
	}

	void TryStartOpenRaces(CareerData& Data)
	{
		if (Data.WorldClock.NowMs() < Data.NextOpenRaceAtMs)
			return;
		Data.NextOpenRaceAtMs = Data.WorldClock.NowMs() + CareerOpenRaceIntervalMs;
		const std::unordered_set<std::uint64_t> NearLeagueFixtureRacerIds =
			GetNearLeagueFixtureRacerIds(Data);
		for (const std::size_t TrackIndex : GetTrackPlotIndices(Data))
		{
			std::vector<Racer*> Candidates;
			for (const CareerRacerState& State : Data.RacerStates)
			{
				const CareerLeague* League = FindLeague(Data, State.LeagueId);
				const bool WaitingForAmateurLeague = League != nullptr &&
					League->Division == CareerLeagueDivision::Amateur &&
					League->State == CareerLeagueState::Filling;
				if (State.Runner != nullptr && (State.LeagueId == 0 || WaitingForAmateurLeague) &&
					!IsOwnedRacer(Data, State.Runner) &&
					State.Cash >= CareerRaceEntryFee &&
					(State.Activity == CareerRacerActivity::InBuilding ||
						State.Activity == CareerRacerActivity::Resting ||
						State.Activity == CareerRacerActivity::WaitingForLeague) &&
					!NearLeagueFixtureRacerIds.contains(GetRacerId(State.Runner)) &&
					!HasPendingPlayoffMatch(Data, State.Runner))
					Candidates.push_back(State.Runner);
			}
			std::shuffle(Candidates.begin(), Candidates.end(), Data.WorldRandom);
			if (Candidates.size() < CareerWorldRaceEntrantCount)
				continue;
			Candidates.resize(CareerWorldRaceEntrantCount);

			bool Ready = false;
			std::uint64_t RetryAtMs = 0;
			RecordRaceStartTravel(Data, Candidates, TrackIndex, RetryAtMs, Ready);
			if (!Ready)
				continue;
			StartWorldRace(Data, TrackIndex, Candidates, "Open amateur race", {});
		}
	}

	std::vector<Racer*> GetLeagueRunnersByRank(CareerData& Data, CareerLeague& League, bool Top,
		std::size_t Count)
	{
		std::vector<CareerLeagueStanding*> Sorted = GetSortedActiveStandings(Data, League);
		std::vector<Racer*> Runners;
		if (Top)
		{
			for (std::size_t Index = 0; Index < std::min(Count, Sorted.size()); ++Index)
				Runners.push_back(Sorted[Index]->Runner);
		}
		else
		{
			const std::size_t First = Sorted.size() > Count ? Sorted.size() - Count : 0;
			for (std::size_t Index = First; Index < Sorted.size(); ++Index)
				Runners.push_back(Sorted[Index]->Runner);
		}
		return Runners;
	}

	bool AllSeasonsReadyToAdvance(const CareerData& Data)
	{
		for (const CareerLeague& League : Data.Leagues)
		{
			if (League.IsAmateurCup)
				continue;
			if (League.Division == CareerLeagueDivision::Amateur && League.State == CareerLeagueState::Filling)
				continue;
			if (League.State != CareerLeagueState::Complete)
				return false;
		}
		return true;
	}

	void MoveMember(std::vector<std::vector<Racer*>>& Members, std::size_t SourceIndex,
		std::size_t DestinationIndex, Racer* Runner)
	{
		if (Runner == nullptr || SourceIndex >= Members.size() || DestinationIndex >= Members.size())
			return;
		auto& Source = Members[SourceIndex];
		const auto Entry = std::find(Source.begin(), Source.end(), Runner);
		if (Entry == Source.end())
			return;
		Source.erase(Entry);
		auto& Destination = Members[DestinationIndex];
		if (std::find(Destination.begin(), Destination.end(), Runner) == Destination.end())
			Destination.push_back(Runner);
	}

	void AdvanceCareerSeasons(CareerData& Data)
	{
		if (!AllSeasonsReadyToAdvance(Data))
			return;

		std::vector<std::vector<Racer*>> Members(Data.Leagues.size());
		for (std::size_t LeagueIndex = 0; LeagueIndex < Data.Leagues.size(); ++LeagueIndex)
			if (!Data.Leagues[LeagueIndex].IsAmateurCup)
			for (Racer* Runner : Data.Leagues[LeagueIndex].Entrants)
				if (Runner != nullptr && IsRacerActive(Data, Runner))
					Members[LeagueIndex].push_back(Runner);

		const auto IndexOf = [&Data](CareerLeague* League)
		{
			return League == nullptr ? InvalidPlotIndex : static_cast<std::size_t>(League - Data.Leagues.data());
		};
		CareerLeague* Division1 = FindLeague(Data, CareerLeagueDivision::Division1);
		CareerLeague* Division2 = FindLeague(Data, CareerLeagueDivision::Division2);
		CareerLeague* Division3 = FindLeague(Data, CareerLeagueDivision::Division3);
		CareerLeague* Division4 = FindLeague(Data, CareerLeagueDivision::Division4);
		const std::size_t Division1Index = IndexOf(Division1);
		const std::size_t Division2Index = IndexOf(Division2);
		const std::size_t Division3Index = IndexOf(Division3);
		const std::size_t Division4Index = IndexOf(Division4);
		if (Division1 != nullptr && Division2 != nullptr && Division3 != nullptr && Division4 != nullptr)
		{
			for (Racer* Runner : GetLeagueRunnersByRank(Data, *Division1, false, 2))
				MoveMember(Members, Division1Index, Division2Index, Runner);
			for (Racer* Runner : GetLeagueRunnersByRank(Data, *Division2, true, 2))
				MoveMember(Members, Division2Index, Division1Index, Runner);
			for (Racer* Runner : GetLeagueRunnersByRank(Data, *Division2, false, 2))
				MoveMember(Members, Division2Index, Division3Index, Runner);
			for (Racer* Runner : GetLeagueRunnersByRank(Data, *Division3, true, 2))
				MoveMember(Members, Division3Index, Division2Index, Runner);
			for (Racer* Runner : GetLeagueRunnersByRank(Data, *Division3, false, 2))
				MoveMember(Members, Division3Index, Division4Index, Runner);
			for (Racer* Runner : GetLeagueRunnersByRank(Data, *Division4, true, 2))
				MoveMember(Members, Division4Index, Division3Index, Runner);

			std::vector<CareerLeagueStanding*> LicensedAmateurRunners;
			for (CareerLeague& League : Data.Leagues)
				if (!League.IsAmateurCup && League.Division == CareerLeagueDivision::Amateur)
					for (CareerLeagueStanding* Standing : GetSortedActiveStandings(Data, League))
					{
						const CareerRacerState* State = FindRacerState(Data, Standing->Runner);
						const bool HasLicense = Standing->Runner == Data.PlayerLeagueRacer
							? Data.Licenses[0] : State != nullptr && State->HasRacingLicense;
						if (HasLicense)
							LicensedAmateurRunners.push_back(Standing);
					}
			std::stable_sort(LicensedAmateurRunners.begin(), LicensedAmateurRunners.end(),
				[](const CareerLeagueStanding* Left, const CareerLeagueStanding* Right)
				{
					if (Left->Points != Right->Points)
						return Left->Points > Right->Points;
					return Left->Runner->CareerRecord.PositionPoints > Right->Runner->CareerRecord.PositionPoints;
				});
			if (Division4 != nullptr)
				for (std::size_t Index = 0; Index < LicensedAmateurRunners.size() && Index < 2; ++Index)
				{
					const std::size_t SourceIndex = [&Data, &Members, &LicensedAmateurRunners, Index]()
					{
						for (std::size_t LeagueIndex = 0; LeagueIndex < Data.Leagues.size(); ++LeagueIndex)
							if (std::find(Members[LeagueIndex].begin(), Members[LeagueIndex].end(),
								LicensedAmateurRunners[Index]->Runner) != Members[LeagueIndex].end())
								return LeagueIndex;
						return InvalidPlotIndex;
					}();
					MoveMember(Members, SourceIndex, Division4Index, LicensedAmateurRunners[Index]->Runner);
				}

			if (Division4 != nullptr)
				for (Racer* Runner : GetLeagueRunnersByRank(Data, *Division4, false, 2))
				{
					std::size_t AmateurDestination = InvalidPlotIndex;
					for (std::size_t LeagueIndex = 0; LeagueIndex < Data.Leagues.size(); ++LeagueIndex)
						if (!Data.Leagues[LeagueIndex].IsAmateurCup &&
							Data.Leagues[LeagueIndex].Division == CareerLeagueDivision::Amateur &&
							Members[LeagueIndex].size() < CareerLeagueSize &&
							(AmateurDestination == InvalidPlotIndex ||
								Members[LeagueIndex].size() < Members[AmateurDestination].size()))
							AmateurDestination = LeagueIndex;

					if (AmateurDestination == InvalidPlotIndex)
					{
						CareerLeague NewLeague;
						NewLeague.Id = Data.NextLeagueId++;
						NewLeague.Name = MakeNextAmateurLeagueName(Data);
						NewLeague.Division = CareerLeagueDivision::Amateur;
						Data.Leagues.push_back(std::move(NewLeague));
						Members.emplace_back();
						AmateurDestination = Members.size() - 1;
					}

					MoveMember(Members, Division4Index, AmateurDestination, Runner);
				}
		}

		Data.PlayerLeagueId = 0;
		for (CareerRacerState& State : Data.RacerStates)
			if (State.Activity != CareerRacerActivity::Retired)
				State.LeagueId = 0;
		for (std::size_t LeagueIndex = 0; LeagueIndex < Data.Leagues.size(); ++LeagueIndex)
		{
			CareerLeague& League = Data.Leagues[LeagueIndex];
			if (League.IsAmateurCup)
				continue;
			League.Entrants = std::move(Members[LeagueIndex]);
			League.Fixtures.clear();
			League.Standings.clear();
			League.Playoffs = SportRaceCore::PlayoffBracket{};
			League.State = CareerLeagueState::Filling;
			for (Racer* Runner : League.Entrants)
				if (Runner == Data.PlayerLeagueRacer)
					Data.PlayerLeagueId = League.Id;
				else if (CareerRacerState* State = FindRacerState(Data, Runner))
				{
					State->LeagueId = League.Id;
					if (State->Activity != CareerRacerActivity::InRace)
						State->Activity = CareerRacerActivity::InBuilding;
				}
		}
		FillLeagueVacancies(Data);
	}

	void TrimWorldRaceHistory(CareerData& Data)
	{
		if (Data.WorldRaces.size() <= CareerWorldRaceHistoryLimit)
			return;
		for (auto RaceIterator = Data.WorldRaces.begin(); RaceIterator != Data.WorldRaces.end() &&
			Data.WorldRaces.size() > CareerWorldRaceHistoryLimit;)
		{
			if (*RaceIterator != nullptr && (*RaceIterator)->IsComplete && RaceIterator->use_count() == 1)
				RaceIterator = Data.WorldRaces.erase(RaceIterator);
			else
				++RaceIterator;
		}
	}

	struct PlayerLeagueMatch
	{
		CareerLeague* League = nullptr;
		Racer* Opponent = nullptr;
		std::size_t FixtureIndex = InvalidPlotIndex;
		std::size_t PlayoffRoundIndex = InvalidPlotIndex;
		std::size_t PlayoffMatchIndex = InvalidPlotIndex;
		std::size_t TrackPlotIndex = InvalidPlotIndex;
		bool IsPlayoff = false;
		std::string EventName;
	};

	bool FindNextPlayerLeagueMatch(CareerData& Data, PlayerLeagueMatch& Match)
	{
		if (Data.PlayerLeagueRacer == nullptr || Data.PlayerLeagueId == 0)
			return false;

		CareerLeague* League = FindLeague(Data, Data.PlayerLeagueId);
		if (League == nullptr)
			return false;
		const std::uint64_t PlayerId = GetRacerId(Data.PlayerLeagueRacer);
		if (PlayerId == 0)
			return false;

		if (League->State == CareerLeagueState::RegularSeason)
		{
			std::uint32_t EarliestUnresolvedRound = std::numeric_limits<std::uint32_t>::max();
			for (const CareerLeagueFixture& Fixture : League->Fixtures)
				if (Fixture.State == CareerFixtureState::Scheduled ||
					Fixture.State == CareerFixtureState::InProgress)
					EarliestUnresolvedRound = std::min(EarliestUnresolvedRound, Fixture.Fixture.Round);
			if (EarliestUnresolvedRound == std::numeric_limits<std::uint32_t>::max())
				return false;

			for (std::size_t Index = 0; Index < League->Fixtures.size(); ++Index)
			{
				CareerLeagueFixture& Fixture = League->Fixtures[Index];
				if (Fixture.State != CareerFixtureState::Scheduled ||
					Fixture.Fixture.Round != EarliestUnresolvedRound ||
					Fixture.ScheduledAtMs > Data.WorldClock.NowMs())
					continue;

				std::uint64_t OpponentId = 0;
				if (Fixture.Fixture.FirstCompetitorId == PlayerId)
					OpponentId = Fixture.Fixture.SecondCompetitorId;
				else if (Fixture.Fixture.SecondCompetitorId == PlayerId)
					OpponentId = Fixture.Fixture.FirstCompetitorId;
				if (OpponentId == 0)
					continue;

				Match.League = League;
				Match.Opponent = GetRacerById(Data, OpponentId);
				Match.FixtureIndex = Index;
				Match.TrackPlotIndex = Fixture.TrackPlotIndex;
				Match.EventName = League->Name + " - round " +
					std::to_string(Fixture.Fixture.Round + 1);
				return Match.Opponent != nullptr;
			}
			return false;
		}

		if (League->State != CareerLeagueState::Playoffs)
			return false;
		const std::vector<std::size_t> Tracks = GetTrackPlotIndices(Data);
		if (Tracks.empty())
			return false;
		for (std::size_t RoundIndex = 0; RoundIndex < League->Playoffs.Rounds.size(); ++RoundIndex)
			for (std::size_t MatchIndex = 0;
				MatchIndex < League->Playoffs.Rounds[RoundIndex].Matches.size(); ++MatchIndex)
			{
				const SportRaceCore::PlayoffMatch& PlayoffMatch =
					League->Playoffs.Rounds[RoundIndex].Matches[MatchIndex];
				if (PlayoffMatch.IsComplete || !PlayoffMatch.CompetitorAId.has_value() ||
					!PlayoffMatch.CompetitorBId.has_value())
					continue;

				std::uint64_t OpponentId = 0;
				if (*PlayoffMatch.CompetitorAId == PlayerId)
					OpponentId = *PlayoffMatch.CompetitorBId;
				else if (*PlayoffMatch.CompetitorBId == PlayerId)
					OpponentId = *PlayoffMatch.CompetitorAId;
				if (OpponentId == 0)
					continue;

				Match.League = League;
				Match.Opponent = GetRacerById(Data, OpponentId);
				Match.PlayoffRoundIndex = RoundIndex;
				Match.PlayoffMatchIndex = MatchIndex;
				Match.TrackPlotIndex = Tracks[(RoundIndex + MatchIndex + League->Id) % Tracks.size()];
				Match.IsPlayoff = true;
				Match.EventName = League->Name + " playoff round " + std::to_string(RoundIndex + 1);
				return Match.Opponent != nullptr;
			}
		return false;
	}

	bool IsPlayerRaceEntrantAvailable(CareerData& Data, Racer* Runner, std::size_t TrackPlotIndex)
	{
		if (Runner == nullptr || Runner == Data.PlayerLeagueRacer)
			return false;
		const CareerRacerState* State = FindRacerState(Data, Runner);
		if (State == nullptr || State->Activity == CareerRacerActivity::Retired ||
			State->Activity == CareerRacerActivity::InRace ||
			State->Activity == CareerRacerActivity::Training || State->Cash < CareerRaceEntryFee)
			return false;
		return State->Activity != CareerRacerActivity::Travelling ||
			State->DestinationPlotIndex == TrackPlotIndex;
	}

	bool CollectPlayerLeagueRaceEntrants(CareerData& Data, const PlayerLeagueMatch& Match,
		std::vector<Racer*>& Entrants, std::vector<CareerRaceFixtureReference>& Fixtures)
	{
		if (Match.League == nullptr || Match.Opponent == nullptr ||
			Match.TrackPlotIndex >= Data.CityPlots.size() ||
			Data.CityPlots[Match.TrackPlotIndex].Type != Game::Career::CityPlotType::RaceTrack ||
			!IsPlayerRaceEntrantAvailable(Data, Match.Opponent, Match.TrackPlotIndex))
			return false;

		Entrants = { Data.PlayerLeagueRacer, Match.Opponent };
		if (!Match.IsPlayoff)
			Fixtures.push_back({ Match.League->Id, Match.FixtureIndex });

		if (!Match.IsPlayoff && Match.FixtureIndex < Match.League->Fixtures.size())
		{
			const std::uint32_t MatchRound = Match.League->Fixtures[Match.FixtureIndex].Fixture.Round;
			for (std::size_t Index = 0; Index < Match.League->Fixtures.size() &&
				Entrants.size() + 2 <= CareerWorldRaceEntrantCount; ++Index)
			{
				const CareerLeagueFixture& Fixture = Match.League->Fixtures[Index];
				if (Index == Match.FixtureIndex || Fixture.State != CareerFixtureState::Scheduled ||
					Fixture.Fixture.Round != MatchRound ||
					Fixture.ScheduledAtMs > Data.WorldClock.NowMs())
					continue;

				Racer* First = GetRacerById(Data, Fixture.Fixture.FirstCompetitorId);
				Racer* Second = GetRacerById(Data, Fixture.Fixture.SecondCompetitorId);
				if (First == nullptr || Second == nullptr ||
					std::find(Entrants.begin(), Entrants.end(), First) != Entrants.end() ||
					std::find(Entrants.begin(), Entrants.end(), Second) != Entrants.end() ||
					!IsPlayerRaceEntrantAvailable(Data, First, Match.TrackPlotIndex) ||
					!IsPlayerRaceEntrantAvailable(Data, Second, Match.TrackPlotIndex))
					continue;

				Entrants.push_back(First);
				Entrants.push_back(Second);
				Fixtures.push_back({ Match.League->Id, Index });
			}
		}

		return FillWorldRaceToSix(Data, Entrants);
	}
}

void Game::Career::CareerOrchestrator::Init()
{
	if (Data == nullptr || Data->WorldSimulationInitialized)
		return;

	std::mt19937 Random(static_cast<unsigned int>(std::rand()));
	Data->WorldRandom.seed(Random());
	GenerateCity(*Data, Data->WorldRandom);

	Race::RacerNameMaker NameMaker;
	for (std::size_t Index = Data->Racers.Size(); Index < CareerTargetPopulation; ++Index)
		Data->Racers.Make(NameMaker.Make());

	if (Data->OwnedRacers.empty())
	{
		std::vector<Race::Racer*> AvailableRacers = Data->Racers.GetAll();
		std::shuffle(AvailableRacers.begin(), AvailableRacers.end(), Data->WorldRandom);
		const std::size_t OwnedCount = std::min(CareerStarterOwnedRacerCount, AvailableRacers.size());
		Data->OwnedRacers.assign(AvailableRacers.begin(), AvailableRacers.begin() + OwnedCount);
	}

	Data->RacerStates.clear();
	std::vector<Race::Racer*> ActiveRoster = Data->Racers.GetAll();
	ActiveRoster.erase(std::remove_if(ActiveRoster.begin(), ActiveRoster.end(), [this](const Race::Racer* Runner)
		{
			return Runner == nullptr || std::find(Data->OwnedRacers.begin(), Data->OwnedRacers.end(), Runner) !=
				Data->OwnedRacers.end();
		}), ActiveRoster.end());
	for (Race::Racer* Runner : ActiveRoster)
		Data->RacerStates.push_back(MakeRacerWorldState(*Data, Runner));
	Data->TargetAiPopulation = Data->RacerStates.size();
	EnsureRaceTrackCapacity(*Data);

	std::stable_sort(ActiveRoster.begin(), ActiveRoster.end(), [](const Race::Racer* Left, const Race::Racer* Right)
		{
			const double LeftStrength = EstimateRacerStrength(*Left, 500);
			const double RightStrength = EstimateRacerStrength(*Right, 500);
			if (LeftStrength != RightStrength)
				return LeftStrength > RightStrength;
			return Left->GUID.GUID < Right->GUID.GUID;
		});
	Data->Leagues.clear();
	Data->NextLeagueId = 1;
	std::size_t EntrantOffset = 0;
	for (const auto& Division : std::array<std::pair<CareerLeagueDivision, const char*>, 4>{{
		{ CareerLeagueDivision::Division1, "Division 1" },
		{ CareerLeagueDivision::Division2, "Division 2" },
		{ CareerLeagueDivision::Division3, "Division 3" },
		{ CareerLeagueDivision::Division4, "Division 4" }
	}})
	{
		const std::size_t RemainingEntrants = EntrantOffset < ActiveRoster.size()
			? ActiveRoster.size() - EntrantOffset : 0;
		const std::size_t EntrantCount = std::min(CareerLeagueSize, RemainingEntrants);
		std::vector<Race::Racer*> Entrants(ActiveRoster.begin() + EntrantOffset,
			ActiveRoster.begin() + EntrantOffset + EntrantCount);
		AddLeague(*Data, Division.second, Division.first, Entrants);
		EntrantOffset += EntrantCount;
	}

	std::vector<const char*> LeagueNames(AmateurLeagueNames.begin(), AmateurLeagueNames.end());
	std::shuffle(LeagueNames.begin(), LeagueNames.end(), Data->WorldRandom);
	std::vector<Race::Racer*> AmateurRoster(ActiveRoster.begin() + EntrantOffset, ActiveRoster.end());
	std::shuffle(AmateurRoster.begin(), AmateurRoster.end(), Data->WorldRandom);
	std::size_t AmateurOffset = 0;
	for (std::size_t LeagueIndex = 0; LeagueIndex < CareerAmateurLeagueCount; ++LeagueIndex)
	{
		const std::size_t RemainingEntrants = AmateurOffset < AmateurRoster.size()
			? AmateurRoster.size() - AmateurOffset : 0;
		const std::size_t EntrantCount = std::min(CareerLeagueSize, RemainingEntrants);
		std::vector<Race::Racer*> Entrants(AmateurRoster.begin() + AmateurOffset,
			AmateurRoster.begin() + AmateurOffset + EntrantCount);
		AddLeague(*Data, LeagueNames[LeagueIndex], CareerLeagueDivision::Amateur, Entrants);
		AmateurOffset += EntrantCount;
	}

	for (CareerLeague& League : Data->Leagues)
		if (League.Entrants.size() == CareerLeagueSize)
			StartLeagueSeason(*Data, League);
	EnsureOpenAmateurLeague(*Data);
	CareerLeague AmateurCup;
	AmateurCup.Id = CareerAmateurCupId;
	AmateurCup.Name = "Amateur Cup";
	AmateurCup.Division = CareerLeagueDivision::Amateur;
	AmateurCup.IsAmateurCup = true;
	Data->Leagues.push_back(std::move(AmateurCup));
	Data->NextOpenRaceAtMs = Data->WorldClock.NowMs() + CareerOpenRaceIntervalMs;
	Data->NextAmateurCupAtMs = GetNextWorldDayBoundary(Data->WorldClock.NowMs());
	Data->WorldSimulationInitialized = true;
}

void Game::Career::CareerOrchestrator::BeginInitialWorldMonthSimulation()
{
	if (Data == nullptr || Data->InitialWorldMonthSimulated || Data->InitialWorldMonthSimulationInProgress ||
		Data->InitialWorldMonthSimulationFailed)
		return;

	Init();
	if (!Data->WorldSimulationInitialized)
	{
		Data->InitialWorldMonthSimulationFailed = true;
		spdlog::error("Cannot simulate the career start month before world initialization.");
		return;
	}

	const std::uint64_t TimeScale = Data->WorldClock.GetTimeScale();
	if (TimeScale == 0 || CareerInitialWorldMonthMs % TimeScale != 0)
	{
		Data->InitialWorldMonthSimulationFailed = true;
		spdlog::error("Cannot simulate an exact career start month at world time scale {}.", TimeScale);
		return;
	}

	Data->InitialWorldMonthTotalRealMs = CareerInitialWorldMonthMs / TimeScale;
	Data->InitialWorldMonthElapsedRealMs = 0;
	Data->InitialWorldMonthStartingWorldMs = Data->WorldClock.NowMs();
	Data->InitialWorldMonthStartingRealMs = Data->RealTimeMs;
	if (CareerInitialWorldMonthMs > std::numeric_limits<std::uint64_t>::max() -
		Data->InitialWorldMonthStartingWorldMs ||
		Data->InitialWorldMonthTotalRealMs > std::numeric_limits<std::uint64_t>::max() -
		Data->InitialWorldMonthStartingRealMs)
	{
		Data->InitialWorldMonthSimulationFailed = true;
		spdlog::error("Cannot simulate the career start month: the world or real-time clock would overflow.");
		return;
	}

	Data->InitialWorldMonthProcessingMs = 0;
	Data->InitialWorldMonthSimulationInProgress = true;
}

bool Game::Career::CareerOrchestrator::AdvanceInitialWorldMonthSimulation(std::size_t MaximumUpdates)
{
	if (Data == nullptr)
		return false;
	if (Data->InitialWorldMonthSimulated)
		return true;
	if (!Data->InitialWorldMonthSimulationInProgress)
	{
		BeginInitialWorldMonthSimulation();
		if (!Data->InitialWorldMonthSimulationInProgress)
			return false;
	}

	const auto SimulationStartedAt = std::chrono::steady_clock::now();
	bool ClockOverflow = false;
	for (std::size_t UpdateIndex = 0; UpdateIndex < MaximumUpdates; ++UpdateIndex)
	{
		const bool MonthComplete = Data->InitialWorldMonthElapsedRealMs >=
			Data->InitialWorldMonthTotalRealMs;
		if (MonthComplete && AllActiveRacersHaveHistory(*Data))
			break;
		if (MonthComplete && Data->InitialWorldMonthElapsedRealMs - Data->InitialWorldMonthTotalRealMs >=
			CareerInitialWorldHistoryCatchupLimitMs)
			break;

		const std::uint64_t RemainingMs = MonthComplete
			? CareerInitialWorldHistoryCatchupLimitMs -
				(Data->InitialWorldMonthElapsedRealMs - Data->InitialWorldMonthTotalRealMs)
			: Data->InitialWorldMonthTotalRealMs - Data->InitialWorldMonthElapsedRealMs;
		const std::uint64_t StepMs = std::min(CareerInitialWorldSimulationStepMs, RemainingMs);
		if (MonthComplete)
		{
			const std::uint64_t TimeScale = Data->WorldClock.GetTimeScale();
			const std::uint64_t Maximum = std::numeric_limits<std::uint64_t>::max();
			if (TimeScale == 0 || StepMs > Maximum - Data->InitialWorldMonthElapsedRealMs ||
				StepMs > Maximum - Data->RealTimeMs || StepMs > Maximum / TimeScale ||
				StepMs * TimeScale > Maximum - Data->WorldClock.NowMs())
			{
				ClockOverflow = true;
				break;
			}
		}
		Update(StepMs);
		Data->InitialWorldMonthElapsedRealMs += StepMs;
	}
	const auto SimulationEndedAt = std::chrono::steady_clock::now();
	const auto ProcessingDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
		SimulationEndedAt - SimulationStartedAt).count();
	if (ProcessingDuration > 0)
		Data->InitialWorldMonthProcessingMs += static_cast<std::uint64_t>(ProcessingDuration);

	if (ClockOverflow)
	{
		Data->InitialWorldMonthSimulationInProgress = false;
		Data->InitialWorldMonthSimulationFailed = true;
		spdlog::error("Career start history catch-up would overflow the world or real-time clock.");
		return false;
	}

	if (Data->InitialWorldMonthElapsedRealMs < Data->InitialWorldMonthTotalRealMs)
		return false;
	if (!AllActiveRacersHaveHistory(*Data))
	{
		if (Data->InitialWorldMonthElapsedRealMs - Data->InitialWorldMonthTotalRealMs >=
			CareerInitialWorldHistoryCatchupLimitMs)
		{
			Data->InitialWorldMonthSimulationInProgress = false;
			Data->InitialWorldMonthSimulationFailed = true;
			spdlog::error("Active AI racers did not all complete a first race within the career-start history catch-up limit.");
		}
		return false;
	}

	const std::uint64_t ElapsedRealMs = Data->InitialWorldMonthElapsedRealMs;
	const std::uint64_t TimeScale = Data->WorldClock.GetTimeScale();
	if (TimeScale == 0 || ElapsedRealMs > std::numeric_limits<std::uint64_t>::max() -
		Data->InitialWorldMonthStartingRealMs || ElapsedRealMs >
		(std::numeric_limits<std::uint64_t>::max() - Data->InitialWorldMonthStartingWorldMs) / TimeScale)
	{
		Data->InitialWorldMonthSimulationInProgress = false;
		Data->InitialWorldMonthSimulationFailed = true;
		spdlog::error("Career start simulation exceeded the world or real-time clock range.");
		return false;
	}
	const std::uint64_t ExpectedWorldMs = Data->InitialWorldMonthStartingWorldMs + ElapsedRealMs * TimeScale;
	const std::uint64_t ExpectedRealMs = Data->InitialWorldMonthStartingRealMs +
		ElapsedRealMs;
	if (Data->WorldClock.NowMs() != ExpectedWorldMs || Data->RealTimeMs != ExpectedRealMs)
	{
		Data->InitialWorldMonthSimulationInProgress = false;
		Data->InitialWorldMonthSimulationFailed = true;
		spdlog::error("Career start simulation did not advance the world and real-time clocks as expected.");
		return false;
	}

	Data->InitialWorldMonthSimulationInProgress = false;
	Data->InitialWorldMonthSimulated = true;
	Data->NextAmateurCupAtMs = GetNextWorldDayBoundary(Data->WorldClock.NowMs());
	Data->InitialWorldMonthClockResetPending = true;

	std::uint64_t HistoryEntryCount = 0;
	for (const Racer* Runner : Data->Racers.GetAll())
		if (Runner != nullptr)
			HistoryEntryCount += static_cast<std::uint64_t>(Runner->CareerRecord.History.size());
	const std::size_t ActiveRacersWithHistory = static_cast<std::size_t>(std::count_if(
		Data->RacerStates.begin(), Data->RacerStates.end(), [](const CareerRacerState& State)
		{
			return State.Activity != CareerRacerActivity::Retired && State.Runner != nullptr &&
				!State.Runner->CareerRecord.History.empty();
		}));
	spdlog::info("Career start simulated at least 30 world days in {} ms; {}/{} active AI racers have race history ({} results).",
		Data->InitialWorldMonthProcessingMs, ActiveRacersWithHistory, GetActiveRacerCount(), HistoryEntryCount);
	return true;
}

void Game::Career::CareerOrchestrator::SimulateInitialWorldMonth()
{
	BeginInitialWorldMonthSimulation();
	while (Data != nullptr && Data->InitialWorldMonthSimulationInProgress)
		AdvanceInitialWorldMonthSimulation(4096);
}

bool Game::Career::CareerOrchestrator::CanJoinLeague(std::uint64_t LeagueId) const
{
	if (Data == nullptr || Profile == nullptr || !Data->WorldSimulationInitialized ||
		Data->PlayerLeagueRacer != nullptr || Data->PlayerLeagueId != 0 ||
		Data->PendingPlayerLeagueRace.Preparing || Data->PendingPlayerLeagueRace.Active)
		return false;

	const CareerLeague* League = FindLeague(*Data, LeagueId);
	return League != nullptr && League->Division == CareerLeagueDivision::Amateur &&
		!League->IsAmateurCup && League->State == CareerLeagueState::Filling &&
		League->Entrants.size() < CareerLeagueSize;
}

bool Game::Career::CareerOrchestrator::JoinLeague(std::uint64_t LeagueId)
{
	if (!CanJoinLeague(LeagueId))
		return false;

	CareerLeague* League = FindLeague(*Data, LeagueId);
	if (League == nullptr)
		return false;
	const std::size_t MissingAiCount = CareerLeagueSize - League->Entrants.size() - 1;
	if (MissingAiCount > std::numeric_limits<std::size_t>::max() - Data->TargetAiPopulation)
	{
		spdlog::error("Cannot join league {}: the AI population count would overflow.", League->Name);
		return false;
	}

	unsigned int PlayerId = Profile->MainFella.GUID.GUID;
	if (PlayerId == 0 || Data->Racers.Get(PlayerId) != nullptr)
	{
		std::uint64_t CandidateId = 1;
		while (CandidateId <= std::numeric_limits<unsigned int>::max() &&
			Data->Racers.Get(static_cast<unsigned int>(CandidateId)) != nullptr)
			++CandidateId;
		if (CandidateId > std::numeric_limits<unsigned int>::max())
		{
			spdlog::error("Cannot join league {}: no unique racer identifier remains.", League->Name);
			return false;
		}
		PlayerId = static_cast<unsigned int>(CandidateId);
	}

	Profile->MainFella.GUID.GUID = PlayerId;
	Data->PlayerLeagueRacer = &Profile->MainFella;
	Data->PlayerLeagueId = League->Id;
	League->Entrants.push_back(Data->PlayerLeagueRacer);

	Race::RacerNameMaker NameMaker;
	for (std::size_t Index = 0; Index < MissingAiCount; ++Index)
	{
		Racer* Runner = Data->Racers.Make(NameMaker.Make());
		if (Runner == nullptr)
		{
			spdlog::error("Could not fill league {} after enrolling the player.", League->Name);
			break;
		}

		CareerRacerState State = MakeRacerWorldState(*Data, Runner);
		State.LeagueId = League->Id;
		State.Activity = CareerRacerActivity::InBuilding;
		Data->RacerStates.push_back(std::move(State));
		League->Entrants.push_back(Runner);
		++Data->TargetAiPopulation;
	}

	EnsureRaceTrackCapacity(*Data);
	FillLeagueVacancies(*Data);
	return true;
}

bool Game::Career::CareerOrchestrator::CanRaceNextPlayerLeagueMatch() const
{
	if (Data == nullptr || Profile == nullptr || Data->PlayerLeagueRacer == nullptr ||
		Data->PlayerLeagueId == 0 || Data->PendingPlayerLeagueRace.Active)
		return false;

	const CareerPendingLeagueRace& Pending = Data->PendingPlayerLeagueRace;
	if (Pending.Preparing)
	{
		if (Pending.Entrants.size() != CareerWorldRaceEntrantCount ||
			Pending.TrackPlotIndex >= Data->CityPlots.size() ||
			Data->CityPlots[Pending.TrackPlotIndex].Type != CityPlotType::RaceTrack)
			return false;
		for (const CareerRaceFixtureReference& Reference : Pending.LeagueFixtures)
		{
			const CareerLeague* League = FindLeague(*Data, Reference.LeagueId);
			if (League == nullptr || Reference.FixtureIndex >= League->Fixtures.size() ||
				League->Fixtures[Reference.FixtureIndex].State != CareerFixtureState::InProgress)
				return false;
		}
		for (Racer* Runner : Pending.Entrants)
		{
			if (Runner == Data->PlayerLeagueRacer)
				continue;
			const CareerRacerState* State = FindRacerState(*Data, Runner);
			if (State == nullptr || State->Activity == CareerRacerActivity::Retired ||
				State->Activity == CareerRacerActivity::InRace ||
				State->Activity == CareerRacerActivity::Training ||
				State->Activity == CareerRacerActivity::Travelling ||
				State->Cash < CareerRaceEntryFee || State->LocationPlotIndex != Pending.TrackPlotIndex)
				return false;
		}
		return true;
	}

	PlayerLeagueMatch Match;
	std::vector<Racer*> Entrants;
	std::vector<CareerRaceFixtureReference> Fixtures;
	return FindNextPlayerLeagueMatch(*Data, Match) &&
		CollectPlayerLeagueRaceEntrants(*Data, Match, Entrants, Fixtures);
}

bool Game::Career::CareerOrchestrator::PreparePlayerLeagueRace(Race::Race& RaceBuffer)
{
	if (Data == nullptr || Profile == nullptr || Data->PendingPlayerLeagueRace.Active)
		return false;

	if (!Data->PendingPlayerLeagueRace.Preparing)
	{
		PlayerLeagueMatch Match;
		std::vector<Racer*> Entrants;
		std::vector<CareerRaceFixtureReference> Fixtures;
		if (!FindNextPlayerLeagueMatch(*Data, Match) ||
			!CollectPlayerLeagueRaceEntrants(*Data, Match, Entrants, Fixtures))
			return false;

		std::shuffle(Entrants.begin(), Entrants.end(), Data->WorldRandom);
		CareerPendingLeagueRace& Pending = Data->PendingPlayerLeagueRace;
		Pending = CareerPendingLeagueRace{};
		Pending.LeagueId = Match.League->Id;
		Pending.FixtureIndex = Match.FixtureIndex;
		Pending.PlayoffRoundIndex = Match.PlayoffRoundIndex;
		Pending.PlayoffMatchIndex = Match.PlayoffMatchIndex;
		Pending.TrackPlotIndex = Match.TrackPlotIndex;
		Pending.Entrants = std::move(Entrants);
		Pending.LeagueFixtures = std::move(Fixtures);
		Pending.EventName = Match.EventName;
		Pending.IsPlayoff = Match.IsPlayoff;
		Pending.Preparing = true;

		for (const CareerRaceFixtureReference& Reference : Pending.LeagueFixtures)
			if (CareerLeague* League = FindLeague(*Data, Reference.LeagueId);
				League != nullptr && Reference.FixtureIndex < League->Fixtures.size())
			{
				CareerLeagueFixture& Fixture = League->Fixtures[Reference.FixtureIndex];
				Fixture.State = CareerFixtureState::InProgress;
				Fixture.RaceId = 0;
				Fixture.TrackPlotIndex = Pending.TrackPlotIndex;
			}

		for (Racer* Runner : Pending.Entrants)
		{
			if (Runner == Data->PlayerLeagueRacer)
				continue;
			CareerRacerState* State = FindRacerState(*Data, Runner);
			if (State != nullptr && State->Activity != CareerRacerActivity::Travelling &&
				State->LocationPlotIndex != Pending.TrackPlotIndex)
				BeginTravel(*Data, *State, Pending.TrackPlotIndex);
		}
	}

	if (!CanRaceNextPlayerLeagueMatch())
		return false;

	CareerPendingLeagueRace& Pending = Data->PendingPlayerLeagueRace;
	if (Pending.TrackPlotIndex >= Data->CityPlots.size() ||
		Data->CityPlots[Pending.TrackPlotIndex].TrackLength == 0)
	{
		spdlog::error("Cannot start player league race '{}': the selected track is unavailable.", Pending.EventName);
		Race::RaceResult FailedResult;
		FailedResult.Error = Race::RaceError::MissingTrack;
		CompletePlayerLeagueRace(FailedResult);
		return false;
	}

	Race::Race PreparedRace(Data->WorldRandom());
	if (PreparedRace.ThisTrack == nullptr)
	{
		spdlog::error("Cannot start player league race '{}': the race has no track object.", Pending.EventName);
		Race::RaceResult FailedResult;
		FailedResult.Error = Race::RaceError::MissingTrack;
		CompletePlayerLeagueRace(FailedResult);
		return false;
	}
	PreparedRace.Racers = Pending.Entrants;
	PreparedRace.ThisTrack->Length = static_cast<Race::TrackLength>(
		Data->CityPlots[Pending.TrackPlotIndex].TrackLength);
	PreparedRace.Financials = Race::RaceFinancials(CareerRaceFirstPrize, CareerRaceEntryFee);
	Pending.Financials = PreparedRace.Financials;
	Pending.Preparing = false;
	Pending.Active = true;

	for (Racer* Runner : Pending.Entrants)
	{
		if (Runner == Data->PlayerLeagueRacer)
			continue;
		CareerRacerState* State = FindRacerState(*Data, Runner);
		if (State == nullptr)
		{
			spdlog::error("Cannot start player league race '{}': a career entrant has no world state.",
				Pending.EventName);
			Race::RaceResult FailedResult;
			FailedResult.Error = Race::RaceError::SimulationFailed;
			CompletePlayerLeagueRace(FailedResult);
			return false;
		}
		State->Cash -= CareerRaceEntryFee;
		State->Activity = CareerRacerActivity::InRace;
		State->ActiveRaceId = 0;
		State->ActionEndsAtRealMs = Data->RealTimeMs;
		State->LocationPlotIndex = Pending.TrackPlotIndex;
		State->DestinationPlotIndex = Pending.TrackPlotIndex;
	}

	RaceBuffer = std::move(PreparedRace);
	return true;
}

void Game::Career::CareerOrchestrator::CompletePlayerLeagueRace(const Race::RaceResult& Result)
{
	if (Data == nullptr || (!Data->PendingPlayerLeagueRace.Preparing &&
		!Data->PendingPlayerLeagueRace.Active))
		return;

	CareerPendingLeagueRace Pending = std::move(Data->PendingPlayerLeagueRace);
	Data->PendingPlayerLeagueRace = CareerPendingLeagueRace{};
	CareerLeague* League = FindLeague(*Data, Pending.LeagueId);
	bool Successful = Pending.Active && League != nullptr && Result.Error == Race::RaceError::None &&
		Result.RacerResults.size() == Pending.Entrants.size();
	if (Successful)
		for (Racer* Runner : Pending.Entrants)
			if (Runner == nullptr || std::none_of(Result.RacerResults.begin(), Result.RacerResults.end(),
				[Runner](const Race::RacerRaceResult& Entry) { return Entry.Racer == Runner; }))
			{
				Successful = false;
				break;
			}

	std::vector<Racer*> BankruptRacers;
	for (Racer* Runner : Pending.Entrants)
	{
		if (Runner == nullptr || Runner == Data->PlayerLeagueRacer)
			continue;
		if (CareerRacerState* State = FindRacerState(*Data, Runner))
		{
			if (Successful)
			{
				const auto RacerResult = std::find_if(Result.RacerResults.begin(), Result.RacerResults.end(),
					[Runner](const Race::RacerRaceResult& Entry) { return Entry.Racer == Runner; });
				State->Cash += static_cast<int>(GetPrizeForPosition(Pending.Financials, RacerResult->Position));
				if (State->Cash <= 0)
					BankruptRacers.push_back(Runner);
			}
			else if (Pending.Active)
				State->Cash += CareerRaceEntryFee;

			if (State->Activity != CareerRacerActivity::Retired)
			{
				State->Activity = CareerRacerActivity::Resting;
				State->ActiveRaceId = 0;
				State->ActionEndsAtRealMs = Data->RealTimeMs + CareerPostRaceRestMs;
				State->LocationPlotIndex = Pending.TrackPlotIndex;
				State->DestinationPlotIndex = Pending.TrackPlotIndex;
			}
		}
	}

	if (Successful && Pending.IsPlayoff)
	{
		if (League->State != CareerLeagueState::Playoffs ||
			Pending.PlayoffRoundIndex >= League->Playoffs.Rounds.size() ||
			Pending.PlayoffMatchIndex >= League->Playoffs.Rounds[Pending.PlayoffRoundIndex].Matches.size())
		{
			spdlog::error("Could not resolve player playoff race '{}': the match no longer exists.", Pending.EventName);
			Successful = false;
		}
		else
		{
			const SportRaceCore::PlayoffMatch& Match =
				League->Playoffs.Rounds[Pending.PlayoffRoundIndex].Matches[Pending.PlayoffMatchIndex];
			Racer* First = GetRacerById(*Data, Match.CompetitorAId.value_or(0));
			Racer* Second = GetRacerById(*Data, Match.CompetitorBId.value_or(0));
			std::uint64_t WinnerId = GetFixtureWinner(Result, First, Second);
			if (WinnerId == 0)
				WinnerId = Match.CompetitorAId.value_or(0);
			if (WinnerId == 0 || !SportRaceCore::SetPlayoffMatchWinner(League->Playoffs,
				Pending.PlayoffRoundIndex, Pending.PlayoffMatchIndex, WinnerId))
				spdlog::error("Could not advance the playoff winner in league {}.", League->Name);
		}
	}
	else if (Successful)
	{
		for (const CareerRaceFixtureReference& Reference : Pending.LeagueFixtures)
		{
			CareerLeague* FixtureLeague = FindLeague(*Data, Reference.LeagueId);
			if (FixtureLeague == nullptr || Reference.FixtureIndex >= FixtureLeague->Fixtures.size())
			{
				spdlog::error("Could not resolve a player league fixture for race '{}'.", Pending.EventName);
				continue;
			}
			CareerLeagueFixture& Fixture = FixtureLeague->Fixtures[Reference.FixtureIndex];
			Racer* First = GetRacerById(*Data, Fixture.Fixture.FirstCompetitorId);
			Racer* Second = GetRacerById(*Data, Fixture.Fixture.SecondCompetitorId);
			RecordLeagueFixture(*FixtureLeague, Fixture, GetFixtureWinner(Result, First, Second));
		}
	}

	if (!Successful)
		for (const CareerRaceFixtureReference& Reference : Pending.LeagueFixtures)
			if (CareerLeague* FixtureLeague = FindLeague(*Data, Reference.LeagueId);
				FixtureLeague != nullptr && Reference.FixtureIndex < FixtureLeague->Fixtures.size())
			{
				CareerLeagueFixture& Fixture = FixtureLeague->Fixtures[Reference.FixtureIndex];
				Fixture.State = CareerFixtureState::Scheduled;
				Fixture.RaceId = 0;
				Fixture.ScheduledAtMs = GetWorldTimeAfterRealDelay(*Data, CareerRaceRetryDelayMs);
			}

	for (Racer* Runner : BankruptRacers)
		RetireAndReplaceRacer(*Data, Runner);
	if (Successful)
		UpdateLeagueStates(*Data);
	FillLeagueVacancies(*Data);
}

std::size_t Game::Career::CareerOrchestrator::SpawnRacers(std::size_t Count)
{
	if (Data == nullptr || Count == 0)
		return 0;
	if (!Data->WorldSimulationInitialized)
	{
		spdlog::error("Cannot spawn career racers before the world simulation is initialized.");
		return 0;
	}
	if (Count > std::numeric_limits<std::size_t>::max() - Data->TargetAiPopulation)
	{
		spdlog::error("Cannot spawn {} career racers: the AI population count would overflow.", Count);
		return 0;
	}

	Race::RacerNameMaker NameMaker;
	std::size_t SpawnedCount = 0;
	for (; SpawnedCount < Count; ++SpawnedCount)
	{
		Racer* Runner = Data->Racers.Make(NameMaker.Make());
		if (Runner == nullptr)
		{
			spdlog::error("Could not spawn career racer {} of {}.", SpawnedCount + 1, Count);
			break;
		}
		Data->RacerStates.push_back(MakeRacerWorldState(*Data, Runner));
	}

	Data->TargetAiPopulation += SpawnedCount;
	EnsureRaceTrackCapacity(*Data);
	FillLeagueVacancies(*Data);
	return SpawnedCount;
}

void Game::Career::CareerOrchestrator::Update(std::uint64_t Ms)
{
	if (Data == nullptr || !Data->WorldSimulationInitialized)
		return;

	if (Ms > std::numeric_limits<std::uint64_t>::max() - Data->RealTimeMs)
	{
		spdlog::error("Career real-time clock overflowed; world simulation update was skipped.");
		return;
	}
	if (!Data->WorldClock.Advance(Ms))
	{
		spdlog::error("Career world clock overflowed; world simulation update was skipped.");
		return;
	}
	Data->RealTimeMs += Ms;
	TickWorldRaces(*Data, Ms);
	UpdateAgentActivities(*Data);
	UpdateLicenseStatus(*Data);
	UpdateLeagueStates(*Data);
	AdvanceCareerSeasons(*Data);
	FillLeagueVacancies(*Data);
	TryStartLeagueRaces(*Data);
	StartDailyAmateurCup(*Data);
	TryStartPlayoffRaces(*Data);
	TryStartOpenRaces(*Data);
	UpdateLeagueStates(*Data);
	AdvanceCareerSeasons(*Data);
	TrimWorldRaceHistory(*Data);
}

std::size_t Game::Career::CareerOrchestrator::GetActiveRacerCount() const
{
	if (Data == nullptr)
		return 0;
	return static_cast<std::size_t>(std::count_if(Data->RacerStates.begin(), Data->RacerStates.end(),
		[](const CareerRacerState& State) { return State.Activity != CareerRacerActivity::Retired; }));
}

std::size_t Game::Career::CareerOrchestrator::GetActiveRaceCount() const
{
	if (Data == nullptr)
		return 0;
	return static_cast<std::size_t>(std::count_if(Data->WorldRaces.begin(), Data->WorldRaces.end(),
		[](const std::shared_ptr<CareerWorldRace>& WorldRace)
		{
			return WorldRace != nullptr && !WorldRace->IsComplete;
		}));
}

std::string Game::Career::CareerRacerActivityToString(CareerRacerActivity Activity)
{
	switch (Activity)
	{
	case CareerRacerActivity::InBuilding:
		return "In building";
	case CareerRacerActivity::Travelling:
		return "Travelling";
	case CareerRacerActivity::Training:
		return "Training";
	case CareerRacerActivity::InRace:
		return "In race";
	case CareerRacerActivity::Resting:
		return "Resting";
	case CareerRacerActivity::WaitingForLeague:
		return "Waiting for league";
	case CareerRacerActivity::Retired:
		return "Retired";
	}
	return "Unknown";
}

std::string Game::Career::CareerLeagueDivisionToString(CareerLeagueDivision Division)
{
	switch (Division)
	{
	case CareerLeagueDivision::Amateur:
		return "Amateur";
	case CareerLeagueDivision::Division4:
		return "Division 4";
	case CareerLeagueDivision::Division3:
		return "Division 3";
	case CareerLeagueDivision::Division2:
		return "Division 2";
	case CareerLeagueDivision::Division1:
		return "Division 1";
	}
	return "Unknown";
}

std::string Game::Career::CareerLeagueStateToString(CareerLeagueState State)
{
	switch (State)
	{
	case CareerLeagueState::Filling:
		return "Filling places";
	case CareerLeagueState::RegularSeason:
		return "Regular season";
	case CareerLeagueState::Playoffs:
		return "Playoffs";
	case CareerLeagueState::Complete:
		return "Season complete";
	}
	return "Unknown";
}