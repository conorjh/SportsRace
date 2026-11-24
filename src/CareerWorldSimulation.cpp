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

namespace CareerWorldSimulationDetail
{
	using Game::Career::CareerData;
	using Game::Career::CareerCompetitionLevel;
	using Game::Career::CareerFixtureState;
	using Game::Career::CareerHistoricalRace;
	using Game::Career::CareerLeague;
	using Game::Career::CareerLeagueDivision;
	using Game::Career::CareerLeagueFixture;
	using Game::Career::CareerLeagueStanding;
	using Game::Career::CareerLeagueState;
	using Game::Career::CareerPendingLeagueRace;
	using Game::Career::CareerPendingOpenRace;
	using Game::Career::CareerPendingWorldPlayoffRace;
	using Game::Career::CareerRacerActivity;
	using Game::Career::CareerRacerState;
	using Game::Career::CareerRaceFixtureReference;
	using Game::Career::CareerWorldRace;
	using Game::Career::CareerWorldCity;
	using Game::Career::CareerWorldCountry;
	using Game::Career::CareerWorldRegion;
	using Game::Race::Race;
	using Game::Race::RaceResult;
	using Game::Race::RacerCareerHistoryEntry;
	using Game::Race::Racer;

	constexpr unsigned int CareerCityGridWidth = 8;
	constexpr unsigned int CareerCityGridHeight = 5;
	constexpr unsigned int CareerCitySpriteCount = 12;
	constexpr std::size_t CareerStarterOwnedRacerCount = 2;
	constexpr std::size_t CareerTargetPopulation = 322;
	constexpr std::size_t CareerTargetAiPopulation = CareerTargetPopulation - CareerStarterOwnedRacerCount;
	constexpr std::size_t CareerLeagueSize = 8;
	constexpr std::size_t CareerPreviousLeagueSize = 6;
	constexpr std::uint64_t CareerAmateurCupId = std::uint64_t{ 1 } << 63;
	constexpr std::size_t CareerMinimumRaceTrackCount = 5;
	constexpr std::size_t CareerRacersPerRaceTrack = 20;
	constexpr std::size_t CareerFullFieldRaceCount = (CareerLeagueSize + 1) / 2;
	constexpr std::size_t CareerAmateurLeagueHeatCount = 1;
	constexpr std::size_t CareerRegionCount = 2;
	constexpr std::size_t CareerCitiesPerRegion = 3;
	constexpr std::size_t CareerWorldRaceEntrantCount = 6;
	constexpr std::size_t CareerPlayoffRaceEntrantCount = 2;
	constexpr std::size_t CareerWorldRaceHistoryLimit = 1024;
	constexpr int CareerStartingCash = 1000;
	constexpr int CareerLicenseSavingsTarget = CareerStartingCash * 2;
	constexpr int CareerRacingLicensePrice = CareerStartingCash;
	constexpr int CareerRaceEntryFee = 50;
	constexpr unsigned int CareerRaceFirstPrize = 100;
	constexpr int CareerTrainingFee = 10;
	constexpr std::uint64_t CareerLeagueFirstFixtureDelayMs = 1000;
	constexpr std::uint64_t CareerLeagueRoundIntervalMs = 60000;
	constexpr std::uint64_t CareerAmateurCupIntervalMs = 24ull * 60 * 60 * 1000;
	constexpr std::uint64_t CareerOpenRaceIntervalMs = 10000;
	constexpr std::uint64_t CareerTrainingDurationMs = 10000;
	constexpr std::uint64_t CareerRecoveryIntervalMs = 5000;
	constexpr std::uint64_t CareerRaceRetryDelayMs = 2000;
	constexpr std::uint64_t CareerTravelBaseMs = 1000;
	constexpr std::uint64_t CareerTravelPerGridStepMs = 1500;
	constexpr std::uint64_t CareerPostRaceRestMs = 2000;

	bool IsFullFieldLeagueFixture(const CareerLeagueFixture& Fixture)
	{
		return Fixture.Fixture.FirstCompetitorId == 0 && Fixture.Fixture.SecondCompetitorId == 0;
	}

	std::vector<Racer*> BuildCareerLeagueRaceEntrants(const CareerLeague& League, std::size_t RaceIndex)
	{
		std::vector<Racer*> Entrants;
		if (League.Entrants.size() < CareerWorldRaceEntrantCount)
			return Entrants;

		Entrants.reserve(CareerWorldRaceEntrantCount);
		const std::size_t FirstIndex = (RaceIndex * CareerWorldRaceEntrantCount) % League.Entrants.size();
		for (std::size_t Offset = 0; Offset < CareerWorldRaceEntrantCount; ++Offset)
			Entrants.push_back(League.Entrants[(FirstIndex + Offset) % League.Entrants.size()]);
		return Entrants;
	}

	bool MigrateLegacyLeagueFixtures(CareerData& Data)
	{
		struct LeagueFixtureMigration
		{
			std::uint64_t LeagueId = 0;
			std::vector<std::size_t> NewFixtureByOldIndex;
		};

		bool MigratedAny = false;
		std::vector<LeagueFixtureMigration> Migrations;
		for (CareerLeague& League : Data.Leagues)
		{
			if (League.IsCup || League.IsAmateurCup || League.IsOpenLobby || League.Fixtures.empty() ||
				!std::any_of(League.Fixtures.begin(), League.Fixtures.end(), [](const CareerLeagueFixture& Fixture)
				{
					return !IsFullFieldLeagueFixture(Fixture);
				}))
				continue;

			std::vector<std::uint32_t> Rounds;
			Rounds.reserve(League.Fixtures.size());
			for (const CareerLeagueFixture& Fixture : League.Fixtures)
				Rounds.push_back(Fixture.Fixture.Round);
			std::sort(Rounds.begin(), Rounds.end());
			Rounds.erase(std::unique(Rounds.begin(), Rounds.end()), Rounds.end());

			std::vector<CareerLeagueFixture> FullFieldFixtures;
			FullFieldFixtures.reserve(Rounds.size());
			std::vector<std::size_t> NewFixtureByOldIndex(League.Fixtures.size(),
				std::numeric_limits<std::size_t>::max());
			for (const std::uint32_t Round : Rounds)
			{
				const auto FirstFixture = std::find_if(League.Fixtures.begin(), League.Fixtures.end(),
					[Round](const CareerLeagueFixture& Fixture) { return Fixture.Fixture.Round == Round; });
				if (FirstFixture == League.Fixtures.end())
					continue;

				CareerLeagueFixture FullFieldFixture;
				FullFieldFixture.Fixture.Round = Round;
				FullFieldFixture.TrackPlotIndex = FirstFixture->TrackPlotIndex;
				FullFieldFixture.ScheduledAtMs = FirstFixture->ScheduledAtMs;
				const auto InProgressFixture = std::find_if(League.Fixtures.begin(), League.Fixtures.end(),
					[Round](const CareerLeagueFixture& Fixture)
					{
						return Fixture.Fixture.Round == Round && Fixture.State == CareerFixtureState::InProgress;
					});
				const bool IsRoundComplete = std::all_of(League.Fixtures.begin(), League.Fixtures.end(),
					[Round](const CareerLeagueFixture& Fixture)
					{
						return Fixture.Fixture.Round != Round || Fixture.State == CareerFixtureState::Complete;
					});
				if (InProgressFixture != League.Fixtures.end())
				{
					FullFieldFixture.State = CareerFixtureState::InProgress;
					FullFieldFixture.RaceId = InProgressFixture->RaceId;
					FullFieldFixture.TrackPlotIndex = InProgressFixture->TrackPlotIndex;
				}
				else if (IsRoundComplete)
				{
					FullFieldFixture.State = CareerFixtureState::Complete;
					FullFieldFixture.HistoricalRaceId = FirstFixture->HistoricalRaceId;
					const bool SameRace = std::all_of(League.Fixtures.begin(), League.Fixtures.end(),
						[Round, RaceId = FirstFixture->RaceId](const CareerLeagueFixture& Fixture)
						{
							return Fixture.Fixture.Round != Round || Fixture.RaceId == RaceId;
						});
					FullFieldFixture.RaceId = SameRace ? FirstFixture->RaceId : 0;
				}
				else
					for (const CareerLeagueFixture& Fixture : League.Fixtures)
						if (Fixture.Fixture.Round == Round)
							FullFieldFixture.ScheduledAtMs = std::min(FullFieldFixture.ScheduledAtMs,
								Fixture.ScheduledAtMs);
				for (std::size_t OldIndex = 0; OldIndex < League.Fixtures.size(); ++OldIndex)
					if (League.Fixtures[OldIndex].Fixture.Round == Round)
						NewFixtureByOldIndex[OldIndex] = FullFieldFixtures.size();
				FullFieldFixtures.push_back(std::move(FullFieldFixture));
			}

			Migrations.push_back({ League.Id, std::move(NewFixtureByOldIndex) });
			League.Fixtures = std::move(FullFieldFixtures);
			for (CareerLeagueStanding& Standing : League.Standings)
			{
				Standing.Wins = 0;
				Standing.Draws = 0;
				Standing.Losses = 0;
			}
			spdlog::info("Migrated league {} from head-to-head fixtures to full-field races.", League.Name);
			MigratedAny = true;
		}

		const auto FindMigration = [&Migrations](std::uint64_t LeagueId)
		{
			return std::find_if(Migrations.begin(), Migrations.end(), [LeagueId](const LeagueFixtureMigration& Migration)
				{
					return Migration.LeagueId == LeagueId;
				});
		};
		const auto RemapFixtureReferences = [&FindMigration, &Migrations](
			std::vector<CareerRaceFixtureReference>& FixtureReferences)
		{
			std::vector<CareerRaceFixtureReference> RemappedReferences;
			RemappedReferences.reserve(FixtureReferences.size());
			for (CareerRaceFixtureReference Reference : FixtureReferences)
			{
				const auto Migration = FindMigration(Reference.LeagueId);
				if (Migration != Migrations.end())
				{
					if (Reference.FixtureIndex >= Migration->NewFixtureByOldIndex.size())
					{
						spdlog::error("Could not migrate a saved race reference for league {}.", Reference.LeagueId);
						continue;
					}
					Reference.FixtureIndex = Migration->NewFixtureByOldIndex[Reference.FixtureIndex];
					if (Reference.FixtureIndex == std::numeric_limits<std::size_t>::max())
					{
						spdlog::error("Could not map a saved race reference for league {}.", Reference.LeagueId);
						continue;
					}
				}
				const bool IsDuplicate = std::any_of(RemappedReferences.begin(), RemappedReferences.end(),
					[&Reference](const CareerRaceFixtureReference& Candidate)
					{
						return Candidate.LeagueId == Reference.LeagueId &&
							Candidate.FixtureIndex == Reference.FixtureIndex;
					});
				if (!IsDuplicate)
					RemappedReferences.push_back(Reference);
			}
			FixtureReferences = std::move(RemappedReferences);
		};
		if (MigratedAny)
		{
			for (const std::shared_ptr<CareerWorldRace>& WorldRace : Data.WorldRaces)
				if (WorldRace != nullptr)
					RemapFixtureReferences(WorldRace->LeagueFixtures);
			for (Game::Career::CareerPendingWorldLeagueRace& PendingRace : Data.PendingWorldLeagueRaces)
				RemapFixtureReferences(PendingRace.LeagueFixtures);
			CareerPendingLeagueRace& PendingPlayerRace = Data.PendingPlayerLeagueRace;
			const auto Migration = FindMigration(PendingPlayerRace.LeagueId);
			if (!PendingPlayerRace.IsPlayoff && Migration != Migrations.end() &&
				PendingPlayerRace.FixtureIndex < Migration->NewFixtureByOldIndex.size())
				PendingPlayerRace.FixtureIndex = Migration->NewFixtureByOldIndex[PendingPlayerRace.FixtureIndex];
			RemapFixtureReferences(PendingPlayerRace.LeagueFixtures);
		}
		return MigratedAny;
	}
	constexpr std::uint64_t CareerInitialWorldPresimMs = 7ull * 24 * 60 * 60 * 1000;
	constexpr std::uint64_t CareerInitialWorldSimulationStepMs = 1000;
	constexpr std::uint64_t CareerInitialWorldHistoryCatchupLimitMs = 30ull * 60 * 1000;
	constexpr double CareerTrainingFatigueThreshold = 0.35;
	constexpr double CareerTrainingInjuryThreshold = 0.15;
	constexpr double CareerTrainingFatigueRecovery = 0.35;
	constexpr double CareerTrainingInjuryRecovery = 0.10;
	constexpr double CareerPassiveFatigueRecovery = 0.02;
	constexpr double CareerPassiveInjuryRecovery = 0.01;

	constexpr std::array<unsigned int, 5> CareerTrackLengths{
		static_cast<unsigned int>(Game::Race::TrackLength::Standard100m),
		static_cast<unsigned int>(Game::Race::TrackLength::Standard200m),
		static_cast<unsigned int>(Game::Race::TrackLength::Standard250m),
		static_cast<unsigned int>(Game::Race::TrackLength::Standard500m),
		static_cast<unsigned int>(Game::Race::TrackLength::Standard1000m)
	};

	constexpr std::size_t InvalidPlotIndex = std::numeric_limits<std::size_t>::max();

	void GenerateWorldGeography(CareerData& Data)
	{
		if (!Data.Cities.empty() || !Data.Regions.empty() || !Data.Countries.empty())
			return;

		Data.Countries.push_back({ 1, 1, "Albion", 0, { 1, 2 } });
		Data.Regions.push_back({ 1, 1, 1, "Northshire", 0, 0.32f, 0.42f, { 1, 2, 3 } });
		Data.Regions.push_back({ 2, 1, 4, "Southshire", 0, 0.68f, 0.58f, { 4, 5, 6 } });
		Data.Cities = {
			{ 1, 1, "Northbridge", 1200000, 0.18f, 0.32f, true, {} },
			{ 2, 1, "Norchester", 540000, 0.52f, 0.62f, false, {} },
			{ 3, 1, "Westmere", 230000, 0.84f, 0.34f, false, {} },
			{ 4, 2, "Kingsport", 960000, 0.18f, 0.32f, true, {} },
			{ 5, 2, "Eastford", 410000, 0.52f, 0.62f, false, {} },
			{ 6, 2, "Fairhaven", 145000, 0.84f, 0.34f, false, {} }
		};
		Data.HomeCityId = Data.Cities.front().Id;

		for (CareerWorldRegion& Region : Data.Regions)
		{
			for (const std::uint64_t CityId : Region.CityIds)
				for (const CareerWorldCity& City : Data.Cities)
					if (City.Id == CityId)
						Region.Population += City.Population;
			Data.Countries.front().Population += Region.Population;
		}
	}

	void GenerateCityPlots(std::vector<Game::Career::CityPlot>& CityPlots,
		std::vector<unsigned int>& RaceTrackLengths, const std::string& CityName, std::mt19937& Random)
	{
		if (!CityPlots.empty())
			return;

		CityPlots.reserve(CareerCityGridWidth * CareerCityGridHeight);
		std::uniform_int_distribution<unsigned int> SpriteDistribution(0, CareerCitySpriteCount - 1);
		for (unsigned int GridY = 0; GridY < CareerCityGridHeight; ++GridY)
			for (unsigned int GridX = 0; GridX < CareerCityGridWidth; ++GridX)
			{
				Game::Career::CityPlot Plot;
				Plot.GridX = GridX;
				Plot.GridY = GridY;
				Plot.Name = "City scenery";
				Plot.SpriteIndex = SpriteDistribution(Random);
				CityPlots.push_back(std::move(Plot));
			}

		std::vector<std::size_t> AvailablePlotIndices(CityPlots.size());
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
			Game::Career::CityPlot& Plot = CityPlots[AvailablePlotIndices[BuildingIndex]];
			Plot.Type = BuildingTypes[BuildingIndex];
			switch (Plot.Type)
			{
			case Game::Career::CityPlotType::Gym:
				Plot.Name = CityName + " Municipal Gym";
				break;
			case Game::Career::CityPlotType::RaceTrack:
				Plot.Name = CityName + " Track " + std::to_string(++TrackNumber);
				Plot.TrackLength = CareerTrackLengths[TrackDistribution(Random)];
				RaceTrackLengths.push_back(Plot.TrackLength);
				break;
			case Game::Career::CityPlotType::Bookies:
				Plot.Name = CityName + " Bookmaker's";
				break;
			case Game::Career::CityPlotType::LicenseOffice:
				Plot.Name = CityName + " Racing License Office";
				break;
			case Game::Career::CityPlotType::BreedingFarm:
				Plot.Name = CityName + " Breeding Clinic";
				break;
			case Game::Career::CityPlotType::Stable:
				Plot.Name = CityName == "Northbridge" ? "Your Stable" : CityName + " Stable";
				break;
			default:
				break;
			}
		}

		for (std::size_t VacantIndex = BuildingTypes.size(); VacantIndex < BuildingTypes.size() + 5; ++VacantIndex)
		{
			Game::Career::CityPlot& Plot = CityPlots[AvailablePlotIndices[VacantIndex]];
			Plot.Type = Game::Career::CityPlotType::VacantLot;
			Plot.Name = "Vacant Lot";
		}
		Game::Career::CityPlot& TrackPlot = CityPlots[AvailablePlotIndices[BuildingTypes.size() + 5]];
		TrackPlot.Type = Game::Career::CityPlotType::RaceTrack;
		TrackPlot.Name = CityName + " Track " + std::to_string(++TrackNumber);
		TrackPlot.TrackLength = CareerTrackLengths[TrackDistribution(Random)];
		RaceTrackLengths.push_back(TrackPlot.TrackLength);
	}

	void GenerateCity(CareerData& Data, std::mt19937& Random)
	{
		GenerateWorldGeography(Data);
		const CareerWorldCity& HomeCity = Data.Cities.front();
		GenerateCityPlots(Data.CityPlots, Data.RaceTrackLengths, HomeCity.Name, Random);
		for (CareerWorldCity& City : Data.Cities)
			if (City.Id != Data.HomeCityId)
			{
				std::vector<unsigned int> CityTrackLengths;
				GenerateCityPlots(City.Plots, CityTrackLengths, City.Name, Random);
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

	CareerWorldCity* FindCity(CareerData& Data, std::uint64_t CityId)
	{
		const auto City = std::find_if(Data.Cities.begin(), Data.Cities.end(),
			[CityId](const CareerWorldCity& Candidate) { return Candidate.Id == CityId; });
		return City == Data.Cities.end() ? nullptr : &*City;
	}

	const CareerWorldCity* FindCity(const CareerData& Data, std::uint64_t CityId)
	{
		const auto City = std::find_if(Data.Cities.begin(), Data.Cities.end(),
			[CityId](const CareerWorldCity& Candidate) { return Candidate.Id == CityId; });
		return City == Data.Cities.end() ? nullptr : &*City;
	}

	CareerWorldRegion* FindRegion(CareerData& Data, std::uint64_t RegionId)
	{
		const auto Region = std::find_if(Data.Regions.begin(), Data.Regions.end(),
			[RegionId](const CareerWorldRegion& Candidate) { return Candidate.Id == RegionId; });
		return Region == Data.Regions.end() ? nullptr : &*Region;
	}

	const CareerWorldRegion* FindRegion(const CareerData& Data, std::uint64_t RegionId)
	{
		const auto Region = std::find_if(Data.Regions.begin(), Data.Regions.end(),
			[RegionId](const CareerWorldRegion& Candidate) { return Candidate.Id == RegionId; });
		return Region == Data.Regions.end() ? nullptr : &*Region;
	}

	CareerLeague* FindLeague(CareerData& Data, CareerCompetitionLevel Level,
		CareerLeagueDivision Division, std::uint64_t LocationId)
	{
		const auto League = std::find_if(Data.Leagues.begin(), Data.Leagues.end(),
			[Level, Division, LocationId](const CareerLeague& Candidate)
			{
				if (Candidate.IsCup || Candidate.Level != Level || Candidate.Division != Division)
					return false;
				if (Level == CareerCompetitionLevel::City)
					return Candidate.CityId == LocationId;
				if (Level == CareerCompetitionLevel::Regional)
					return Candidate.RegionId == LocationId;
				return true;
			});
		return League == Data.Leagues.end() ? nullptr : &*League;
	}

	const CareerLeague* FindLeague(const CareerData& Data, std::uint64_t LeagueId)
	{
		const auto League = std::find_if(Data.Leagues.begin(), Data.Leagues.end(),
			[LeagueId](const CareerLeague& Candidate) { return Candidate.Id == LeagueId; });
		return League == Data.Leagues.end() ? nullptr : &*League;
	}

	unsigned int GetLeagueFirstPrize(const CareerLeague& League)
	{
		unsigned int Prize = CareerRaceFirstPrize;
		switch (League.Division)
		{
		case Game::Career::CareerLeagueDivision::Division4:
			Prize *= 2;
			break;
		case Game::Career::CareerLeagueDivision::Division3:
			Prize *= 3;
			break;
		case Game::Career::CareerLeagueDivision::Division2:
			Prize *= 4;
			break;
		case Game::Career::CareerLeagueDivision::Division1:
			Prize *= 5;
			break;
		default:
			break;
		}
		if (League.Level == CareerCompetitionLevel::Regional)
			Prize *= 3;
		else if (League.Level == CareerCompetitionLevel::National)
			Prize *= 6;
		return Prize;
	}

	unsigned int GetLeagueRaceFirstPrize(const CareerData& Data,
		const std::vector<CareerRaceFixtureReference>& LeagueFixtures, std::uint64_t LeagueId, bool IsPlayoff)
	{
		if (IsPlayoff)
		{
			const CareerLeague* League = FindLeague(Data, LeagueId);
			return League == nullptr ? CareerRaceFirstPrize : GetLeagueFirstPrize(*League);
		}

		unsigned int FirstPlacePrize = CareerRaceFirstPrize;
		for (const CareerRaceFixtureReference& Reference : LeagueFixtures)
			if (const CareerLeague* League = FindLeague(Data, Reference.LeagueId))
				FirstPlacePrize = std::max(FirstPlacePrize, GetLeagueFirstPrize(*League));
		return FirstPlacePrize;
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

	void RecordLeagueRaceResults(CareerData& Data, const RaceResult& Result,
		const std::vector<CareerRaceFixtureReference>& FixtureReferences)
	{
		struct LeagueRaceEntrants
		{
			CareerLeague* League = nullptr;
			std::unordered_set<std::uint64_t> RacerIds;
		};
		std::vector<LeagueRaceEntrants> EntrantsByLeague;
		for (const CareerRaceFixtureReference& Reference : FixtureReferences)
		{
			CareerLeague* League = FindLeague(Data, Reference.LeagueId);
			if (League == nullptr || Reference.FixtureIndex >= League->Fixtures.size())
				continue;
			CareerLeagueFixture& Fixture = League->Fixtures[Reference.FixtureIndex];
			if (!IsFullFieldLeagueFixture(Fixture))
			{
				spdlog::error("Cannot record league race for {}: fixture was not migrated to a full field.",
					League->Name);
				Fixture.State = CareerFixtureState::Cancelled;
				continue;
			}
			Fixture.State = CareerFixtureState::Complete;
			Fixture.WinnerId = 0;

			auto LeagueEntrants = std::find_if(EntrantsByLeague.begin(), EntrantsByLeague.end(),
				[League](const LeagueRaceEntrants& Candidate) { return Candidate.League == League; });
			if (LeagueEntrants == EntrantsByLeague.end())
			{
				EntrantsByLeague.push_back({ League, {} });
				LeagueEntrants = EntrantsByLeague.end() - 1;
			}
			for (Racer* Runner : BuildCareerLeagueRaceEntrants(*League, Fixture.Fixture.Round))
				if (Runner != nullptr)
					LeagueEntrants->RacerIds.insert(GetRacerId(Runner));
			for (const Game::Race::RacerRaceResult& RacerResult : Result.RacerResults)
				if (RacerResult.Position == Game::Race::FirstPlaceRank && RacerResult.Racer != nullptr &&
					LeagueEntrants->RacerIds.contains(GetRacerId(RacerResult.Racer)))
					Fixture.WinnerId = GetRacerId(RacerResult.Racer);
		}

		for (const Game::Race::RacerRaceResult& RacerResult : Result.RacerResults)
		{
			if (RacerResult.Racer == nullptr)
				continue;

			const std::uint64_t RacerId = GetRacerId(RacerResult.Racer);
			for (const LeagueRaceEntrants& LeagueEntrants : EntrantsByLeague)
				if (LeagueEntrants.League != nullptr && LeagueEntrants.RacerIds.contains(RacerId))
					if (CareerLeagueStanding* Standing = FindStanding(*LeagueEntrants.League, RacerResult.Racer))
					{
						if (RacerResult.Position == Game::Race::FirstPlaceRank)
							++Standing->Wins;
						else
							++Standing->Losses;
						if (RacerResult.Position != 0 &&
							RacerResult.Position <= CareerWorldRaceEntrantCount)
							Standing->Points += static_cast<unsigned int>(CareerWorldRaceEntrantCount + 1 -
								RacerResult.Position);
					}
		}
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
		League.IsOpenLobby = false;
		if (League.Division == CareerLeagueDivision::Amateur)
		{
			auto PlayerEntrant = std::find(League.Entrants.begin(), League.Entrants.end(),
				Data.PlayerLeagueRacer);
			std::size_t RotationStart = 0;
			if (PlayerEntrant != League.Entrants.end())
			{
				std::rotate(League.Entrants.begin(), PlayerEntrant, PlayerEntrant + 1);
				RotationStart = 1;
			}
			const std::size_t RotationCount = League.Entrants.size() - RotationStart;
			const std::size_t RotationOffset =
				(static_cast<std::size_t>(League.Season % RotationCount) * 2) % RotationCount;
			std::rotate(League.Entrants.begin() + RotationStart,
				League.Entrants.begin() + RotationStart + RotationOffset, League.Entrants.end());
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

		League.Fixtures.clear();
		League.Standings.clear();
		League.Playoffs = SportRaceCore::PlayoffBracket{};
		const std::size_t HeatCount = League.Division == CareerLeagueDivision::Amateur
			? CareerAmateurLeagueHeatCount : CareerFullFieldRaceCount;
		League.Fixtures.reserve(HeatCount);
		League.Standings.reserve(League.Entrants.size());
		for (Racer* Runner : League.Entrants)
			League.Standings.push_back({ Runner, 0, 0, 0, 0 });

		const std::uint64_t SeasonStartMs = Data.WorldClock.NowMs() + CareerLeagueFirstFixtureDelayMs;
		for (std::size_t Index = 0; Index < HeatCount; ++Index)
		{
			CareerLeagueFixture WorldFixture;
			WorldFixture.Fixture.Round = static_cast<std::uint32_t>(Index);
			if (BuildCareerLeagueRaceEntrants(League, Index).size() != CareerWorldRaceEntrantCount)
			{
				spdlog::error("Could not build full-field races for league {}.", League.Name);
				League.State = CareerLeagueState::Filling;
				League.Fixtures.clear();
				League.Standings.clear();
				return;
			}
			WorldFixture.ScheduledAtMs = SeasonStartMs +
				static_cast<std::uint64_t>(Index) * CareerLeagueRoundIntervalMs;
			WorldFixture.TrackPlotIndex = Tracks[(Index + League.Id) % Tracks.size()];
			League.Fixtures.push_back(WorldFixture);
		}
		++League.Season;
		League.State = CareerLeagueState::RegularSeason;
}

	void AddLeague(CareerData& Data, const std::string& Name, CareerLeagueDivision Division,
		const std::vector<Racer*>& Entrants, std::uint64_t PromotionTargetLeagueId = 0, bool IsOpenLobby = false,
		CareerCompetitionLevel Level = CareerCompetitionLevel::City, std::uint64_t CityId = 0,
		std::uint64_t RegionId = 0, std::size_t PromotionSlots = 2,
		std::size_t IncomingPromotionLimit = 2)
	{
		CareerLeague League;
		League.Id = Data.NextLeagueId++;
		League.CityId = CityId;
		League.RegionId = RegionId;
		League.Name = Name;
		League.Level = Level;
		League.Division = Division;
		League.PromotionTargetLeagueId = PromotionTargetLeagueId;
		League.PromotionSlots = PromotionSlots;
		League.IncomingPromotionLimit = IncomingPromotionLimit;
		League.IsOpenLobby = IsOpenLobby;
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

	bool IsPendingPlayerLeagueRaceEntrant(const CareerData& Data, const Racer* Runner)
	{
		const CareerPendingLeagueRace& PendingRace = Data.PendingPlayerLeagueRace;
		return (PendingRace.Preparing || PendingRace.Active) &&
			std::find(PendingRace.Entrants.begin(), PendingRace.Entrants.end(), Runner) != PendingRace.Entrants.end();
	}

	bool IsPendingWorldLeagueRaceEntrant(const CareerData& Data, const Racer* Runner)
	{
		return std::any_of(Data.PendingWorldLeagueRaces.begin(), Data.PendingWorldLeagueRaces.end(),
			[Runner](const Game::Career::CareerPendingWorldLeagueRace& PendingRace)
			{
				return std::find(PendingRace.Entrants.begin(), PendingRace.Entrants.end(), Runner) !=
					PendingRace.Entrants.end();
			});
	}

	std::shared_ptr<CareerWorldRace> StartWorldRace(CareerData& Data, std::size_t TrackPlotIndex,
		const std::vector<Racer*>& Entrants, const std::string& EventName,
		const std::vector<CareerRaceFixtureReference>& LeagueFixtures, std::uint64_t LeagueId = 0,
		bool IsPlayoff = false, std::size_t PlayoffRoundIndex = 0, std::size_t PlayoffMatchIndex = 0)
	{
		const std::size_t ExpectedEntrantCount = IsPlayoff
			? CareerPlayoffRaceEntrantCount : CareerWorldRaceEntrantCount;
		const unsigned int EntryFee = LeagueFixtures.empty() && !IsPlayoff
			? static_cast<unsigned int>(CareerRaceEntryFee) : 0u;
		const unsigned int FirstPlacePrize = GetLeagueRaceFirstPrize(Data, LeagueFixtures, LeagueId, IsPlayoff);
		if (TrackPlotIndex >= Data.CityPlots.size() || Entrants.size() != ExpectedEntrantCount ||
			(!LeagueFixtures.empty() && LeagueFixtures.size() * 2 > Entrants.size()))
		{
			spdlog::error("Cannot start world race '{}' with {} entrants and {} league fixtures on track {} "
				"(expected {} entrants; league fixtures cannot exceed the field).", EventName, Entrants.size(), LeagueFixtures.size(),
				TrackPlotIndex, ExpectedEntrantCount);
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
				State->Cash < static_cast<int>(EntryFee) ||
				State->RaceCooldownEndsAtRealMs > Data.RealTimeMs)
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
		CurrentRace.Financials = Game::Race::RaceFinancials(FirstPlacePrize, EntryFee);

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
			State->Cash -= static_cast<int>(CurrentRace.Financials.EntranceFee);
			State->Activity = CareerRacerActivity::InRace;
			State->ActiveRaceId = WorldRace->Id;
			State->RaceCooldownEndsAtRealMs = 0;
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

	CareerRacerState MakeRacerWorldState(CareerData& Data, Racer* Runner, std::uint64_t CityId = 0)
	{
		CareerRacerState State;
		State.Runner = Runner;
		State.CityId = CityId;
		if (State.CityId == 0 && !Data.Cities.empty())
		{
			std::vector<double> PopulationWeights;
			PopulationWeights.reserve(Data.Cities.size());
			for (const CareerWorldCity& City : Data.Cities)
				PopulationWeights.push_back(static_cast<double>(City.Population));
			std::discrete_distribution<std::size_t> CityDistribution(PopulationWeights.begin(),
				PopulationWeights.end());
			State.CityId = Data.Cities[CityDistribution(Data.WorldRandom)].Id;
		}
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
		const std::uint64_t ReplacementCityId = RetiredState->CityId;
		RetiredState->Activity = CareerRacerActivity::Retired;
		RetiredState->RetiredAtMs = Data.WorldClock.NowMs();
		RetiredState->ActiveRaceId = 0;
		if (CareerLeague* League = FindLeague(Data, RetiredState->LeagueId);
			League != nullptr && League->State == CareerLeagueState::Filling)
		{
			League->Entrants.erase(std::remove(League->Entrants.begin(), League->Entrants.end(), Runner),
				League->Entrants.end());
			RetiredState->LeagueId = 0;
		}

		Racer* Replacement = Data.Racers.Make();
		CareerRacerState ReplacementState = MakeRacerWorldState(Data, Replacement);
		if (ReplacementCityId != 0)
			ReplacementState.CityId = ReplacementCityId;
		Data.RacerStates.push_back(std::move(ReplacementState));
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
			std::uint64_t HistoricalRaceId = 0;
			std::string HistoryError;
			if (!RecordCareerRaceHistory(Data, CurrentRace, WorldRace.EventName, Data.WorldClock.NowMs(),
				HistoricalRaceId, HistoryError))
				spdlog::error("Could not archive completed world race {}: {}", WorldRace.Id, HistoryError);
			else if (CareerHistoricalRace* HistoricalRace = FindCareerHistoricalRace(Data, HistoricalRaceId))
			{
				HistoricalRace->WorldRaceId = WorldRace.Id;
				HistoricalRace->LeagueId = WorldRace.LeagueId;
				HistoricalRace->IsPlayoff = WorldRace.IsPlayoff;
				HistoricalRace->PlayoffRoundIndex = WorldRace.PlayoffRoundIndex;
				HistoricalRace->PlayoffMatchIndex = WorldRace.PlayoffMatchIndex;
			}
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
				HistoryEntry.HistoricalRaceId = HistoricalRaceId;
				RacerResult.Racer->RecordCareerResult(RacerResult.Position, HistoryEntry.FinishTimeMs, HistoryEntry);
				if (CareerRacerState* State = FindRacerState(Data, RacerResult.Racer))
				{
					State->Cash += static_cast<int>(GetPrizeForPosition(CurrentRace, RacerResult.Position));
					if (State->Cash <= 0)
						BankruptRacers.push_back(RacerResult.Racer);
				}
			}

			if (HistoricalRaceId != 0)
				for (const CareerRaceFixtureReference& Reference : WorldRace.LeagueFixtures)
					if (CareerLeague* League = FindLeague(Data, Reference.LeagueId);
						League != nullptr && Reference.FixtureIndex < League->Fixtures.size())
						League->Fixtures[Reference.FixtureIndex].HistoricalRaceId = HistoricalRaceId;

			RecordLeagueRaceResults(Data, Result, WorldRace.LeagueFixtures);

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
						else
							League->Playoffs.Rounds[WorldRace.PlayoffRoundIndex].Matches[
								WorldRace.PlayoffMatchIndex].HistoricalRaceId = HistoricalRaceId;
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
					State->Cash += static_cast<int>(CurrentRace.Financials.EntranceFee);
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
				State->RaceCooldownEndsAtRealMs = Data.RealTimeMs + CareerPostRaceRestMs;
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

	void FinishTravel(CareerData& Data, CareerRacerState& State, bool StartTraining = true)
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

		if (StartTraining && State.LocationPlotIndex == FindPlotIndex(Data, Game::Career::CityPlotType::Gym))
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

	std::unordered_set<std::uint64_t> GetNearLeagueFixtureRacerIds(const CareerData& Data,
		std::uint64_t ReservationWindowMs);

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

	bool IsPendingWorldPlayoffRaceEntrant(const CareerData& Data, const Racer* Runner)
	{
		return std::any_of(Data.PendingWorldPlayoffRaces.begin(), Data.PendingWorldPlayoffRaces.end(),
			[Runner](const CareerPendingWorldPlayoffRace& PendingRace)
			{
				return std::find(PendingRace.Entrants.begin(), PendingRace.Entrants.end(), Runner) !=
					PendingRace.Entrants.end();
			});
	}

	void UpdateAgentActivities(CareerData& Data)
	{
		std::vector<Racer*> BankruptRacers(Data.RacerStates.size(), nullptr);
		const std::uint64_t Now = Data.RealTimeMs;
		const std::size_t GymIndex = FindPlotIndex(Data, Game::Career::CityPlotType::Gym);
		const std::unordered_set<std::uint64_t> NearLeagueFixtureRacerIds =
			GetNearLeagueFixtureRacerIds(Data, CareerLeagueRoundIntervalMs);
		const std::size_t LicenseOfficeIndex = FindPlotIndex(Data, Game::Career::CityPlotType::LicenseOffice);
		std::transform(std::execution::par, Data.RacerStates.begin(), Data.RacerStates.end(),
			BankruptRacers.begin(), [&Data, Now, GymIndex, LicenseOfficeIndex,
				&NearLeagueFixtureRacerIds](CareerRacerState& State) -> Racer*
		{
			if (State.Runner == nullptr || State.Activity == CareerRacerActivity::Retired)
				return nullptr;
			if (State.Cash <= 0 && State.Activity != CareerRacerActivity::InRace)
				return State.Runner;

			const bool HasUpcomingLeagueFixture = NearLeagueFixtureRacerIds.contains(GetRacerId(State.Runner));
			if (State.Activity == CareerRacerActivity::Travelling && State.ActionEndsAtRealMs <= Now)
				FinishTravel(Data, State, !HasUpcomingLeagueFixture);
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

			if (NeedsTraining(*State.Runner) && GymIndex != InvalidPlotIndex && !HasUpcomingLeagueFixture)
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

	bool IsPendingOpenRaceEntrant(const CareerData& Data, const Racer* Runner)
	{
		return std::any_of(Data.PendingOpenRaces.begin(), Data.PendingOpenRaces.end(),
			[Runner](const CareerPendingOpenRace& PendingRace)
			{
				return std::find(PendingRace.Entrants.begin(), PendingRace.Entrants.end(), Runner) !=
					PendingRace.Entrants.end();
			});
	}

	bool IsPendingWorldRaceEntrant(const CareerData& Data, const Racer* Runner)
	{
		return IsPendingOpenRaceEntrant(Data, Runner) || IsPendingWorldLeagueRaceEntrant(Data, Runner) ||
			IsPendingWorldPlayoffRaceEntrant(Data, Runner) || IsPendingPlayerLeagueRaceEntrant(Data, Runner);
	}

	std::unordered_set<std::uint64_t> GetPendingWorldRaceRacerIds(const CareerData& Data)
	{
		std::unordered_set<std::uint64_t> RacerIds;
		const auto AddEntrants = [&RacerIds](const std::vector<Racer*>& Entrants)
		{
			for (const Racer* Runner : Entrants)
				if (Runner != nullptr)
					RacerIds.insert(GetRacerId(Runner));
		};
		for (const CareerPendingOpenRace& PendingRace : Data.PendingOpenRaces)
			AddEntrants(PendingRace.Entrants);
		for (const Game::Career::CareerPendingWorldLeagueRace& PendingRace : Data.PendingWorldLeagueRaces)
			AddEntrants(PendingRace.Entrants);
		for (const CareerPendingWorldPlayoffRace& PendingRace : Data.PendingWorldPlayoffRaces)
			AddEntrants(PendingRace.Entrants);
		const CareerPendingLeagueRace& PendingPlayerRace = Data.PendingPlayerLeagueRace;
		if (PendingPlayerRace.Preparing || PendingPlayerRace.Active)
			AddEntrants(PendingPlayerRace.Entrants);
		return RacerIds;
	}

	void FillLeagueVacancies(CareerData& Data)
	{
		for (CareerLeague& League : Data.Leagues)
		{
			if (League.IsCup || League.IsAmateurCup || League.State != CareerLeagueState::Filling)
				continue;
			const std::size_t EntrantLimit = League.IsOpenLobby && Data.PlayerLeagueRacer == nullptr
				? CareerLeagueSize - 1 : CareerLeagueSize;
			while (League.Entrants.size() < EntrantLimit)
			{
				CareerRacerState* Candidate = nullptr;
				for (CareerRacerState& State : Data.RacerStates)
				{
					const bool IsAvailable = State.Activity == CareerRacerActivity::InBuilding ||
						State.Activity == CareerRacerActivity::WaitingForLeague;
					if (State.Runner == nullptr || State.LeagueId != 0 || !IsAvailable ||
						IsPendingWorldRaceEntrant(Data, State.Runner) ||
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
	}

	void UpdateLicenseStatus(CareerData& Data)
	{
		for (CareerRacerState& State : Data.RacerStates)
			if (State.Runner != nullptr && State.Activity != CareerRacerActivity::Retired &&
				State.HasRacingLicense && State.LeagueId == 0 &&
				(State.Activity == CareerRacerActivity::InBuilding ||
					State.Activity == CareerRacerActivity::WaitingForLeague))
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
			if (State->RaceCooldownEndsAtRealMs > Data.RealTimeMs)
			{
				IsReady = false;
				RetryAtWorldMs = std::max(RetryAtWorldMs,
					GetWorldTimeAtRealDeadline(Data, State->RaceCooldownEndsAtRealMs));
				continue;
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

	void TryStartPendingWorldLeagueRaces(CareerData& Data)
	{
		for (auto Pending = Data.PendingWorldLeagueRaces.begin(); Pending != Data.PendingWorldLeagueRaces.end();)
		{
			const bool ReferencesAreScheduled = std::all_of(Pending->LeagueFixtures.begin(),
				Pending->LeagueFixtures.end(), [&Data](const CareerRaceFixtureReference& Reference)
				{
					const CareerLeague* League = FindLeague(Data, Reference.LeagueId);
					return League != nullptr && Reference.FixtureIndex < League->Fixtures.size() &&
						League->Fixtures[Reference.FixtureIndex].State == CareerFixtureState::Scheduled;
				});
			const bool EntrantsAreActive = std::all_of(Pending->Entrants.begin(), Pending->Entrants.end(),
				[&Data](Racer* Runner)
				{
					const CareerRacerState* State = FindRacerState(Data, Runner);
					return Runner != nullptr && State != nullptr && State->Activity != CareerRacerActivity::Retired;
				});
			if (!ReferencesAreScheduled || !EntrantsAreActive)
			{
				Pending = Data.PendingWorldLeagueRaces.erase(Pending);
				continue;
			}

			bool Ready = false;
			std::uint64_t RetryAtWorldMs = 0;
			RecordRaceStartTravel(Data, Pending->Entrants, Pending->TrackPlotIndex, RetryAtWorldMs, Ready);
			if (!Ready)
			{
				++Pending;
				continue;
			}

			if (StartWorldRace(Data, Pending->TrackPlotIndex, Pending->Entrants, Pending->EventName,
				Pending->LeagueFixtures, Pending->LeagueId) == nullptr)
				for (const CareerRaceFixtureReference& Reference : Pending->LeagueFixtures)
					if (CareerLeague* League = FindLeague(Data, Reference.LeagueId);
						League != nullptr && Reference.FixtureIndex < League->Fixtures.size())
					{
						CareerLeagueFixture& Fixture = League->Fixtures[Reference.FixtureIndex];
						if (Fixture.State == CareerFixtureState::Scheduled)
							Fixture.ScheduledAtMs = GetWorldTimeAfterRealDelay(Data, CareerRaceRetryDelayMs);
					}
			Pending = Data.PendingWorldLeagueRaces.erase(Pending);
		}
	}

	bool IsOwnedRacer(const CareerData& Data, const Racer* Runner)
	{
		return std::find(Data.OwnedRacers.begin(), Data.OwnedRacers.end(), Runner) != Data.OwnedRacers.end();
	}

	std::unordered_set<std::uint64_t> GetNearLeagueFixtureRacerIds(const CareerData& Data,
		std::uint64_t ReservationWindowMs = CareerLeagueRoundIntervalMs)
	{
		std::unordered_set<std::uint64_t> RacerIds = GetPendingWorldRaceRacerIds(Data);

		const std::uint64_t Now = Data.WorldClock.NowMs();
		const std::uint64_t WindowEnd = ReservationWindowMs >
			std::numeric_limits<std::uint64_t>::max() - Now
			? std::numeric_limits<std::uint64_t>::max() : Now + ReservationWindowMs;
		for (const CareerLeague& League : Data.Leagues)
			if (League.State == CareerLeagueState::RegularSeason)
				for (const CareerLeagueFixture& Fixture : League.Fixtures)
					if (Fixture.State == CareerFixtureState::Scheduled && Fixture.ScheduledAtMs <= WindowEnd)
					{
						if (IsFullFieldLeagueFixture(Fixture))
						{
							for (const Racer* Runner : BuildCareerLeagueRaceEntrants(League,
								Fixture.Fixture.Round))
								if (Runner != nullptr)
									RacerIds.insert(GetRacerId(Runner));
						}
					}
		return RacerIds;
	}

	std::unordered_set<std::uint64_t> GetPendingPlayoffRacerIds(const CareerData& Data)
	{
		std::unordered_set<std::uint64_t> RacerIds;
		for (const CareerLeague& League : Data.Leagues)
			if (League.State == CareerLeagueState::Playoffs)
				for (const SportRaceCore::PlayoffRound& Round : League.Playoffs.Rounds)
					for (const SportRaceCore::PlayoffMatch& Match : Round.Matches)
						if (!Match.IsComplete)
						{
							if (Match.CompetitorAId.has_value() && *Match.CompetitorAId != 0)
								RacerIds.insert(*Match.CompetitorAId);
							if (Match.CompetitorBId.has_value() && *Match.CompetitorBId != 0)
								RacerIds.insert(*Match.CompetitorBId);
						}
		return RacerIds;
	}

	bool FillWorldRaceToSix(CareerData& Data, std::vector<Racer*>& Entrants,
		bool AllowNearFixtureGuests = false)
	{
		if (Entrants.size() >= CareerWorldRaceEntrantCount)
			return Entrants.size() == CareerWorldRaceEntrantCount;

		const std::unordered_set<Racer*> SelectedEntrants(Entrants.begin(), Entrants.end());
		const std::unordered_set<std::uint64_t> NearLeagueFixtureRacerIds =
			GetNearLeagueFixtureRacerIds(Data);
		const std::unordered_set<std::uint64_t> PendingWorldRaceRacerIds = GetPendingWorldRaceRacerIds(Data);
		const std::unordered_set<std::uint64_t> PendingPlayoffRacerIds = GetPendingPlayoffRacerIds(Data);
		std::vector<Racer*> GuestEntrants;
		std::vector<Racer*> NearFixtureGuestEntrants;
		for (const CareerRacerState& State : Data.RacerStates)
		{
			if (State.Runner == nullptr)
				continue;
			const std::uint64_t RacerId = GetRacerId(State.Runner);
			const bool IsAvailable = State.Activity == CareerRacerActivity::InBuilding ||
				State.Activity == CareerRacerActivity::Resting ||
				State.Activity == CareerRacerActivity::WaitingForLeague;
			if (SelectedEntrants.contains(State.Runner) ||
				IsOwnedRacer(Data, State.Runner) || !IsAvailable || PendingWorldRaceRacerIds.contains(RacerId) ||
				PendingPlayoffRacerIds.contains(RacerId) ||
				State.RaceCooldownEndsAtRealMs > Data.RealTimeMs)
				continue;
			if (NearLeagueFixtureRacerIds.contains(RacerId))
				NearFixtureGuestEntrants.push_back(State.Runner);
			else
				GuestEntrants.push_back(State.Runner);
		}

		const std::size_t Needed = CareerWorldRaceEntrantCount - Entrants.size();
		if (GuestEntrants.size() < Needed)
		{
			const std::size_t AdditionalNeeded = Needed - GuestEntrants.size();
			if (NearFixtureGuestEntrants.size() < AdditionalNeeded && !AllowNearFixtureGuests)
				return false;
			std::shuffle(NearFixtureGuestEntrants.begin(), NearFixtureGuestEntrants.end(), Data.WorldRandom);
			const std::size_t NearFixtureGuestCount = std::min(AdditionalNeeded, NearFixtureGuestEntrants.size());
			GuestEntrants.insert(GuestEntrants.end(), NearFixtureGuestEntrants.begin(),
				NearFixtureGuestEntrants.begin() + NearFixtureGuestCount);
			if (GuestEntrants.size() < Needed)
				return false;
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
		TryStartPendingWorldLeagueRaces(Data);

		struct DueFixture
		{
			CareerRaceFixtureReference Reference;
			std::uint64_t ScheduledAtMs = 0;
			std::size_t TrackPlotIndex = 0;
			std::vector<Racer*> Entrants;
		};

		const std::uint64_t Now = Data.WorldClock.NowMs();
		std::vector<DueFixture> DueFixtures;
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

			for (std::size_t FixtureIndex = 0; FixtureIndex < League.Fixtures.size(); ++FixtureIndex)
			{
				CareerLeagueFixture& Fixture = League.Fixtures[FixtureIndex];
				if (Fixture.State != CareerFixtureState::Scheduled ||
					Fixture.Fixture.Round != EarliestUnresolvedRound || Fixture.ScheduledAtMs > Now ||
					!IsFullFieldLeagueFixture(Fixture))
					continue;

				std::vector<Racer*> Entrants = BuildCareerLeagueRaceEntrants(League, Fixture.Fixture.Round);
				if (Entrants.size() != CareerWorldRaceEntrantCount ||
					std::find(Entrants.begin(), Entrants.end(), Data.PlayerLeagueRacer) != Entrants.end())
					continue;

				Entrants.erase(std::remove_if(Entrants.begin(), Entrants.end(), [&Data](Racer* Runner)
					{
						const CareerRacerState* State = FindRacerState(Data, Runner);
						return Runner == nullptr || State == nullptr || State->Activity == CareerRacerActivity::Retired;
					}), Entrants.end());
				const bool HasBusyEntrant = std::any_of(Entrants.begin(), Entrants.end(), [&Data](Racer* Runner)
					{
						const CareerRacerState* State = FindRacerState(Data, Runner);
						return State == nullptr || IsPendingWorldRaceEntrant(Data, Runner) ||
							State->Activity == CareerRacerActivity::InRace ||
							State->Activity == CareerRacerActivity::Training;
					});
				if (HasBusyEntrant || !FillWorldRaceToSix(Data, Entrants, true))
					continue;

				DueFixtures.push_back({ { League.Id, FixtureIndex }, Fixture.ScheduledAtMs,
					Fixture.TrackPlotIndex, std::move(Entrants) });
			}
		}

		std::stable_sort(DueFixtures.begin(), DueFixtures.end(), [](const DueFixture& Left, const DueFixture& Right)
			{
				if (Left.TrackPlotIndex != Right.TrackPlotIndex)
					return Left.TrackPlotIndex < Right.TrackPlotIndex;
				if (Left.ScheduledAtMs != Right.ScheduledAtMs)
					return Left.ScheduledAtMs < Right.ScheduledAtMs;
				if (Left.Reference.LeagueId != Right.Reference.LeagueId)
					return Left.Reference.LeagueId < Right.Reference.LeagueId;
				return Left.Reference.FixtureIndex < Right.Reference.FixtureIndex;
			});

		auto DeferFixtures = [&Data](const std::vector<CareerRaceFixtureReference>& References,
			std::uint64_t RetryAtWorldMs)
		{
			const std::uint64_t Now = Data.WorldClock.NowMs();
			const std::uint64_t NotBefore = Now == std::numeric_limits<std::uint64_t>::max() ? Now : Now + 1;
			const std::uint64_t ScheduledAtMs = std::max(RetryAtWorldMs, NotBefore);
			for (const CareerRaceFixtureReference& Reference : References)
				if (CareerLeague* League = FindLeague(Data, Reference.LeagueId);
					League != nullptr && Reference.FixtureIndex < League->Fixtures.size())
				{
					CareerLeagueFixture& Fixture = League->Fixtures[Reference.FixtureIndex];
					if (Fixture.State == CareerFixtureState::Scheduled)
						Fixture.ScheduledAtMs = ScheduledAtMs;
				}
		};

		auto StartFullFieldRace = [&Data, &DeferFixtures](const DueFixture& Candidate)
		{
			CareerLeague* League = FindLeague(Data, Candidate.Reference.LeagueId);
			if (League == nullptr || Candidate.Reference.FixtureIndex >= League->Fixtures.size())
				return;
			CareerLeagueFixture& Fixture = League->Fixtures[Candidate.Reference.FixtureIndex];
			if (Fixture.State != CareerFixtureState::Scheduled || !IsFullFieldLeagueFixture(Fixture) ||
				Candidate.Entrants.size() != CareerWorldRaceEntrantCount)
				return;

			const std::vector<CareerRaceFixtureReference> References{ Candidate.Reference };
			for (Racer* Runner : Candidate.Entrants)
				if (Runner == nullptr || IsPendingWorldRaceEntrant(Data, Runner))
				{
					DeferFixtures(References, GetWorldTimeAfterRealDelay(Data, CareerRaceRetryDelayMs));
					return;
				}
				else if (const CareerRacerState* State = FindRacerState(Data, Runner);
					State == nullptr || State->Activity == CareerRacerActivity::Retired ||
					State->Activity == CareerRacerActivity::InRace ||
					State->Activity == CareerRacerActivity::Training)
				{
					DeferFixtures(References, GetWorldTimeAfterRealDelay(Data, CareerRaceRetryDelayMs));
					return;
				}
			const std::string EventName = League->Name + " - heat " +
				std::to_string(Fixture.Fixture.Round + 1);
			bool Ready = false;
			std::uint64_t RetryAtWorldMs = 0;
			RecordRaceStartTravel(Data, Candidate.Entrants, Candidate.TrackPlotIndex, RetryAtWorldMs, Ready);
			if (!Ready)
			{
				Data.PendingWorldLeagueRaces.push_back({ Candidate.TrackPlotIndex, League->Id, EventName,
					Candidate.Entrants, References });
				return;
			}

			if (StartWorldRace(Data, Candidate.TrackPlotIndex, Candidate.Entrants, EventName, References,
				League->Id) == nullptr)
				DeferFixtures(References, GetWorldTimeAfterRealDelay(Data, CareerRaceRetryDelayMs));
		};

		for (const DueFixture& Candidate : DueFixtures)
			StartFullFieldRace(Candidate);
	}

	void TryStartPendingWorldPlayoffRaces(CareerData& Data)
	{
		for (auto Pending = Data.PendingWorldPlayoffRaces.begin(); Pending != Data.PendingWorldPlayoffRaces.end();)
		{
			CareerLeague* League = FindLeague(Data, Pending->LeagueId);
			if (League == nullptr || League->State != CareerLeagueState::Playoffs ||
				Pending->PlayoffRoundIndex >= League->Playoffs.Rounds.size() ||
				Pending->PlayoffMatchIndex >= League->Playoffs.Rounds[Pending->PlayoffRoundIndex].Matches.size() ||
				Pending->Entrants.size() != CareerPlayoffRaceEntrantCount)
			{
				Pending = Data.PendingWorldPlayoffRaces.erase(Pending);
				continue;
			}

			const SportRaceCore::PlayoffMatch& Match =
				League->Playoffs.Rounds[Pending->PlayoffRoundIndex].Matches[Pending->PlayoffMatchIndex];
			if (Match.IsComplete || !Match.CompetitorAId.has_value() || !Match.CompetitorBId.has_value() ||
				*Match.CompetitorAId != GetRacerId(Pending->Entrants[0]) ||
				*Match.CompetitorBId != GetRacerId(Pending->Entrants[1]) ||
				IsRunnerInRace(Data, League->Id, Pending->PlayoffRoundIndex, Pending->PlayoffMatchIndex))
			{
				Pending = Data.PendingWorldPlayoffRaces.erase(Pending);
				continue;
			}

			const CareerRacerState* FirstState = FindRacerState(Data, Pending->Entrants[0]);
			const CareerRacerState* SecondState = FindRacerState(Data, Pending->Entrants[1]);
			if (FirstState == nullptr || SecondState == nullptr ||
				FirstState->Activity == CareerRacerActivity::Retired ||
				SecondState->Activity == CareerRacerActivity::Retired)
			{
				Pending = Data.PendingWorldPlayoffRaces.erase(Pending);
				continue;
			}

			bool Ready = false;
			std::uint64_t RetryAtWorldMs = 0;
			RecordRaceStartTravel(Data, Pending->Entrants, Pending->TrackPlotIndex, RetryAtWorldMs, Ready);
			if (!Ready)
			{
				++Pending;
				continue;
			}

			if (StartWorldRace(Data, Pending->TrackPlotIndex, Pending->Entrants, Pending->EventName, {},
				League->Id, true, Pending->PlayoffRoundIndex, Pending->PlayoffMatchIndex) == nullptr)
			{
				spdlog::error("Could not start pending playoff match {} in {}.",
					Pending->PlayoffMatchIndex + 1, League->Name);
				++Pending;
				continue;
			}
			Pending = Data.PendingWorldPlayoffRaces.erase(Pending);
		}
	}

	void TryStartPlayoffRaces(CareerData& Data)
	{
		const std::vector<std::size_t> Tracks = GetTrackPlotIndices(Data);
		if (Tracks.empty())
			return;
		TryStartPendingWorldPlayoffRaces(Data);
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
						(FirstState != nullptr && FirstState->Activity != CareerRacerActivity::Retired);
					const bool SecondCanRace = Second == Data.PlayerLeagueRacer ||
						(SecondState != nullptr && SecondState->Activity != CareerRacerActivity::Retired);
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
					if (IsPendingWorldRaceEntrant(Data, First) || IsPendingWorldRaceEntrant(Data, Second))
						continue;

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
					const std::string EventName = League.Name + " playoff round " +
						std::to_string(RoundIndex + 1);
					bool Ready = false;
					std::uint64_t RetryAtWorldMs = 0;
					RecordRaceStartTravel(Data, Entrants, TrackIndex, RetryAtWorldMs, Ready);
					if (!Ready)
					{
						Data.PendingWorldPlayoffRaces.push_back({ TrackIndex, League.Id, RoundIndex, MatchIndex,
							EventName, std::move(Entrants) });
						continue;
					}
					StartWorldRace(Data, TrackIndex, Entrants, EventName, {}, League.Id, true,
						RoundIndex, MatchIndex);
			}
		}
	}

	void TryStartOpenRaces(CareerData& Data)
	{
		const std::uint64_t Now = Data.WorldClock.NowMs();
		if (Now < Data.NextOpenRaceAtMs)
			return;
		Data.NextOpenRaceAtMs = Now + CareerOpenRaceIntervalMs;
		for (auto PendingRace = Data.PendingOpenRaces.begin(); PendingRace != Data.PendingOpenRaces.end();)
		{
			if (PendingRace->TrackPlotIndex >= Data.CityPlots.size() ||
				Data.CityPlots[PendingRace->TrackPlotIndex].Type != Game::Career::CityPlotType::RaceTrack)
			{
				spdlog::error("Discarding queued open city race with invalid track {}.",
					PendingRace->TrackPlotIndex);
				PendingRace = Data.PendingOpenRaces.erase(PendingRace);
				continue;
			}
			const bool HasUnavailableEntrant = std::any_of(PendingRace->Entrants.begin(),
				PendingRace->Entrants.end(), [&Data](Racer* Runner)
				{
					const CareerRacerState* State = FindRacerState(Data, Runner);
					return Runner == nullptr || State == nullptr || State->Activity == CareerRacerActivity::Retired ||
						State->Cash < CareerRaceEntryFee;
				});
			if (HasUnavailableEntrant)
			{
				spdlog::warn("Cancelling queued open city race at track {} because an entrant is unavailable.",
					PendingRace->TrackPlotIndex);
				PendingRace = Data.PendingOpenRaces.erase(PendingRace);
				continue;
			}

			bool Ready = false;
			std::uint64_t RetryAtWorldMs = 0;
			RecordRaceStartTravel(Data, PendingRace->Entrants, PendingRace->TrackPlotIndex,
				RetryAtWorldMs, Ready);
			if (!Ready)
			{
				++PendingRace;
				continue;
			}

			if (StartWorldRace(Data, PendingRace->TrackPlotIndex, PendingRace->Entrants,
					"Open city race", {}) == nullptr)
					spdlog::error("Could not start queued open city race at track {}.",
					PendingRace->TrackPlotIndex);
			PendingRace = Data.PendingOpenRaces.erase(PendingRace);
		}

		const std::vector<std::size_t> Tracks = GetTrackPlotIndices(Data);
		if (Tracks.empty())
			return;
		const std::size_t StartingTrackOffset = Data.NextOpenRaceTrackIndex % Tracks.size();
		const std::uint64_t FixtureReservationWindowMs = Data.InitialWorldMonthSimulationInProgress
			? CareerOpenRaceIntervalMs : CareerLeagueRoundIntervalMs;
		const std::unordered_set<std::uint64_t> NearLeagueFixtureRacerIds =
			GetNearLeagueFixtureRacerIds(Data, FixtureReservationWindowMs);
		const std::unordered_set<std::uint64_t> PendingPlayoffRacerIds = GetPendingPlayoffRacerIds(Data);
		std::unordered_set<const Racer*> AttemptedEntrants;
		for (std::size_t TrackOffset = 0; TrackOffset < Tracks.size(); ++TrackOffset)
		{
			const std::size_t TrackIndex = Tracks[(StartingTrackOffset + TrackOffset) % Tracks.size()];
			std::vector<Racer*> Candidates;
			std::vector<Racer*> LeagueCandidates;
			for (const CareerRacerState& State : Data.RacerStates)
			{
				if (State.Runner == nullptr)
					continue;
				const std::uint64_t RacerId = GetRacerId(State.Runner);
				const bool IsAvailable = State.Activity == CareerRacerActivity::InBuilding ||
					State.Activity == CareerRacerActivity::Resting ||
					State.Activity == CareerRacerActivity::WaitingForLeague;
				if (AttemptedEntrants.contains(State.Runner) || IsOwnedRacer(Data, State.Runner) ||
					State.Cash < CareerRaceEntryFee || !IsAvailable ||
					State.RaceCooldownEndsAtRealMs > Data.RealTimeMs ||
					NearLeagueFixtureRacerIds.contains(RacerId) || PendingPlayoffRacerIds.contains(RacerId))
					continue;
				const CareerLeague* RacerLeague = State.LeagueId == 0 ? nullptr : FindLeague(Data, State.LeagueId);
				if (State.LeagueId == 0 || (Data.InitialWorldMonthSimulationInProgress &&
					RacerLeague != nullptr && RacerLeague->IsOpenLobby))
					Candidates.push_back(State.Runner);
				else
					LeagueCandidates.push_back(State.Runner);
			}
			if (Candidates.empty() && LeagueCandidates.empty())
				continue;
			if (Data.InitialWorldMonthSimulationInProgress)
			{
				std::stable_partition(Candidates.begin(), Candidates.end(), [](const Racer* Runner)
				{
					return Runner->CareerRecord.History.empty();
				});
				const auto FirstWithHistory = std::find_if(Candidates.begin(), Candidates.end(),
					[](const Racer* Runner) { return !Runner->CareerRecord.History.empty(); });
				std::shuffle(FirstWithHistory, Candidates.end(), Data.WorldRandom);
				if (Candidates.size() < CareerWorldRaceEntrantCount)
				{
					std::stable_partition(LeagueCandidates.begin(), LeagueCandidates.end(), [](const Racer* Runner)
					{
						return Runner->CareerRecord.History.empty();
					});
					const auto FirstLeagueEntrantWithHistory = std::find_if(LeagueCandidates.begin(),
						LeagueCandidates.end(), [](const Racer* Runner)
						{
							return !Runner->CareerRecord.History.empty();
						});
					std::shuffle(FirstLeagueEntrantWithHistory, LeagueCandidates.end(), Data.WorldRandom);
				}
			}
			else
			{
				std::shuffle(Candidates.begin(), Candidates.end(), Data.WorldRandom);
				if (Candidates.size() < CareerWorldRaceEntrantCount)
					std::shuffle(LeagueCandidates.begin(), LeagueCandidates.end(), Data.WorldRandom);
					std::shuffle(LeagueCandidates.begin(), LeagueCandidates.end(), Data.WorldRandom);
			}
			if (Candidates.size() < CareerWorldRaceEntrantCount)
			{
				const std::size_t LeagueEntrantsNeeded = CareerWorldRaceEntrantCount - Candidates.size();
				const std::size_t LeagueEntrantsToAdd = std::min(LeagueEntrantsNeeded,
					LeagueCandidates.size());
				for (std::size_t Index = 0; Index < LeagueEntrantsToAdd; ++Index)
					Candidates.push_back(LeagueCandidates[Index]);
			}

			if (Candidates.size() < CareerWorldRaceEntrantCount)
				continue;
			else if (Candidates.size() > CareerWorldRaceEntrantCount)
				Candidates.resize(CareerWorldRaceEntrantCount);
			if (std::any_of(Candidates.begin(), Candidates.end(), [&AttemptedEntrants](const Racer* Runner)
				{ return AttemptedEntrants.contains(Runner); }))
				continue;
			AttemptedEntrants.insert(Candidates.begin(), Candidates.end());

			bool Ready = false;
			std::uint64_t RetryAtMs = 0;
			RecordRaceStartTravel(Data, Candidates, TrackIndex, RetryAtMs, Ready);
			if (Ready)
			{
				if (StartWorldRace(Data, TrackIndex, Candidates, "Open city race", {}) == nullptr)
					spdlog::error("Could not start open city race at track {}.", TrackIndex);
			}
			else
				Data.PendingOpenRaces.push_back({ TrackIndex, std::move(Candidates) });
			Data.NextOpenRaceTrackIndex = (StartingTrackOffset + TrackOffset + 1) % Tracks.size();
			if (Data.InitialWorldMonthSimulationInProgress)
				break;
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

	void EnsureCareerCupLeagues(CareerData& Data)
	{
		for (const CareerWorldRegion& Region : Data.Regions)
		{
			const bool HasCup = std::any_of(Data.Leagues.begin(), Data.Leagues.end(),
				[&Region](const CareerLeague& League)
				{
					return League.IsCup && !League.IsAmateurCup &&
						League.Level == CareerCompetitionLevel::Regional && League.RegionId == Region.Id;
				});
			if (HasCup)
				continue;

			CareerLeague Cup;
			Cup.Id = Data.NextLeagueId++;
			Cup.CountryId = Region.CountryId;
			Cup.RegionId = Region.Id;
			Cup.Name = Region.Name + " Regional Cup";
			Cup.Level = CareerCompetitionLevel::Regional;
			Cup.Division = CareerLeagueDivision::Division1;
			Cup.IsCup = true;
			Data.Leagues.push_back(std::move(Cup));
		}

		for (const CareerWorldCountry& Country : Data.Countries)
		{
			const bool HasCup = std::any_of(Data.Leagues.begin(), Data.Leagues.end(),
				[&Country](const CareerLeague& League)
				{
					return League.IsCup && !League.IsAmateurCup &&
						League.Level == CareerCompetitionLevel::National && League.CountryId == Country.Id;
				});
			if (HasCup)
				continue;

			CareerLeague Cup;
			Cup.Id = Data.NextLeagueId++;
			Cup.CountryId = Country.Id;
			Cup.Name = Country.Name + " National Cup";
			Cup.Level = CareerCompetitionLevel::National;
			Cup.Division = CareerLeagueDivision::Division1;
			Cup.IsCup = true;
			Data.Leagues.push_back(std::move(Cup));
		}
	}

	void StartCareerCups(CareerData& Data)
	{
		EnsureCareerCupLeagues(Data);
		for (CareerLeague& Cup : Data.Leagues)
		{
			if (!Cup.IsCup || Cup.IsAmateurCup)
				continue;

			std::vector<Racer*> Entrants;
			std::unordered_set<std::uint64_t> EntrantIds;
			const auto AppendRankedEntrants = [&Data, &Entrants, &EntrantIds](CareerLeague* Source,
				std::size_t Count)
			{
				if (Source == nullptr)
					return;
				for (Racer* Runner : GetLeagueRunnersByRank(Data, *Source, true, Count))
				{
					const std::uint64_t RunnerId = GetRacerId(Runner);
					if (RunnerId != 0 && EntrantIds.insert(RunnerId).second)
						Entrants.push_back(Runner);
				}
			};

			if (Cup.Level == CareerCompetitionLevel::Regional)
			{
				const CareerWorldRegion* Region = FindRegion(Data, Cup.RegionId);
				if (Region != nullptr)
					for (const std::uint64_t CityId : Region->CityIds)
						AppendRankedEntrants(FindLeague(Data, CareerCompetitionLevel::City,
							CareerLeagueDivision::Division1, CityId), 2);
			}
			else if (Cup.Level == CareerCompetitionLevel::National)
			{
				for (const CareerWorldRegion& Region : Data.Regions)
					AppendRankedEntrants(FindLeague(Data, CareerCompetitionLevel::Regional,
						CareerLeagueDivision::Division1, Region.Id), CareerLeagueSize / 2);
			}

			if (Entrants.size() < 2)
			{
				Cup.State = CareerLeagueState::Filling;
				Cup.Entrants = std::move(Entrants);
				Cup.Playoffs = SportRaceCore::PlayoffBracket{};
				continue;
			}

			std::vector<std::uint64_t> CompetitorIds;
			CompetitorIds.reserve(Entrants.size());
			for (const Racer* Runner : Entrants)
				CompetitorIds.push_back(GetRacerId(Runner));
			Cup.Season++;
			Cup.Entrants = std::move(Entrants);
			Cup.Fixtures.clear();
			Cup.Standings.clear();
			Cup.Playoffs = SportRaceCore::GeneratePlayoffBracket(CompetitorIds);
			Cup.State = Cup.Playoffs.IsComplete ? CareerLeagueState::Complete : CareerLeagueState::Playoffs;
		}
	}

	bool AllSeasonsReadyToAdvance(const CareerData& Data)
	{
		bool HasCompletedSeason = false;
		for (const CareerLeague& League : Data.Leagues)
		{
			if (League.IsAmateurCup || (League.IsCup && League.State == CareerLeagueState::Filling))
				continue;
			if (League.State == CareerLeagueState::Complete)
			{
				HasCompletedSeason = true;
				continue;
			}
			if (League.State == CareerLeagueState::Filling)
				continue;
			return false;
		}
		return HasCompletedSeason;
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

	void ProcessLeaguePromotions(CareerData& Data, std::vector<std::vector<Racer*>>& Members)
	{
		struct PromotionCandidate
		{
			std::size_t SourceIndex = 0;
			CareerLeagueStanding* Standing = nullptr;
		};

		for (CareerCompetitionLevel Level : { CareerCompetitionLevel::City,
			CareerCompetitionLevel::Regional, CareerCompetitionLevel::National })
			for (CareerLeagueDivision Division : { CareerLeagueDivision::Division1,
				CareerLeagueDivision::Division2, CareerLeagueDivision::Division3,
				CareerLeagueDivision::Division4 })
			for (std::size_t TargetIndex = 0; TargetIndex < Data.Leagues.size(); ++TargetIndex)
			{
				CareerLeague& Target = Data.Leagues[TargetIndex];
				if (Target.IsCup || Target.IsAmateurCup || Target.Level != Level ||
					Target.Division != Division || (Target.State != CareerLeagueState::Complete &&
						Target.State != CareerLeagueState::Filling))
					continue;

				std::vector<PromotionCandidate> Candidates;
				std::unordered_set<Racer*> CandidateRunners;
				for (std::size_t SourceIndex = 0; SourceIndex < Data.Leagues.size(); ++SourceIndex)
				{
					CareerLeague& Source = Data.Leagues[SourceIndex];
					if (SourceIndex == TargetIndex || Source.IsCup || Source.IsAmateurCup ||
						Source.PromotionSlots == 0 || Source.PromotionTargetLeagueId != Target.Id ||
						Source.State != CareerLeagueState::Complete)
						continue;

					std::size_t AddedFromSource = 0;
					for (CareerLeagueStanding* Standing : GetSortedActiveStandings(Data, Source))
					{
						Racer* Runner = Standing == nullptr ? nullptr : Standing->Runner;
						if (Runner == nullptr || std::find(Members[SourceIndex].begin(), Members[SourceIndex].end(),
							Runner) == Members[SourceIndex].end())
							continue;
						if (Target.Division == CareerLeagueDivision::Division4)
						{
							const CareerRacerState* State = FindRacerState(Data, Runner);
							const bool HasLicense = Runner == Data.PlayerLeagueRacer ? Data.Licenses[0] :
								State != nullptr && State->HasRacingLicense;
							if (!HasLicense)
								continue;
						}
						if (CandidateRunners.insert(Runner).second)
						{
							Candidates.push_back({ SourceIndex, Standing });
							if (++AddedFromSource >= Source.PromotionSlots)
								break;
						}
					}
				}

				std::stable_sort(Candidates.begin(), Candidates.end(), [](const PromotionCandidate& Left,
					const PromotionCandidate& Right)
				{
					if (Left.Standing->Points != Right.Standing->Points)
						return Left.Standing->Points > Right.Standing->Points;
					if (Left.Standing->Wins != Right.Standing->Wins)
						return Left.Standing->Wins > Right.Standing->Wins;
					if (Left.Standing->Runner->CareerRecord.PositionPoints !=
						Right.Standing->Runner->CareerRecord.PositionPoints)
						return Left.Standing->Runner->CareerRecord.PositionPoints >
							Right.Standing->Runner->CareerRecord.PositionPoints;
					return Left.Standing->Runner->Name < Right.Standing->Runner->Name;
				});

				std::size_t PromotionLimit = std::min(Target.IncomingPromotionLimit, Candidates.size());
				if (Target.State == CareerLeagueState::Filling)
				{
					const std::size_t Vacancies = Members[TargetIndex].size() < CareerLeagueSize
						? CareerLeagueSize - Members[TargetIndex].size() : 0;
					PromotionLimit = std::min(PromotionLimit, Vacancies);
				}
				Candidates.resize(PromotionLimit);
				if (Candidates.empty())
					continue;

				std::size_t RelegationCount = 0;
				if (Target.State == CareerLeagueState::Complete &&
					Members[TargetIndex].size() + Candidates.size() > CareerLeagueSize)
					RelegationCount = std::min({ Candidates.size(), Members[TargetIndex].size(),
						Members[TargetIndex].size() + Candidates.size() - CareerLeagueSize });
				std::vector<Racer*> RelegatedRunners;
				const std::vector<CareerLeagueStanding*> TargetStandings = GetSortedActiveStandings(Data, Target);
				for (auto Standing = TargetStandings.rbegin(); Standing != TargetStandings.rend() &&
					RelegatedRunners.size() < RelegationCount; ++Standing)
					if (*Standing != nullptr && (*Standing)->Runner != nullptr &&
						std::find(Members[TargetIndex].begin(), Members[TargetIndex].end(), (*Standing)->Runner) !=
						Members[TargetIndex].end())
						RelegatedRunners.push_back((*Standing)->Runner);

				for (const PromotionCandidate& Candidate : Candidates)
					MoveMember(Members, Candidate.SourceIndex, TargetIndex, Candidate.Standing->Runner);
				for (std::size_t Index = 0; Index < RelegatedRunners.size(); ++Index)
					MoveMember(Members, TargetIndex, Candidates[Index].SourceIndex, RelegatedRunners[Index]);
		}
	}

	void AdvanceCareerSeasons(CareerData& Data)
	{
		EnsureCareerCupLeagues(Data);
		if (!AllSeasonsReadyToAdvance(Data))
			return;

		std::vector<std::vector<Racer*>> Members(Data.Leagues.size());
		for (std::size_t LeagueIndex = 0; LeagueIndex < Data.Leagues.size(); ++LeagueIndex)
			if (!Data.Leagues[LeagueIndex].IsCup && !Data.Leagues[LeagueIndex].IsAmateurCup)
			for (Racer* Runner : Data.Leagues[LeagueIndex].Entrants)
				if (Runner != nullptr && IsRacerActive(Data, Runner))
					Members[LeagueIndex].push_back(Runner);

		ProcessLeaguePromotions(Data, Members);
		StartCareerCups(Data);

		Data.PlayerLeagueId = 0;
		for (CareerRacerState& State : Data.RacerStates)
			if (State.Activity != CareerRacerActivity::Retired)
				State.LeagueId = 0;
		for (std::size_t LeagueIndex = 0; LeagueIndex < Data.Leagues.size(); ++LeagueIndex)
		{
			CareerLeague& League = Data.Leagues[LeagueIndex];
			if (League.IsCup || League.IsAmateurCup)
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
		std::vector<Racer*> Entrants;
		std::size_t FixtureIndex = InvalidPlotIndex;
		std::size_t PlayoffRoundIndex = InvalidPlotIndex;
		std::size_t PlayoffMatchIndex = InvalidPlotIndex;
		std::size_t TrackPlotIndex = InvalidPlotIndex;
		bool IsPlayoff = false;
		std::string EventName;
	};

	bool FindNextPlayerLeagueMatch(CareerData& Data, PlayerLeagueMatch& Match)
	{
		MigrateLegacyLeagueFixtures(Data);
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

				if (IsFullFieldLeagueFixture(Fixture))
				{
					std::vector<Racer*> Entrants = BuildCareerLeagueRaceEntrants(*League,
						Fixture.Fixture.Round);
					if (Entrants.size() != CareerWorldRaceEntrantCount ||
						std::find(Entrants.begin(), Entrants.end(), Data.PlayerLeagueRacer) == Entrants.end())
						continue;
					Match.League = League;
					Match.Entrants = std::move(Entrants);
					Match.FixtureIndex = Index;
					Match.TrackPlotIndex = Fixture.TrackPlotIndex;
					Match.EventName = League->Name + " - heat " +
						std::to_string(Fixture.Fixture.Round + 1);
					return true;
				}
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
			State->Activity == CareerRacerActivity::InRace || State->Activity == CareerRacerActivity::Training)
			return false;
		return State->Activity != CareerRacerActivity::Travelling ||
			State->DestinationPlotIndex == TrackPlotIndex;
	}

	bool CollectPlayerLeagueRaceEntrants(CareerData& Data, const PlayerLeagueMatch& Match,
		std::vector<Racer*>& Entrants, std::vector<CareerRaceFixtureReference>& Fixtures)
	{
		if (Match.League == nullptr || Match.TrackPlotIndex >= Data.CityPlots.size() ||
			Data.CityPlots[Match.TrackPlotIndex].Type != Game::Career::CityPlotType::RaceTrack ||
			(!Match.IsPlayoff && Match.FixtureIndex >= Match.League->Fixtures.size()))
			return false;

		if (Match.IsPlayoff)
		{
			if (Match.Opponent == nullptr ||
				!IsPlayerRaceEntrantAvailable(Data, Match.Opponent, Match.TrackPlotIndex))
				return false;
			Entrants = { Data.PlayerLeagueRacer, Match.Opponent };
			return Entrants.size() == CareerPlayoffRaceEntrantCount;
		}

		const CareerLeagueFixture& SelectedFixture = Match.League->Fixtures[Match.FixtureIndex];
		if (IsFullFieldLeagueFixture(SelectedFixture))
		{
			Entrants = Match.Entrants;
			if (Entrants.size() != CareerWorldRaceEntrantCount ||
				std::find(Entrants.begin(), Entrants.end(), Data.PlayerLeagueRacer) == Entrants.end())
				return false;
			for (Racer* Runner : Entrants)
				if (Runner != Data.PlayerLeagueRacer &&
					!IsPlayerRaceEntrantAvailable(Data, Runner, Match.TrackPlotIndex))
					return false;
			Fixtures.push_back({ Match.League->Id, Match.FixtureIndex });
			return true;
		}

		return false;
	}
}
using namespace CareerWorldSimulationDetail;

std::vector<Game::Race::Racer*> Game::Career::GetCareerLeagueRaceEntrants(
	const Game::Career::CareerLeague& League, std::size_t RaceIndex)
{
	return CareerWorldSimulationDetail::BuildCareerLeagueRaceEntrants(League, RaceIndex);
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
	std::vector<std::uint64_t> CityAssignments;
	if (!Data->Cities.empty())
	{
		const std::size_t RegionalRacersPerCity =
			(CareerLeagueSize * 4 + CareerLeagueSize / 2) / CareerCitiesPerRegion;
		const std::size_t MinimumCityRacers = CareerLeagueSize * 5 + RegionalRacersPerCity;
		const std::size_t BaseRacersPerCity = std::min(MinimumCityRacers,
			ActiveRoster.size() / Data->Cities.size());
		for (const CareerWorldCity& City : Data->Cities)
			CityAssignments.insert(CityAssignments.end(), BaseRacersPerCity, City.Id);

		std::size_t RemainingAssignments = ActiveRoster.size() - CityAssignments.size();
		const auto HomeCity = std::find_if(Data->Cities.begin(), Data->Cities.end(), [this](
			const CareerWorldCity& City) { return City.Id == Data->HomeCityId; });
		const std::uint64_t HomeCityId = HomeCity == Data->Cities.end()
			? Data->Cities.front().Id : HomeCity->Id;
		const std::size_t HomeAmateurRacers = std::min(CareerLeagueSize, RemainingAssignments);
		CityAssignments.insert(CityAssignments.end(), HomeAmateurRacers, HomeCityId);
		RemainingAssignments -= HomeAmateurRacers;

		std::vector<double> PopulationWeights;
		PopulationWeights.reserve(Data->Cities.size());
		for (const CareerWorldCity& City : Data->Cities)
			PopulationWeights.push_back(static_cast<double>(City.Population));
		std::discrete_distribution<std::size_t> CityDistribution(PopulationWeights.begin(),
			PopulationWeights.end());
		for (std::size_t Index = 0; Index < RemainingAssignments; ++Index)
			CityAssignments.push_back(Data->Cities[CityDistribution(Data->WorldRandom)].Id);
		std::shuffle(CityAssignments.begin(), CityAssignments.end(), Data->WorldRandom);
	}
	for (std::size_t Index = 0; Index < ActiveRoster.size(); ++Index)
	{
		const std::uint64_t CityId = Index < CityAssignments.size() ? CityAssignments[Index] : Data->HomeCityId;
		Data->RacerStates.push_back(MakeRacerWorldState(*Data, ActiveRoster[Index], CityId));
	}
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
	std::vector<std::vector<Race::Racer*>> CityRosters(Data->Cities.size());
	for (Race::Racer* Runner : ActiveRoster)
	{
		const CareerRacerState* State = FindRacerState(*Data, Runner);
		if (State == nullptr)
			continue;
		for (std::size_t CityIndex = 0; CityIndex < Data->Cities.size(); ++CityIndex)
			if (Data->Cities[CityIndex].Id == State->CityId)
			{
				CityRosters[CityIndex].push_back(Runner);
				break;
			}
	}
	const auto TakeRacers = [](std::vector<Race::Racer*>& Roster, std::size_t Count)
	{
		Count = std::min(Count, Roster.size());
		std::vector<Race::Racer*> Taken(Roster.begin(), Roster.begin() + Count);
		Roster.erase(Roster.begin(), Roster.begin() + Count);
		return Taken;
	};
	const std::array<CareerLeagueDivision, 4> Divisions{
		CareerLeagueDivision::Division1,
		CareerLeagueDivision::Division2,
		CareerLeagueDivision::Division3,
		CareerLeagueDivision::Division4
	};
	std::vector<std::array<std::uint64_t, 4>> CityLeagueIds(Data->Cities.size());
	for (std::size_t CityIndex = 0; CityIndex < Data->Cities.size(); ++CityIndex)
	{
		const CareerWorldCity& City = Data->Cities[CityIndex];
		const CareerWorldRegion* Region = FindRegion(*Data, City.RegionId);
		const std::uint64_t CountryId = Region == nullptr ? 0 : Region->CountryId;
		std::vector<Race::Racer*> Entrants = TakeRacers(CityRosters[CityIndex], CareerLeagueSize * 4);
		for (std::size_t DivisionIndex = 0; DivisionIndex < Divisions.size(); ++DivisionIndex)
		{
			const std::size_t First = std::min(DivisionIndex * CareerLeagueSize, Entrants.size());
			const std::size_t Last = std::min(First + CareerLeagueSize, Entrants.size());
			std::vector<Race::Racer*> DivisionEntrants(Entrants.begin() + First, Entrants.begin() + Last);
			const std::size_t IncomingLimit = City.Id == Data->HomeCityId &&
				Divisions[DivisionIndex] == CareerLeagueDivision::Division4 ? CareerLeagueSize - 2 : 2;
			AddLeague(*Data, City.Name + " " + CareerLeagueDivisionToString(Divisions[DivisionIndex]),
				Divisions[DivisionIndex], DivisionEntrants, 0, false, CareerCompetitionLevel::City,
				City.Id, City.RegionId, 2, IncomingLimit);
			CityLeagueIds[CityIndex][DivisionIndex] = Data->Leagues.back().Id;
			Data->Leagues.back().CountryId = CountryId;
		}
		for (std::size_t DivisionIndex = 1; DivisionIndex < Divisions.size(); ++DivisionIndex)
			if (CareerLeague* League = FindLeague(*Data, CityLeagueIds[CityIndex][DivisionIndex]))
				League->PromotionTargetLeagueId = CityLeagueIds[CityIndex][DivisionIndex - 1];
	}

	for (std::size_t CityIndex = 0; CityIndex < Data->Cities.size(); ++CityIndex)
	{
		const CareerWorldCity& City = Data->Cities[CityIndex];
		const CareerWorldRegion* Region = FindRegion(*Data, City.RegionId);
		const std::uint64_t CountryId = Region == nullptr ? 0 : Region->CountryId;
		const std::uint64_t Division4Id = CityLeagueIds[CityIndex][3];
		std::vector<Race::Racer*> Entrants = TakeRacers(CityRosters[CityIndex], CareerLeagueSize);
		AddLeague(*Data, City.Name + " Amateur League", CareerLeagueDivision::Amateur, Entrants,
			Division4Id, false, CareerCompetitionLevel::City, City.Id, City.RegionId, 2, 2);
		Data->Leagues.back().CountryId = CountryId;
	}
	const auto HomeCity = std::find_if(Data->Cities.begin(), Data->Cities.end(), [this](
		const CareerWorldCity& City) { return City.Id == Data->HomeCityId; });
	if (HomeCity != Data->Cities.end())
	{
		const std::size_t HomeCityIndex = static_cast<std::size_t>(HomeCity - Data->Cities.begin());
		const CareerWorldRegion* Region = FindRegion(*Data, HomeCity->RegionId);
		const std::uint64_t CountryId = Region == nullptr ? 0 : Region->CountryId;
		std::vector<Race::Racer*> Entrants = TakeRacers(CityRosters[HomeCityIndex], CareerLeagueSize - 1);
		AddLeague(*Data, HomeCity->Name + " Amateur Lobby", CareerLeagueDivision::Amateur,
			Entrants, CityLeagueIds[HomeCityIndex][3], true, CareerCompetitionLevel::City,
			HomeCity->Id, HomeCity->RegionId, 2, CareerLeagueSize - 2);
		Data->Leagues.back().CountryId = CountryId;
	}

	std::vector<std::vector<std::uint64_t>> RegionalLeagueIds(Data->Regions.size());
	std::vector<Race::Racer*> NationalEntrants;
	for (std::size_t RegionIndex = 0; RegionIndex < Data->Regions.size(); ++RegionIndex)
	{
		const CareerWorldRegion& Region = Data->Regions[RegionIndex];
		std::vector<Race::Racer*> RegionRacers;
		for (const std::uint64_t CityId : Region.CityIds)
			for (std::size_t CityIndex = 0; CityIndex < Data->Cities.size(); ++CityIndex)
				if (Data->Cities[CityIndex].Id == CityId)
					RegionRacers.insert(RegionRacers.end(), CityRosters[CityIndex].begin(),
						CityRosters[CityIndex].end());
		std::stable_sort(RegionRacers.begin(), RegionRacers.end(), [](const Race::Racer* Left,
			const Race::Racer* Right)
		{
			const double LeftStrength = EstimateRacerStrength(*Left, 500);
			const double RightStrength = EstimateRacerStrength(*Right, 500);
			if (LeftStrength != RightStrength)
				return LeftStrength > RightStrength;
			return Left->GUID.GUID < Right->GUID.GUID;
		});
		std::vector<Race::Racer*> TopRegionRacers = TakeRacers(RegionRacers, CareerLeagueSize / 2);
		NationalEntrants.insert(NationalEntrants.end(), TopRegionRacers.begin(), TopRegionRacers.end());
		std::vector<Race::Racer*> RegionalEntrants = TakeRacers(RegionRacers, CareerLeagueSize * 4);
		RegionalLeagueIds[RegionIndex].resize(Divisions.size());
		for (std::size_t DivisionIndex = 0; DivisionIndex < Divisions.size(); ++DivisionIndex)
		{
			const std::size_t First = std::min(DivisionIndex * CareerLeagueSize, RegionalEntrants.size());
			const std::size_t Last = std::min(First + CareerLeagueSize, RegionalEntrants.size());
			std::vector<Race::Racer*> DivisionEntrants(RegionalEntrants.begin() + First,
				RegionalEntrants.begin() + Last);
			const std::size_t PromotionSlots = DivisionIndex == 0 ? CareerLeagueSize / 2 : 2;
			AddLeague(*Data, Region.Name + " " + CareerLeagueDivisionToString(Divisions[DivisionIndex]),
				Divisions[DivisionIndex], DivisionEntrants, 0, false, CareerCompetitionLevel::Regional,
				0, Region.Id, PromotionSlots, 2);
			RegionalLeagueIds[RegionIndex][DivisionIndex] = Data->Leagues.back().Id;
			Data->Leagues.back().CountryId = Region.CountryId;
		}
		for (std::size_t DivisionIndex = 1; DivisionIndex < Divisions.size(); ++DivisionIndex)
			if (CareerLeague* League = FindLeague(*Data, RegionalLeagueIds[RegionIndex][DivisionIndex]))
				League->PromotionTargetLeagueId = RegionalLeagueIds[RegionIndex][DivisionIndex - 1];
	}

	if (!Data->Countries.empty())
	{
		const CareerWorldCountry& Country = Data->Countries.front();
		AddLeague(*Data, Country.Name + " National League", CareerLeagueDivision::Division1,
			NationalEntrants, 0, false, CareerCompetitionLevel::National, 0, 0, 0, CareerLeagueSize);
		CareerLeague& NationalLeague = Data->Leagues.back();
		NationalLeague.CountryId = Country.Id;
		for (std::size_t RegionIndex = 0; RegionIndex < Data->Regions.size(); ++RegionIndex)
			if (CareerLeague* League = FindLeague(*Data, RegionalLeagueIds[RegionIndex][0]))
				League->PromotionTargetLeagueId = NationalLeague.Id;
	}
	for (std::size_t CityIndex = 0; CityIndex < Data->Cities.size(); ++CityIndex)
		for (std::size_t RegionIndex = 0; RegionIndex < Data->Regions.size(); ++RegionIndex)
			if (Data->Cities[CityIndex].RegionId == Data->Regions[RegionIndex].Id)
				if (CareerLeague* League = FindLeague(*Data, CityLeagueIds[CityIndex][0]))
					League->PromotionTargetLeagueId = RegionalLeagueIds[RegionIndex][3];

	for (CareerLeague& League : Data->Leagues)
		if (!League.IsCup && !League.IsAmateurCup && League.Entrants.size() == CareerLeagueSize)
			StartLeagueSeason(*Data, League);
	EnsureCareerCupLeagues(*Data);
	StartCareerCups(*Data);
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
		spdlog::error("Cannot simulate the career start week before world initialization.");
		return;
	}

	const std::uint64_t TimeScale = Data->WorldClock.GetTimeScale();
	if (TimeScale == 0 || CareerInitialWorldPresimMs % TimeScale != 0)
	{
		Data->InitialWorldMonthSimulationFailed = true;
		spdlog::error("Cannot simulate an exact career start week at world time scale {}.", TimeScale);
		return;
	}

	Data->InitialWorldMonthTotalRealMs = CareerInitialWorldPresimMs / TimeScale;
	Data->InitialWorldMonthElapsedRealMs = 0;
	Data->InitialWorldMonthStartingWorldMs = Data->WorldClock.NowMs();
	Data->InitialWorldMonthStartingRealMs = Data->RealTimeMs;
	if (CareerInitialWorldPresimMs > std::numeric_limits<std::uint64_t>::max() -
		Data->InitialWorldMonthStartingWorldMs ||
		Data->InitialWorldMonthTotalRealMs > std::numeric_limits<std::uint64_t>::max() -
		Data->InitialWorldMonthStartingRealMs)
	{
		Data->InitialWorldMonthSimulationFailed = true;
		spdlog::error("Cannot simulate the career start week: the world or real-time clock would overflow.");
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
	spdlog::info("Career start simulated at least 7 world days in {} ms; {}/{} active AI racers have race history ({} results).",
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
		League->Entrants.size() < CareerLeagueSize && IsLeagueAtStableLocation(*Data, *League);
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
	League->IsOpenLobby = false;
	League->Entrants.insert(League->Entrants.begin(), Data->PlayerLeagueRacer);

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
		League->Entrants.insert(League->Entrants.begin(), Runner);
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
		const std::size_t ExpectedEntrantCount = Pending.IsPlayoff
			? CareerPlayoffRaceEntrantCount : CareerWorldRaceEntrantCount;
		if (Pending.Entrants.size() != ExpectedEntrantCount ||
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
				State->LocationPlotIndex != Pending.TrackPlotIndex)
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
	PreparedRace.Financials = Race::RaceFinancials(
		GetLeagueRaceFirstPrize(*Data, Pending.LeagueFixtures, Pending.LeagueId, Pending.IsPlayoff), 0u);
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
		State->Cash -= static_cast<int>(Pending.Financials.EntranceFee);
		State->Activity = CareerRacerActivity::InRace;
		State->ActiveRaceId = 0;
		State->ActionEndsAtRealMs = Data->RealTimeMs;
		State->LocationPlotIndex = Pending.TrackPlotIndex;
		State->DestinationPlotIndex = Pending.TrackPlotIndex;
	}

	RaceBuffer = std::move(PreparedRace);
	return true;
}

void Game::Career::CareerOrchestrator::CompletePlayerLeagueRace(const Race::RaceResult& Result,
	std::uint64_t HistoricalRaceId)
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
				State->Cash += static_cast<int>(Pending.Financials.EntranceFee);

			if (State->Activity != CareerRacerActivity::Retired)
			{
				State->Activity = CareerRacerActivity::Resting;
				State->ActiveRaceId = 0;
				State->ActionEndsAtRealMs = Data->RealTimeMs + CareerPostRaceRestMs;
				State->RaceCooldownEndsAtRealMs = Data->RealTimeMs + CareerPostRaceRestMs;
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
			else
				League->Playoffs.Rounds[Pending.PlayoffRoundIndex].Matches[
					Pending.PlayoffMatchIndex].HistoricalRaceId = HistoricalRaceId;
		}
	}
	else if (Successful)
	{
		if (HistoricalRaceId != 0)
			for (const CareerRaceFixtureReference& Reference : Pending.LeagueFixtures)
				if (CareerLeague* FixtureLeague = FindLeague(*Data, Reference.LeagueId);
					FixtureLeague != nullptr && Reference.FixtureIndex < FixtureLeague->Fixtures.size())
					FixtureLeague->Fixtures[Reference.FixtureIndex].HistoricalRaceId = HistoricalRaceId;
		RecordLeagueRaceResults(*Data, Result, Pending.LeagueFixtures);
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
	MigrateLegacyLeagueFixtures(*Data);
	const bool HasLegacyLeagueRoster = std::any_of(Data->Leagues.begin(), Data->Leagues.end(),
		[](const CareerLeague& League)
		{
			return !League.IsCup && !League.IsAmateurCup && !League.IsOpenLobby &&
				League.Entrants.size() == CareerPreviousLeagueSize;
		});
	if (HasLegacyLeagueRoster && Data->TargetAiPopulation < CareerTargetAiPopulation)
		SpawnRacers(CareerTargetAiPopulation - Data->TargetAiPopulation);

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

std::string Game::Career::CareerCompetitionLevelToString(CareerCompetitionLevel Level)
{
	switch (Level)
	{
	case CareerCompetitionLevel::City:
		return "City";
	case CareerCompetitionLevel::Regional:
		return "Regional";
	case CareerCompetitionLevel::National:
		return "National";
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