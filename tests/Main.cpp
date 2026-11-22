#include "TestHarness.hpp"
#include "App\CommandLine.hpp"
#include "Appraisal.h"
#include "Career.h"
#include "Race.h"
#include "Racer.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

using namespace Game::App;
using namespace Game::Race;

namespace
{
	void ConfigureRacer(Racer& Runner, unsigned int Guid, double Reflex = 100.0, double Acceleration = 100.0,
		double Endurance = 100.0, double Speed = 100.0, double Confidence = 0.5, double Luck = 0.0, double Fatigue = 0.0)
	{
		Runner.GUID = Guid;
		Runner.Skills.Reflex = Reflex;
		Runner.Skills.Composure = 50.0;
		Runner.Skills.Acceleration = Acceleration;
		Runner.Skills.Endurance = Endurance;
		Runner.Skills.Speed = Speed;
		Runner.Skills.Potential = 100.0;
		Runner.Skills.Trainability = 50.0;
		Runner.Skills.Luck = Luck;
		Runner.Conditions.Fatigue = Fatigue;
		Runner.Conditions.Confidence = Confidence;
		Runner.Conditions.Injury = 0.0;
		Runner.AccelerationCurve = { 0.0, 0.0, 1.0, 1.0, false };
		Runner.EnduranceCurve = { 0.0, 1.0, 1.0, 0.0, true };
	}

	bool ConfigureRace(Race& RaceToConfigure, unsigned int Seed, unsigned int TrackLength)
	{
		if (RaceToConfigure.SetSeed(Seed) != RaceError::None)
			return false;
		RaceToConfigure.GUID.GUID = Seed + 1000;
		RaceToConfigure.ThisTrack->Length = static_cast<Game::Race::TrackLength>(TrackLength);
		return true;
	}
}

TEST_CASE(CommandLineParser_ParsesApplicationPath)
{
	char app[] = "SportsRace.exe";
	char* argv[] = { app };

	auto arguments = CommandLineParser::Parse(1, argv);

	ASSERT_EQ(static_cast<size_t>(1), arguments.size());
	ASSERT_TRUE(arguments[0].Type == CommandLineArgumentType::ApplicationPath);
	ASSERT_EQ(std::string("SportsRace.exe"), arguments[0].Data);
}

TEST_CASE(CommandLineParser_ParsesArgumentsWithParameters)
{
	char app[] = "SportsRace.exe";
	char log[] = "-log";
	char level[] = "3";
	char cfg[] = "/cfg";
	char configPath[] = "config.json";
	char* argv[] = { app, log, level, cfg, configPath };

	auto arguments = CommandLineParser::Parse(5, argv);

	ASSERT_EQ(static_cast<size_t>(3), arguments.size());
	ASSERT_TRUE(arguments[1].Type == CommandLineArgumentType::Argument);
	ASSERT_EQ(std::string("log"), arguments[1].Data);
	ASSERT_EQ(std::string("3"), arguments[1].Parameters[0]);
	ASSERT_EQ(std::string("cfg"), arguments[2].Data);
	ASSERT_EQ(std::string("config.json"), arguments[2].Parameters[0]);
}

TEST_CASE(CommandLineParser_ParsesOptionalArguments)
{
	char app[] = "SportsRace.exe";
	char showFps[] = "--showfps";
	char* argv[] = { app, showFps };

	auto arguments = CommandLineParser::Parse(2, argv);

	ASSERT_EQ(static_cast<size_t>(2), arguments.size());
	ASSERT_TRUE(arguments[1].Type == CommandLineArgumentType::OptionalArgument);
	ASSERT_EQ(std::string("showfps"), arguments[1].Data);
}

TEST_CASE(Racer_ResetRestoresRaceState)
{
	srand(1);
	Racer racer("Test Racer");
	racer.CurrentTick = 100;
	racer.LastRunFrameEnd = 50;
	racer.RunFrame = 2;
	racer.Pos.X = 250;

	racer.Reset();

	ASSERT_EQ(0u, racer.CurrentTick);
	ASSERT_EQ(0u, racer.LastRunFrameEnd);
	ASSERT_EQ(0u, racer.RunFrame);
	ASSERT_EQ(0, racer.Pos.X);
}

TEST_CASE(RacerDB_MakeStoresAndReturnsRacer)
{
	srand(2);
	RacerDB db;

	Racer* racer = db.Make("Stored Racer");

	ASSERT_TRUE(racer != nullptr);
	ASSERT_EQ(std::string("Stored Racer"), racer->Name);
	ASSERT_TRUE(db.Contains(racer->GUID.GUID));
	ASSERT_TRUE(db.Get(racer->GUID.GUID) == racer);
	ASSERT_EQ(static_cast<size_t>(1), db.Size());
}

TEST_CASE(RacerDB_GetAllReturnsSortedRacers)
{
	srand(21);
	RacerDB db;
	db.Make("Zed Stablehand");
	db.Make("Alice Odds");
	db.Make("Alice Odds");

	const std::vector<Racer*> racers = db.GetAll();

	ASSERT_EQ(static_cast<size_t>(3), racers.size());
	ASSERT_TRUE(racers[0]->Name <= racers[1]->Name);
	ASSERT_TRUE(racers[1]->Name <= racers[2]->Name);
	if (racers[0]->Name == racers[1]->Name)
		ASSERT_TRUE(racers[0]->GUID.GUID < racers[1]->GUID.GUID);
}

TEST_CASE(Racer_RecordCareerResultAwardsPositionPointsAndXp)
{
	Racer racer("Career Record Runner");
	racer.RecordCareerResult(PulledUpPosition, 0);
	racer.RecordCareerResult(FirstPlaceRank, 1200);
	racer.RecordCareerResult(SecondPlaceRank, 1300);
	racer.RecordCareerResult(ThirdPlaceRank, 1100);
	racer.RecordCareerResult(4, 1000);

	ASSERT_EQ(static_cast<std::uint64_t>(1), racer.CareerRecord.Wins);
	ASSERT_EQ(static_cast<std::uint64_t>(4), racer.CareerRecord.Losses);
	ASSERT_EQ(static_cast<std::uint64_t>(5), racer.CareerRecord.RacesEntered);
	ASSERT_EQ(static_cast<std::uint64_t>(6), racer.CareerRecord.PositionPoints);
	ASSERT_EQ(static_cast<std::uint64_t>(80), racer.Skills.Xp);
	ASSERT_EQ(1000u, racer.CareerRecord.BestTimeMs);
}

TEST_CASE(Racer_RecordCareerResultStoresRaceHistory)
{
	Racer racer("Race History Runner");
	RacerCareerHistoryEntry HistoryEntry;
	HistoryEntry.WorldTimeMs = 86400000;
	HistoryEntry.EventName = "Division 4 Round 1";
	HistoryEntry.TrackLength = 5000;

	racer.RecordCareerResult(SecondPlaceRank, 1234, HistoryEntry);

	ASSERT_EQ(static_cast<std::size_t>(1), racer.CareerRecord.History.size());
	ASSERT_EQ(static_cast<std::uint64_t>(1), racer.CareerRecord.RacesEntered);
	ASSERT_EQ(std::string("Division 4 Round 1"), racer.CareerRecord.History[0].EventName);
	ASSERT_EQ(5000u, racer.CareerRecord.History[0].TrackLength);
	ASSERT_EQ(SecondPlaceRank, racer.CareerRecord.History[0].Position);
	ASSERT_EQ(1234u, racer.CareerRecord.History[0].FinishTimeMs);
}

TEST_CASE(Racer_XpAccumulatesBeyond100)
{
	Racer racer("Veteran Runner");
	for (unsigned int RaceIndex = 0; RaceIndex < 11; ++RaceIndex)
		racer.RecordCareerResult(4, 0);

	ASSERT_EQ(static_cast<std::uint64_t>(110), racer.Skills.Xp);
}

TEST_CASE(Appraisal_OneHundredPercentReturnsBaseRating)
{
	ASSERT_EQ(42.5, Game::Appraisal::AppraiseRating(42.5, 100.0, 7, 11, 3));
}

TEST_CASE(Appraisal_RepeatsTheSameEstimateForIdenticalInputs)
{
	const double FirstEstimate = Game::Appraisal::AppraiseRating(50.0, 90.0, 123, 456, 2);
	const double RepeatedEstimate = Game::Appraisal::AppraiseRating(50.0, 90.0, 123, 456, 2);

	ASSERT_EQ(FirstEstimate, RepeatedEstimate);
}

TEST_CASE(Appraisal_NinetyPercentAccuracyAvoidsExtremeMidrangeRatings)
{
	for (std::uint64_t TargetFingerprint = 0; TargetFingerprint < 512; ++TargetFingerprint)
	{
		const double Estimate = Game::Appraisal::AppraiseRating(50.0, 90.0, 17, TargetFingerprint, 1);
		ASSERT_TRUE(Estimate > 1.0 && Estimate < 99.0);
	}
}

TEST_CASE(Appraisal_RacerFingerprintChangesWhenNameOrSkillChanges)
{
	srand(22);
	Racer racer("Stable Name");
	const std::uint64_t OriginalFingerprint = Game::Appraisal::HashRacer(racer);

	racer.Skills.Speed += 1.0;
	const std::uint64_t ChangedSkillFingerprint = Game::Appraisal::HashRacer(racer);
	ASSERT_TRUE(OriginalFingerprint != ChangedSkillFingerprint);

	racer.Name = "Changed Name";
	ASSERT_TRUE(ChangedSkillFingerprint != Game::Appraisal::HashRacer(racer));
}

TEST_CASE(Career_InitPopulatesRosterOnceAndAssignsHiddenAccuracy)
{
	srand(23);
	Game::Career::CareerData careerData;
	Game::Career::CareerProfile careerProfile;
	Game::Career::CareerOrchestrator orchestrator(&careerData, &careerProfile);
	const double Accuracy = careerProfile.Accuracy;

	ASSERT_TRUE(Accuracy > 0.0 && Accuracy < 100.0);
	ASSERT_TRUE(careerProfile.AppraisalIdentity != 0);
	orchestrator.Init();
	ASSERT_EQ(static_cast<size_t>(256), careerData.Racers.Size());
	ASSERT_TRUE(careerData.CityPlots.size() >= 40);
	ASSERT_EQ(static_cast<size_t>(13), careerData.RaceTrackLengths.size());
	ASSERT_EQ(static_cast<size_t>(2), careerData.OwnedRacers.size());
	const std::size_t InitialCityPlotCount = careerData.CityPlots.size();
	orchestrator.Init();
	ASSERT_EQ(static_cast<size_t>(256), careerData.Racers.Size());
	ASSERT_EQ(InitialCityPlotCount, careerData.CityPlots.size());
	ASSERT_EQ(static_cast<size_t>(13), careerData.RaceTrackLengths.size());
	ASSERT_EQ(static_cast<size_t>(2), careerData.OwnedRacers.size());
	ASSERT_EQ(Accuracy, careerProfile.Accuracy);
}

TEST_CASE(Career_RaceTrackCountGrowsWithAiPopulation)
{
	srand(2302);
	Game::Career::CareerData careerData;
	Game::Career::CareerProfile careerProfile;
	Game::Career::CareerOrchestrator orchestrator(&careerData, &careerProfile);
	orchestrator.Init();

	ASSERT_EQ(static_cast<std::size_t>(13), careerData.RaceTrackLengths.size());
	ASSERT_EQ(static_cast<std::size_t>(13), std::count_if(careerData.CityPlots.begin(), careerData.CityPlots.end(),
		[](const Game::Career::CityPlot& Plot)
		{
			return Plot.Type == Game::Career::CityPlotType::RaceTrack;
		}));
	ASSERT_EQ(static_cast<std::size_t>(20), orchestrator.SpawnRacers(20));
	ASSERT_EQ(static_cast<std::size_t>(274), careerData.TargetAiPopulation);
	ASSERT_EQ(static_cast<std::size_t>(14), careerData.RaceTrackLengths.size());
	ASSERT_EQ(static_cast<std::size_t>(14), std::count_if(careerData.CityPlots.begin(), careerData.CityPlots.end(),
		[](const Game::Career::CityPlot& Plot)
		{
			return Plot.Type == Game::Career::CityPlotType::RaceTrack;
		}));
}

TEST_CASE(Career_PreparesMatchedRacesOnSelectedCityTracks)
{
	srand(230);
	Game::Career::CareerData careerData;
	Game::Career::CareerProfile careerProfile;
	Game::Career::CareerOrchestrator orchestrator(&careerData, &careerProfile);
	orchestrator.Init();
	careerProfile.MainFella.CareerRecord.PositionPoints = 7;
	careerData.Racers.GetAll()[0]->CareerRecord.PositionPoints = 7;

	const unsigned int TrackLength = careerData.RaceTrackLengths[0];
	ASSERT_TRUE(orchestrator.PrepareUpcomingRace(1234, TrackLength));
	ASSERT_EQ(static_cast<std::size_t>(6), careerData.UpcomingRaceEntrants.size());
	ASSERT_TRUE(careerData.UpcomingRaceEntrants[0] == &careerProfile.MainFella);
	ASSERT_EQ(TrackLength, careerData.UpcomingRaceTrackLength);
	for (std::size_t Index = 1; Index < careerData.UpcomingRaceEntrants.size(); ++Index)
		ASSERT_TRUE(std::find(careerData.OwnedRacers.begin(), careerData.OwnedRacers.end(),
			careerData.UpcomingRaceEntrants[Index]) == careerData.OwnedRacers.end());

	const Race UpcomingRace = orchestrator.CreateUpcomingRace();
	ASSERT_EQ(TrackLength, UpcomingRace.GetTrackLength());
	ASSERT_EQ(static_cast<std::size_t>(6), UpcomingRace.Racers.size());
	careerData.PendingBet = { careerData.UpcomingRaceEntrants[1], 25, 2.0, true };
	ASSERT_TRUE(orchestrator.PrepareUpcomingRace(5678, careerData.RaceTrackLengths[1]));
	ASSERT_EQ(TrackLength, careerData.UpcomingRaceTrackLength);
}

TEST_CASE(Career_PredictRaceEstimatesWithoutMutatingLiveRacers)
{
	srand(2301);
	Racer Favorite("Favorite");
	Racer Underdog("Underdog");
	ConfigureRacer(Favorite, 2301, 100.0, 100.0, 100.0, 100.0);
	ConfigureRacer(Underdog, 2302, 20.0, 20.0, 20.0, 20.0);
	Favorite.Conditions.Fatigue = 0.1;
	Underdog.Conditions.Fatigue = 0.2;
	const std::vector<Racer*> Entrants{ &Favorite, &Underdog };

	const Game::Career::RacePrediction Prediction = Game::Career::PredictRace(Entrants,
		static_cast<unsigned int>(TrackLength::Standard100m), 2303, 32);

	ASSERT_TRUE(Prediction.Error == RaceError::None);
	ASSERT_EQ(static_cast<std::size_t>(2), Prediction.AverageFinishTimesMs.size());
	ASSERT_EQ(static_cast<std::size_t>(2), Prediction.WinProbabilities.size());
	ASSERT_TRUE(Prediction.WinProbabilities[0] > Prediction.WinProbabilities[1]);
	ASSERT_TRUE(std::abs(Prediction.WinProbabilities[0] + Prediction.WinProbabilities[1] - 1.0) < 0.000001);
	ASSERT_TRUE(Prediction.AverageFinishTimesMs[0] < Prediction.AverageFinishTimesMs[1]);
	ASSERT_EQ(0, Favorite.Pos.X);
	ASSERT_EQ(0, Underdog.Pos.X);
	ASSERT_EQ(0.1, Favorite.Conditions.Fatigue);
	ASSERT_EQ(0.2, Underdog.Conditions.Fatigue);
	ASSERT_EQ(static_cast<std::uint64_t>(0), Favorite.CareerRecord.Wins);
	ASSERT_EQ(static_cast<std::uint64_t>(0), Underdog.CareerRecord.Wins);
}

TEST_CASE(Career_EstimateRacerStrengthWeightsAccelerationAndEnduranceByTrackLength)
{
	Racer Sprinter("Sprinter");
	Racer Stayer("Stayer");
	for (Racer* Runner : { &Sprinter, &Stayer })
	{
		Runner->Skills.Speed = 50.0;
		Runner->Skills.Reflex = 50.0;
		Runner->Skills.Composure = 50.0;
		Runner->Skills.Luck = 50.0;
	}
	Sprinter.Skills.Acceleration = 100.0;
	Sprinter.Skills.Endurance = 0.0;
	Stayer.Skills.Acceleration = 0.0;
	Stayer.Skills.Endurance = 100.0;

	const double ShortTrackSprinterStrength = Game::Career::EstimateRacerStrength(Sprinter, 100);
	const double ShortTrackStayerStrength = Game::Career::EstimateRacerStrength(Stayer, 100);
	const double LongTrackSprinterStrength = Game::Career::EstimateRacerStrength(Sprinter, 1000);
	const double LongTrackStayerStrength = Game::Career::EstimateRacerStrength(Stayer, 1000);

	ASSERT_TRUE(ShortTrackSprinterStrength > ShortTrackStayerStrength);
	ASSERT_TRUE(LongTrackStayerStrength > LongTrackSprinterStrength);
	ASSERT_EQ(0, Sprinter.Pos.X);
	ASSERT_EQ(0, Stayer.Pos.X);
}

TEST_CASE(Career_EstimateRacerStrengthAccountsForRaceDayConditions)
{
	Racer Fresh("Fresh Runner");
	Racer Fatigued("Fatigued Runner");
	for (Racer* Runner : { &Fresh, &Fatigued })
	{
		Runner->Skills.Speed = 50.0;
		Runner->Skills.Acceleration = 50.0;
		Runner->Skills.Reflex = 50.0;
		Runner->Skills.Endurance = 50.0;
		Runner->Skills.Composure = 50.0;
		Runner->Skills.Luck = 50.0;
	}
	Fatigued.Conditions.Fatigue = 1.0;

	ASSERT_TRUE(Game::Career::EstimateRacerStrength(Fresh, 500) >
		Game::Career::EstimateRacerStrength(Fatigued, 500));
	ASSERT_EQ(0, Fresh.Pos.X);
	ASSERT_EQ(0, Fatigued.Pos.X);
}

TEST_CASE(Career_BreedingAddsAnOwnedDescendantAndRejectsInvalidParents)
{
	srand(2302);
	Game::Career::CareerData careerData;
	Game::Career::CareerProfile careerProfile;
	Game::Career::CareerOrchestrator orchestrator(&careerData, &careerProfile);
	orchestrator.Init();
	Racer* FirstParent = careerData.OwnedRacers[0];
	Racer* SecondParent = careerData.OwnedRacers[1];
	FirstParent->Skills.Reflex = FirstParent->Skills.Composure = FirstParent->Skills.Acceleration =
		FirstParent->Skills.Endurance = FirstParent->Skills.Speed = 40.0;
	SecondParent->Skills.Reflex = SecondParent->Skills.Composure = SecondParent->Skills.Acceleration =
		SecondParent->Skills.Endurance = SecondParent->Skills.Speed = 60.0;

	Racer* Child = orchestrator.BreedOwnedRacers(FirstParent, SecondParent, 9876);

	ASSERT_TRUE(Child != nullptr);
	ASSERT_EQ(static_cast<std::size_t>(3), careerData.OwnedRacers.size());
	ASSERT_TRUE(careerData.OwnedRacers.back() == Child);
	ASSERT_TRUE(careerData.Racers.Get(Child->GUID.GUID) == Child);
	ASSERT_EQ(0u, Child->Details.Age);
	ASSERT_EQ(static_cast<std::uint64_t>(0), Child->CareerRecord.Wins);
	ASSERT_TRUE(Child->Skills.Reflex >= 30.0 && Child->Skills.Reflex <= 70.0);
	ASSERT_TRUE(orchestrator.BreedOwnedRacers(FirstParent, FirstParent, 1) == nullptr);
	ASSERT_TRUE(orchestrator.BreedOwnedRacers(FirstParent, &careerProfile.MainFella, 1) == nullptr);
	ASSERT_EQ(static_cast<std::size_t>(3), careerData.OwnedRacers.size());
}

TEST_CASE(Career_SettlesWinningBetAndClearsTheScheduledRace)
{
	srand(2303);
	Game::Career::CareerData careerData;
	Game::Career::CareerProfile careerProfile;
	Game::Career::CareerOrchestrator orchestrator(&careerData, &careerProfile);
	orchestrator.Init();
	ASSERT_TRUE(orchestrator.PrepareUpcomingRace(4567));
	careerProfile.Financials.Cash = 4975;
	careerData.PendingBet = { &careerProfile.MainFella, 25, 2.0, true };
	RaceResult Result;
	Result.RacerResults.emplace_back(&careerProfile.MainFella, FirstPlaceRank, 1000, RaceGUID(4567));

	orchestrator.CompleteUpcomingRace(Result);

	ASSERT_EQ(5025, careerProfile.Financials.Cash);
	ASSERT_TRUE(!careerData.PendingBet.Active);
	ASSERT_TRUE(careerData.UpcomingRaceEntrants.empty());
	ASSERT_EQ(0u, careerData.UpcomingRaceTrackLength);
}

TEST_CASE(Career_Initializes256RacerWorldAndCreatesOpenAmateurLobbies)
{
	srand(231);
	Game::Career::CareerData careerData;
	Game::Career::CareerProfile careerProfile;
	Game::Career::CareerOrchestrator orchestrator(&careerData, &careerProfile);
	orchestrator.Init();

	ASSERT_TRUE(careerData.WorldSimulationInitialized);
	ASSERT_EQ(static_cast<std::size_t>(256), careerData.Racers.Size());
	ASSERT_EQ(static_cast<std::size_t>(2), careerData.OwnedRacers.size());
	ASSERT_EQ(static_cast<std::size_t>(254), orchestrator.GetActiveRacerCount());
	ASSERT_EQ(static_cast<std::size_t>(254), careerData.TargetAiPopulation);
	ASSERT_EQ(static_cast<std::size_t>(254), careerData.RacerStates.size());
	ASSERT_EQ(static_cast<std::size_t>(7), careerData.Leagues.size());
	ASSERT_EQ(static_cast<std::size_t>(13), careerData.RaceTrackLengths.size());
	ASSERT_EQ(static_cast<std::size_t>(13), std::count_if(careerData.CityPlots.begin(), careerData.CityPlots.end(),
		[](const Game::Career::CityPlot& Plot)
		{
			return Plot.Type == Game::Career::CityPlotType::RaceTrack;
		}));
	ASSERT_EQ(static_cast<std::uint64_t>(0), careerData.WorldClock.NowMs());
	ASSERT_EQ(static_cast<std::uint64_t>(0), careerData.RealTimeMs);

	std::size_t AmateurLeagueCount = 0;
	std::size_t DivisionLeagueCount = 0;
	bool HasAmateurCup = false;
	std::unordered_set<unsigned int> LeagueEntrantIds;
	for (const Game::Career::CareerLeague& League : careerData.Leagues)
	{
		if (League.IsAmateurCup)
		{
			HasAmateurCup = true;
			ASSERT_EQ(std::string("Amateur Cup"), League.Name);
			ASSERT_TRUE(League.State == Game::Career::CareerLeagueState::Filling);
			ASSERT_TRUE(League.Entrants.empty());
			ASSERT_TRUE(League.Fixtures.empty());
			ASSERT_TRUE(League.Standings.empty());
			continue;
		}
		if (League.Division == Game::Career::CareerLeagueDivision::Amateur)
		{
			++AmateurLeagueCount;
			if (League.State == Game::Career::CareerLeagueState::Filling)
			{
				ASSERT_TRUE(League.Entrants.empty());
				ASSERT_TRUE(League.Fixtures.empty());
				ASSERT_TRUE(League.Standings.empty());
			}
			else
			{
				ASSERT_TRUE(League.State == Game::Career::CareerLeagueState::RegularSeason);
				ASSERT_EQ(static_cast<std::size_t>(20), League.Entrants.size());
				ASSERT_EQ(static_cast<std::size_t>(190), League.Fixtures.size());
				ASSERT_EQ(static_cast<std::size_t>(20), League.Standings.size());
			}
		}
		else
		{
			++DivisionLeagueCount;
			ASSERT_EQ(static_cast<std::size_t>(20), League.Entrants.size());
			ASSERT_EQ(static_cast<std::size_t>(190), League.Fixtures.size());
			ASSERT_EQ(static_cast<std::size_t>(20), League.Standings.size());
			ASSERT_TRUE(League.State == Game::Career::CareerLeagueState::RegularSeason);
		}
		for (Racer* Runner : League.Entrants)
		{
			ASSERT_TRUE(Runner != nullptr);
			ASSERT_TRUE(LeagueEntrantIds.insert(Runner->GUID.GUID).second);
		}
	}
	ASSERT_TRUE(HasAmateurCup);
	ASSERT_EQ(static_cast<std::uint64_t>(24ull * 60 * 60 * 1000), careerData.NextAmateurCupAtMs);
	ASSERT_EQ(static_cast<std::size_t>(4), DivisionLeagueCount);
	ASSERT_EQ(static_cast<std::size_t>(2), AmateurLeagueCount);
	ASSERT_EQ(static_cast<std::size_t>(100), LeagueEntrantIds.size());
	ASSERT_EQ(static_cast<std::size_t>(154), std::count_if(careerData.RacerStates.begin(),
		careerData.RacerStates.end(), [](const Game::Career::CareerRacerState& State)
		{
			return State.LeagueId == 0;
		}));
}

TEST_CASE(Career_DebugSpawnerAddsRacersToTheLiveWorld)
{
	srand(2304);
	Game::Career::CareerData careerData;
	Game::Career::CareerProfile careerProfile;
	Game::Career::CareerOrchestrator orchestrator(&careerData, &careerProfile);
	orchestrator.Init();
	const std::size_t InitialStateCount = careerData.RacerStates.size();

	ASSERT_EQ(static_cast<std::size_t>(5), orchestrator.SpawnRacers(5));

	ASSERT_EQ(static_cast<std::size_t>(261), careerData.Racers.Size());
	ASSERT_EQ(static_cast<std::size_t>(259), careerData.TargetAiPopulation);
	ASSERT_EQ(static_cast<std::size_t>(259), orchestrator.GetActiveRacerCount());
	ASSERT_EQ(InitialStateCount + 5, careerData.RacerStates.size());
	for (std::size_t Index = InitialStateCount; Index < careerData.RacerStates.size(); ++Index)
	{
		const Game::Career::CareerRacerState& State = careerData.RacerStates[Index];
		ASSERT_TRUE(State.Runner != nullptr);
		ASSERT_TRUE(!State.Runner->Name.empty());
		ASSERT_EQ(1000, State.Cash);
		ASSERT_TRUE(State.Activity == Game::Career::CareerRacerActivity::InBuilding);
	}
}

TEST_CASE(Career_WorldSimulationRecordsPerRacerRaceHistory)
{
	srand(2305);
	Game::Career::CareerData careerData;
	Game::Career::CareerProfile careerProfile;
	Game::Career::CareerOrchestrator orchestrator(&careerData, &careerProfile);
	orchestrator.Init();

	Racer* RacerWithHistory = nullptr;
	for (unsigned int UpdateIndex = 0; UpdateIndex < 120 && RacerWithHistory == nullptr; ++UpdateIndex)
	{
		orchestrator.Update(1000);
		for (const Game::Career::CareerRacerState& State : careerData.RacerStates)
			if (State.Runner != nullptr && !State.Runner->CareerRecord.History.empty())
			{
				RacerWithHistory = State.Runner;
				break;
			}
	}

	ASSERT_TRUE(RacerWithHistory != nullptr);
	ASSERT_EQ(RacerWithHistory->CareerRecord.RacesEntered, RacerWithHistory->CareerRecord.History.size());
	const Game::Race::RacerCareerHistoryEntry& HistoryEntry = RacerWithHistory->CareerRecord.History.back();
	ASSERT_TRUE(!HistoryEntry.EventName.empty());
	ASSERT_TRUE(HistoryEntry.TrackLength > 0);
	ASSERT_TRUE(HistoryEntry.WorldTimeMs > 0);
}

TEST_CASE(Career_InitialWorldMonthIsFrameSteppedAndCreatesRaceHistory)
{
	srand(2306);
	Game::Career::CareerData careerData;
	Game::Career::CareerProfile careerProfile;
	Game::Career::CareerOrchestrator orchestrator(&careerData, &careerProfile);
	orchestrator.Init();
	orchestrator.BeginInitialWorldMonthSimulation();

	ASSERT_TRUE(careerData.InitialWorldMonthSimulationInProgress);
	ASSERT_EQ(static_cast<std::uint64_t>(108000000), careerData.InitialWorldMonthTotalRealMs);
	ASSERT_TRUE(!orchestrator.AdvanceInitialWorldMonthSimulation(128));
	ASSERT_EQ(static_cast<std::uint64_t>(128000), careerData.InitialWorldMonthElapsedRealMs);
	ASSERT_TRUE(!careerData.InitialWorldMonthSimulated);

	while (!careerData.InitialWorldMonthSimulationFailed && !careerData.InitialWorldMonthSimulated)
		orchestrator.AdvanceInitialWorldMonthSimulation(2048);

	ASSERT_TRUE(careerData.InitialWorldMonthSimulated);
	ASSERT_TRUE(!careerData.InitialWorldMonthSimulationFailed);
	ASSERT_TRUE(careerData.InitialWorldMonthElapsedRealMs >= careerData.InitialWorldMonthTotalRealMs);
	ASSERT_TRUE(careerData.WorldClock.NowMs() >= static_cast<std::uint64_t>(30ull * 24 * 60 * 60 * 1000));
	ASSERT_EQ(careerData.InitialWorldMonthStartingRealMs + careerData.InitialWorldMonthElapsedRealMs,
		careerData.RealTimeMs);
	ASSERT_EQ(careerData.InitialWorldMonthStartingWorldMs + careerData.InitialWorldMonthElapsedRealMs *
		careerData.WorldClock.GetTimeScale(), careerData.WorldClock.NowMs());
	ASSERT_EQ(static_cast<std::size_t>(254), orchestrator.GetActiveRacerCount());

	std::size_t ActiveRacerCount = 0;
	std::size_t ActiveRacersWithHistory = 0;
	std::uint64_t HistoryEntryCount = 0;
	for (const Game::Career::CareerRacerState& State : careerData.RacerStates)
	{
		if (State.Activity == Game::Career::CareerRacerActivity::Retired)
			continue;
		ASSERT_TRUE(State.Runner != nullptr);
		++ActiveRacerCount;
		if (!State.Runner->CareerRecord.History.empty())
			++ActiveRacersWithHistory;
	}
	for (const Racer* Runner : careerData.Racers.GetAll())
		if (Runner != nullptr)
			HistoryEntryCount += static_cast<std::uint64_t>(Runner->CareerRecord.History.size());
	ASSERT_EQ(static_cast<std::size_t>(254), ActiveRacerCount);
	ASSERT_EQ(ActiveRacerCount, ActiveRacersWithHistory);
	ASSERT_TRUE(HistoryEntryCount > static_cast<std::uint64_t>(ActiveRacerCount));
}

TEST_CASE(Career_PlayerCanJoinAnAmateurLeagueAndRaceItsScheduledFixture)
{
	srand(2307);
	Game::Career::CareerData careerData;
	Game::Career::CareerProfile careerProfile;
	careerProfile.MainFella.Name = "Player Runner";
	Game::Career::CareerOrchestrator orchestrator(&careerData, &careerProfile);
	orchestrator.Init();

	auto AmateurLeague = std::find_if(careerData.Leagues.begin(), careerData.Leagues.end(),
		[](const Game::Career::CareerLeague& League)
		{
			return League.Division == Game::Career::CareerLeagueDivision::Amateur &&
				League.State == Game::Career::CareerLeagueState::Filling;
		});
	ASSERT_TRUE(AmateurLeague != careerData.Leagues.end());
	const std::uint64_t AmateurLeagueId = AmateurLeague->Id;
	ASSERT_TRUE(orchestrator.CanJoinLeague(AmateurLeagueId));
	ASSERT_TRUE(orchestrator.JoinLeague(AmateurLeagueId));
	ASSERT_TRUE(!orchestrator.CanJoinLeague(AmateurLeagueId));
	ASSERT_TRUE(careerData.PlayerLeagueRacer == &careerProfile.MainFella);
	ASSERT_EQ(AmateurLeagueId, careerData.PlayerLeagueId);
	ASSERT_TRUE(careerProfile.MainFella.GUID.GUID != 0);
	ASSERT_TRUE(careerData.Racers.Get(careerProfile.MainFella.GUID.GUID) == nullptr);
	ASSERT_EQ(static_cast<std::size_t>(273), careerData.TargetAiPopulation);

	AmateurLeague = std::find_if(careerData.Leagues.begin(), careerData.Leagues.end(),
		[AmateurLeagueId](const Game::Career::CareerLeague& League)
		{
			return League.Id == AmateurLeagueId;
		});
	ASSERT_TRUE(AmateurLeague != careerData.Leagues.end());
	ASSERT_EQ(static_cast<std::size_t>(20), AmateurLeague->Entrants.size());
	ASSERT_EQ(static_cast<std::size_t>(190), AmateurLeague->Fixtures.size());
	ASSERT_EQ(static_cast<std::size_t>(20), AmateurLeague->Standings.size());
	ASSERT_TRUE(AmateurLeague->State == Game::Career::CareerLeagueState::RegularSeason);

	for (Game::Career::CareerLeagueFixture& Fixture : AmateurLeague->Fixtures)
		Fixture.ScheduledAtMs = 0;
	const std::uint64_t PlayerRacerId = careerProfile.MainFella.GUID.GUID;
	const auto PlayerFixture = std::find_if(AmateurLeague->Fixtures.begin(), AmateurLeague->Fixtures.end(),
		[PlayerRacerId](const Game::Career::CareerLeagueFixture& Fixture)
		{
			return Fixture.Fixture.Round == 0 &&
				(Fixture.Fixture.FirstCompetitorId == PlayerRacerId ||
					Fixture.Fixture.SecondCompetitorId == PlayerRacerId);
		});
	ASSERT_TRUE(PlayerFixture != AmateurLeague->Fixtures.end());
	const std::size_t TrackPlotIndex = PlayerFixture->TrackPlotIndex;
	for (Game::Career::CareerRacerState& State : careerData.RacerStates)
	{
		State.Cash = 1000;
		State.Activity = Game::Career::CareerRacerActivity::InBuilding;
		State.LocationPlotIndex = TrackPlotIndex;
		State.DestinationPlotIndex = TrackPlotIndex;
	}
	ASSERT_TRUE(orchestrator.CanRaceNextPlayerLeagueMatch());

	Race PlayerLeagueRace;
	ASSERT_TRUE(orchestrator.PreparePlayerLeagueRace(PlayerLeagueRace));
	ASSERT_EQ(static_cast<std::size_t>(6), PlayerLeagueRace.Racers.size());
	ASSERT_TRUE(careerData.PendingPlayerLeagueRace.Active);
	ASSERT_EQ(static_cast<std::size_t>(3), careerData.PendingPlayerLeagueRace.LeagueFixtures.size());
	std::vector<std::size_t> FixtureIndices;
	for (const Game::Career::CareerRaceFixtureReference& Reference :
		careerData.PendingPlayerLeagueRace.LeagueFixtures)
		FixtureIndices.push_back(Reference.FixtureIndex);
	const RaceResult& Result = PlayerLeagueRace.Sim();
	ASSERT_EQ(RaceError::None, Result.Error);
	orchestrator.CompleteUpcomingRace(Result);
	ASSERT_TRUE(!careerData.PendingPlayerLeagueRace.Active);

	AmateurLeague = std::find_if(careerData.Leagues.begin(), careerData.Leagues.end(),
		[AmateurLeagueId](const Game::Career::CareerLeague& League)
		{
			return League.Id == AmateurLeagueId;
		});
	ASSERT_TRUE(AmateurLeague != careerData.Leagues.end());
	for (std::size_t FixtureIndex : FixtureIndices)
		ASSERT_TRUE(AmateurLeague->Fixtures[FixtureIndex].State == Game::Career::CareerFixtureState::Complete);
	const auto PlayerStanding = std::find_if(AmateurLeague->Standings.begin(), AmateurLeague->Standings.end(),
		[&careerProfile](const Game::Career::CareerLeagueStanding& Standing)
		{
			return Standing.Runner == &careerProfile.MainFella;
		});
	ASSERT_TRUE(PlayerStanding != AmateurLeague->Standings.end());
	ASSERT_EQ(1u, PlayerStanding->Wins + PlayerStanding->Draws + PlayerStanding->Losses);
}

TEST_CASE(Career_FullAmateurLeagueOpensAnotherLobby)
{
	srand(232);
	Game::Career::CareerData careerData;
	Game::Career::CareerProfile careerProfile;
	Game::Career::CareerOrchestrator orchestrator(&careerData, &careerProfile);
	orchestrator.Init();

	const auto AmateurLeague = std::find_if(careerData.Leagues.begin(), careerData.Leagues.end(),
		[](const Game::Career::CareerLeague& League)
		{
			return League.Division == Game::Career::CareerLeagueDivision::Amateur;
		});
	ASSERT_TRUE(AmateurLeague != careerData.Leagues.end());
	const std::uint64_t AmateurLeagueId = AmateurLeague->Id;

	const auto DivisionLeague = std::find_if(careerData.Leagues.begin(), careerData.Leagues.end(),
		[](const Game::Career::CareerLeague& League)
		{
			return League.Division == Game::Career::CareerLeagueDivision::Division1;
		});
	ASSERT_TRUE(DivisionLeague != careerData.Leagues.end());
	for (std::size_t Index = 0; Index < 2; ++Index)
	{
		Racer* Runner = DivisionLeague->Entrants[Index];
		const auto State = std::find_if(careerData.RacerStates.begin(), careerData.RacerStates.end(),
			[Runner](const Game::Career::CareerRacerState& Candidate)
			{
				return Candidate.Runner == Runner;
			});
		ASSERT_TRUE(State != careerData.RacerStates.end());
		State->Cash = 0;
	}

	orchestrator.Update(1);

	const Game::Career::CareerLeague* FilledLeague = nullptr;
	const Game::Career::CareerLeague* OpenLeague = nullptr;
	std::size_t AmateurLeagueCount = 0;
	for (const Game::Career::CareerLeague& League : careerData.Leagues)
		if (!League.IsAmateurCup && League.Division == Game::Career::CareerLeagueDivision::Amateur)
		{
			++AmateurLeagueCount;
			if (League.Id == AmateurLeagueId)
				FilledLeague = &League;
			else if (League.State == Game::Career::CareerLeagueState::Filling && League.Entrants.size() < 20)
				OpenLeague = &League;
		}

	ASSERT_EQ(static_cast<std::size_t>(3), AmateurLeagueCount);
	ASSERT_TRUE(FilledLeague != nullptr);
	ASSERT_TRUE(FilledLeague->State == Game::Career::CareerLeagueState::RegularSeason);
	ASSERT_EQ(static_cast<std::size_t>(20), FilledLeague->Entrants.size());
	ASSERT_EQ(static_cast<std::size_t>(190), FilledLeague->Fixtures.size());
	ASSERT_TRUE(OpenLeague != nullptr);
	ASSERT_TRUE(OpenLeague->Entrants.empty());
	ASSERT_TRUE(OpenLeague->Name != FilledLeague->Name);
	ASSERT_EQ(static_cast<std::size_t>(254), orchestrator.GetActiveRacerCount());
}

TEST_CASE(Career_WorldSimulationAdvancesSeveralIndependentBackgroundRaces)
{
	srand(233);
	Game::Career::CareerData careerData;
	Game::Career::CareerProfile careerProfile;
	Game::Career::CareerOrchestrator orchestrator(&careerData, &careerProfile);
	orchestrator.Init();

	std::vector<std::shared_ptr<Game::Career::CareerWorldRace>> ConcurrentRaces;
	for (unsigned int UpdateIndex = 0; UpdateIndex < 60 && ConcurrentRaces.size() < 2; ++UpdateIndex)
	{
		orchestrator.Update(1000);
		ConcurrentRaces.clear();
		std::unordered_set<Racer*> ActiveEntrants;
		for (const std::shared_ptr<Game::Career::CareerWorldRace>& WorldRace : careerData.WorldRaces)
			if (WorldRace != nullptr && !WorldRace->IsComplete &&
				WorldRace->StateMachine->Data.CurrentTick == 0)
			{
				ConcurrentRaces.push_back(WorldRace);
				ASSERT_EQ(static_cast<std::size_t>(6), WorldRace->Entrants.size());
				for (Racer* Runner : WorldRace->Entrants)
				{
					ASSERT_TRUE(Runner != nullptr);
					ASSERT_TRUE(ActiveEntrants.insert(Runner).second);
				}
			}
	}

	ASSERT_TRUE(ConcurrentRaces.size() >= 2);
	std::vector<unsigned int> PreviousTicks;
	for (const std::shared_ptr<Game::Career::CareerWorldRace>& WorldRace : ConcurrentRaces)
		PreviousTicks.push_back(WorldRace->StateMachine->Data.CurrentTick);
	const std::uint64_t PreviousRealTimeMs = careerData.RealTimeMs;
	const std::uint64_t PreviousWorldTimeMs = careerData.WorldClock.NowMs();

	orchestrator.Update(1000);
	ASSERT_EQ(PreviousRealTimeMs + 1000, careerData.RealTimeMs);
	ASSERT_EQ(PreviousWorldTimeMs + 24000, careerData.WorldClock.NowMs());
	for (std::size_t Index = 0; Index < ConcurrentRaces.size(); ++Index)
	{
		const std::uint64_t SimulatedRaceMs =
			ConcurrentRaces[Index]->StateMachine->Data.CurrentTick - PreviousTicks[Index] +
			ConcurrentRaces[Index]->TickAccumulatorMs;
		ASSERT_EQ(static_cast<std::uint64_t>(1000), SimulatedRaceMs);
	}
}

TEST_CASE(Career_WorldSimulationExceedsFormerPerTrackRaceLimitWithSixRunners)
{
	srand(234);
	Game::Career::CareerData careerData;
	Game::Career::CareerProfile careerProfile;
	Game::Career::CareerOrchestrator orchestrator(&careerData, &careerProfile);
	orchestrator.Init();

	std::size_t TrackPlotIndex = careerData.CityPlots.size();
	for (std::size_t Index = 0; Index < careerData.CityPlots.size(); ++Index)
		if (careerData.CityPlots[Index].Type == Game::Career::CityPlotType::RaceTrack)
		{
			TrackPlotIndex = Index;
			break;
		}
	ASSERT_TRUE(TrackPlotIndex < careerData.CityPlots.size());

	for (Game::Career::CareerRacerState& State : careerData.RacerStates)
	{
		State.Activity = Game::Career::CareerRacerActivity::InBuilding;
		State.LocationPlotIndex = TrackPlotIndex;
		State.DestinationPlotIndex = TrackPlotIndex;
		State.ActionEndsAtRealMs = 0;
		State.Runner->Conditions.Fatigue = 0.0;
		State.Runner->Conditions.Injury = 0.0;
	}
	for (Game::Career::CareerLeague& League : careerData.Leagues)
		if (League.State == Game::Career::CareerLeagueState::RegularSeason)
			for (Game::Career::CareerLeagueFixture& Fixture : League.Fixtures)
			{
				Fixture.TrackPlotIndex = TrackPlotIndex;
				Fixture.ScheduledAtMs = Fixture.Fixture.Round == 0
					? 0 : std::numeric_limits<std::uint64_t>::max();
			}
	careerData.NextOpenRaceAtMs = std::numeric_limits<std::uint64_t>::max();

	orchestrator.Update(1);

	std::size_t ActiveRaceCount = 0;
	std::unordered_set<Racer*> ActiveEntrants;
	for (const std::shared_ptr<Game::Career::CareerWorldRace>& WorldRace : careerData.WorldRaces)
		if (WorldRace != nullptr && !WorldRace->IsComplete && WorldRace->TrackPlotIndex == TrackPlotIndex)
		{
			++ActiveRaceCount;
			ASSERT_EQ(static_cast<std::size_t>(6), WorldRace->Entrants.size());
			for (Racer* Runner : WorldRace->Entrants)
				ASSERT_TRUE(ActiveEntrants.insert(Runner).second);
		}
	ASSERT_TRUE(ActiveRaceCount > 8);
}

TEST_CASE(Career_PlayoffRaceOnlyAdvancesItsBracketCompetitors)
{
	srand(235);
	Game::Career::CareerData careerData;
	Game::Career::CareerProfile careerProfile;
	Game::Career::CareerOrchestrator orchestrator(&careerData, &careerProfile);
	orchestrator.Init();

	const auto League = std::find_if(careerData.Leagues.begin(), careerData.Leagues.end(),
		[](const Game::Career::CareerLeague& Candidate)
		{
			return Candidate.Division == Game::Career::CareerLeagueDivision::Division1;
		});
	ASSERT_TRUE(League != careerData.Leagues.end());
	const std::uint64_t LeagueId = League->Id;
	Racer* FirstCompetitor = League->Entrants[0];
	Racer* SecondCompetitor = League->Entrants[1];
	const std::uint64_t FirstCompetitorId = FirstCompetitor->GUID.GUID;
	const std::uint64_t SecondCompetitorId = SecondCompetitor->GUID.GUID;
	League->Playoffs = SportRaceCore::GeneratePlayoffBracket({ FirstCompetitorId, SecondCompetitorId });
	League->State = Game::Career::CareerLeagueState::Playoffs;
	ASSERT_EQ(static_cast<std::size_t>(1), League->Playoffs.Rounds.size());
	ASSERT_EQ(static_cast<std::size_t>(1), League->Playoffs.Rounds[0].Matches.size());

	std::vector<std::size_t> Tracks;
	for (std::size_t Index = 0; Index < careerData.CityPlots.size(); ++Index)
		if (careerData.CityPlots[Index].Type == Game::Career::CityPlotType::RaceTrack)
			Tracks.push_back(Index);
	ASSERT_TRUE(!Tracks.empty());
	const std::size_t TrackIndex = Tracks[LeagueId % Tracks.size()];
	careerData.CityPlots[TrackIndex].TrackLength =
		static_cast<unsigned int>(Game::Race::TrackLength::Standard100m);

	for (Game::Career::CareerRacerState& State : careerData.RacerStates)
	{
		const bool IsMatchCompetitor = State.Runner == FirstCompetitor || State.Runner == SecondCompetitor;
		const double Skill = IsMatchCompetitor ? 1.0 : 100.0;
		ConfigureRacer(*State.Runner, State.Runner->GUID.GUID, Skill, Skill, Skill, Skill,
			IsMatchCompetitor ? 0.0 : 1.0, IsMatchCompetitor ? 0.0 : 1.0);
		State.Activity = Game::Career::CareerRacerActivity::InBuilding;
		State.LocationPlotIndex = TrackIndex;
		State.DestinationPlotIndex = TrackIndex;
		State.ActionEndsAtRealMs = 0;
		State.Cash = std::max(State.Cash, 1000);
	}
	for (Game::Career::CareerLeague& CandidateLeague : careerData.Leagues)
		if (CandidateLeague.Id != LeagueId)
			for (Game::Career::CareerLeagueFixture& Fixture : CandidateLeague.Fixtures)
				Fixture.ScheduledAtMs = std::numeric_limits<std::uint64_t>::max();
	careerData.NextOpenRaceAtMs = std::numeric_limits<std::uint64_t>::max();

	orchestrator.Update(1);

	std::shared_ptr<Game::Career::CareerWorldRace> PlayoffRace;
	for (const std::shared_ptr<Game::Career::CareerWorldRace>& WorldRace : careerData.WorldRaces)
		if (WorldRace != nullptr && WorldRace->IsPlayoff && WorldRace->LeagueId == LeagueId)
			PlayoffRace = WorldRace;
	ASSERT_TRUE(PlayoffRace != nullptr);
	ASSERT_EQ(static_cast<std::size_t>(6), PlayoffRace->Entrants.size());

	for (unsigned int UpdateIndex = 0; UpdateIndex < 120 && !PlayoffRace->IsComplete; ++UpdateIndex)
		orchestrator.Update(1000);
	ASSERT_TRUE(PlayoffRace->IsComplete);

	const RaceResult& Result = PlayoffRace->StateMachine->Data.ThisRace.Result;
	const auto OverallWinner = std::find_if(Result.RacerResults.begin(), Result.RacerResults.end(),
		[](const RacerRaceResult& RacerResult)
		{
			return RacerResult.Position == FirstPlaceRank;
		});
	ASSERT_TRUE(OverallWinner != Result.RacerResults.end());
	ASSERT_TRUE(OverallWinner->Racer != FirstCompetitor && OverallWinner->Racer != SecondCompetitor);
	const auto UpdatedLeague = std::find_if(careerData.Leagues.begin(), careerData.Leagues.end(),
		[LeagueId](const Game::Career::CareerLeague& Candidate)
		{
			return Candidate.Id == LeagueId;
		});
	ASSERT_TRUE(UpdatedLeague != careerData.Leagues.end());
	ASSERT_EQ(static_cast<std::size_t>(1), UpdatedLeague->Playoffs.Rounds.size());
	ASSERT_EQ(static_cast<std::size_t>(1), UpdatedLeague->Playoffs.Rounds[0].Matches.size());
	const SportRaceCore::PlayoffMatch& Match = UpdatedLeague->Playoffs.Rounds[0].Matches[0];
	ASSERT_TRUE(Match.IsComplete);
	ASSERT_TRUE(Match.WinnerId == FirstCompetitorId || Match.WinnerId == SecondCompetitorId);
}

TEST_CASE(Career_PlayoffWithBothCompetitorsUnavailableAdvancesHigherSeed)
{
	srand(2351);
	Game::Career::CareerData careerData;
	Game::Career::CareerProfile careerProfile;
	Game::Career::CareerOrchestrator orchestrator(&careerData, &careerProfile);
	orchestrator.Init();

	auto League = std::find_if(careerData.Leagues.begin(), careerData.Leagues.end(),
		[](const Game::Career::CareerLeague& Candidate)
		{
			return !Candidate.IsAmateurCup &&
				Candidate.Division == Game::Career::CareerLeagueDivision::Division1;
		});
	ASSERT_TRUE(League != careerData.Leagues.end());
	const std::uint64_t LeagueId = League->Id;
	Racer* HigherSeed = League->Entrants[0];
	Racer* LowerSeed = League->Entrants[1];
	const std::uint64_t HigherSeedId = HigherSeed->GUID.GUID;
	League->Playoffs = SportRaceCore::GeneratePlayoffBracket({ HigherSeedId, LowerSeed->GUID.GUID });
	League->State = Game::Career::CareerLeagueState::Playoffs;
	for (Game::Career::CareerLeague& Candidate : careerData.Leagues)
		for (Game::Career::CareerLeagueFixture& Fixture : Candidate.Fixtures)
			Fixture.ScheduledAtMs = std::numeric_limits<std::uint64_t>::max();
	for (Game::Career::CareerRacerState& State : careerData.RacerStates)
		if (State.Runner == HigherSeed || State.Runner == LowerSeed)
		{
			State.Activity = Game::Career::CareerRacerActivity::InRace;
			State.Cash = 0;
		}
	careerData.NextOpenRaceAtMs = std::numeric_limits<std::uint64_t>::max();

	orchestrator.Update(1);

	const auto UpdatedLeague = std::find_if(careerData.Leagues.begin(), careerData.Leagues.end(),
		[LeagueId](const Game::Career::CareerLeague& Candidate)
		{
			return Candidate.Id == LeagueId;
		});
	ASSERT_TRUE(UpdatedLeague != careerData.Leagues.end());
	ASSERT_TRUE(UpdatedLeague->Playoffs.IsComplete);
	ASSERT_TRUE(UpdatedLeague->State == Game::Career::CareerLeagueState::Complete);
	ASSERT_EQ(HigherSeedId, UpdatedLeague->Playoffs.WinnerId);
}

TEST_CASE(Career_AmateurCupStartsDailyAsPlayoffOnlyTournament)
{
	srand(2352);
	Game::Career::CareerData careerData;
	Game::Career::CareerProfile careerProfile;
	Game::Career::CareerOrchestrator orchestrator(&careerData, &careerProfile);
	orchestrator.Init();

	for (Game::Career::CareerRacerState& State : careerData.RacerStates)
	{
		State.Activity = Game::Career::CareerRacerActivity::Training;
		State.ActionEndsAtRealMs = std::numeric_limits<std::uint64_t>::max();
	}
	for (Game::Career::CareerLeague& League : careerData.Leagues)
		for (Game::Career::CareerLeagueFixture& Fixture : League.Fixtures)
			Fixture.ScheduledAtMs = std::numeric_limits<std::uint64_t>::max();
	careerData.NextAmateurCupAtMs = 0;
	careerData.NextOpenRaceAtMs = std::numeric_limits<std::uint64_t>::max();

	orchestrator.Update(1);

	const auto Cup = std::find_if(careerData.Leagues.begin(), careerData.Leagues.end(),
		[](const Game::Career::CareerLeague& League) { return League.IsAmateurCup; });
	ASSERT_TRUE(Cup != careerData.Leagues.end());
	ASSERT_EQ(static_cast<unsigned int>(1), Cup->Season);
	ASSERT_TRUE(Cup->State == Game::Career::CareerLeagueState::Playoffs);
	ASSERT_TRUE(Cup->Fixtures.empty());
	ASSERT_TRUE(Cup->Standings.empty());
	ASSERT_TRUE(Cup->Entrants.size() >= 100);
	ASSERT_EQ(Cup->Entrants.size(), Cup->Playoffs.EntrantCount);
	ASSERT_TRUE(!Cup->Playoffs.Rounds.empty());
	ASSERT_TRUE(careerData.NextAmateurCupAtMs > careerData.WorldClock.NowMs());
	ASSERT_TRUE(!orchestrator.CanJoinLeague(Cup->Id));
}

TEST_CASE(Career_CompletedSeasonsPromoteRelegateAndRegenerateFixtures)
{
	srand(2353);
	Game::Career::CareerData careerData;
	Game::Career::CareerProfile careerProfile;
	Game::Career::CareerOrchestrator orchestrator(&careerData, &careerProfile);
	orchestrator.Init();

	auto FindLeague = [&careerData](Game::Career::CareerLeagueDivision Division)
	{
		return std::find_if(careerData.Leagues.begin(), careerData.Leagues.end(),
			[Division](const Game::Career::CareerLeague& League)
			{
				return !League.IsAmateurCup && League.Division == Division;
			});
	};
	auto Division1 = FindLeague(Game::Career::CareerLeagueDivision::Division1);
	auto Division2 = FindLeague(Game::Career::CareerLeagueDivision::Division2);
	auto Division3 = FindLeague(Game::Career::CareerLeagueDivision::Division3);
	auto Division4 = FindLeague(Game::Career::CareerLeagueDivision::Division4);
	auto Amateur = FindLeague(Game::Career::CareerLeagueDivision::Amateur);
	ASSERT_TRUE(Division1 != careerData.Leagues.end());
	ASSERT_TRUE(Division2 != careerData.Leagues.end());
	ASSERT_TRUE(Division3 != careerData.Leagues.end());
	ASSERT_TRUE(Division4 != careerData.Leagues.end());
	ASSERT_TRUE(Amateur != careerData.Leagues.end());
	const std::uint64_t Division1Id = Division1->Id;
	const std::uint64_t Division2Id = Division2->Id;
	const std::uint64_t Division3Id = Division3->Id;
	const std::uint64_t Division4Id = Division4->Id;
	const unsigned int Division1Season = Division1->Season;
	const unsigned int Division2Season = Division2->Season;
	const unsigned int Division3Season = Division3->Season;
	const unsigned int Division4Season = Division4->Season;
	Racer* Division1Relegated = Division1->Standings[18].Runner;
	Racer* Division2Promoted = Division2->Standings[0].Runner;
	Racer* Division4Relegated = Division4->Standings[19].Runner;
	Racer* AmateurPromotedA = Amateur->Standings[0].Runner;
	Racer* AmateurPromotedB = Amateur->Standings[1].Runner;
	std::size_t OpenAmateurCandidates = 0;
	for (Game::Career::CareerRacerState& State : careerData.RacerStates)
	{
		State.ActionEndsAtRealMs = std::numeric_limits<std::uint64_t>::max();
		State.Cash = 1000;
		const bool IsOpenAmateurCandidate = State.Runner != nullptr &&
			State.Activity != Game::Career::CareerRacerActivity::Retired && State.LeagueId == 0 &&
			OpenAmateurCandidates < static_cast<std::size_t>(20);
		State.Activity = IsOpenAmateurCandidate ? Game::Career::CareerRacerActivity::InBuilding :
			Game::Career::CareerRacerActivity::Training;
		if (IsOpenAmateurCandidate)
			++OpenAmateurCandidates;
	}
	ASSERT_EQ(static_cast<std::size_t>(20), OpenAmateurCandidates);
	for (Game::Career::CareerLeague& League : careerData.Leagues)
	{
		if (League.IsAmateurCup || League.State == Game::Career::CareerLeagueState::Filling)
			continue;
		League.State = Game::Career::CareerLeagueState::Complete;
		League.Playoffs.IsComplete = true;
		for (Game::Career::CareerLeagueFixture& Fixture : League.Fixtures)
			Fixture.State = Game::Career::CareerFixtureState::Complete;
		for (std::size_t Index = 0; Index < League.Standings.size(); ++Index)
			League.Standings[Index].Points = static_cast<unsigned int>(League.Standings.size() - Index);
		if (League.Division == Game::Career::CareerLeagueDivision::Amateur)
			for (Game::Career::CareerLeagueStanding& Standing : League.Standings)
				for (Game::Career::CareerRacerState& State : careerData.RacerStates)
					if (State.Runner == Standing.Runner)
						State.HasRacingLicense = false;
	}
	for (Game::Career::CareerRacerState& State : careerData.RacerStates)
		if (State.Runner == AmateurPromotedA || State.Runner == AmateurPromotedB)
			State.HasRacingLicense = true;
	careerData.NextOpenRaceAtMs = std::numeric_limits<std::uint64_t>::max();
	careerData.NextAmateurCupAtMs = std::numeric_limits<std::uint64_t>::max();

	orchestrator.Update(1);

	auto FindLeagueById = [&careerData](std::uint64_t Id)
	{
		return std::find_if(careerData.Leagues.begin(), careerData.Leagues.end(),
			[Id](const Game::Career::CareerLeague& League) { return League.Id == Id; });
	};
	Division1 = FindLeagueById(Division1Id);
	Division2 = FindLeagueById(Division2Id);
	Division3 = FindLeagueById(Division3Id);
	Division4 = FindLeagueById(Division4Id);
	ASSERT_TRUE(Division1 != careerData.Leagues.end());
	ASSERT_TRUE(Division2 != careerData.Leagues.end());
	ASSERT_TRUE(Division3 != careerData.Leagues.end());
	ASSERT_TRUE(Division4 != careerData.Leagues.end());
	for (const auto* League : { &*Division1, &*Division2, &*Division3, &*Division4 })
	{
		ASSERT_TRUE(League->State == Game::Career::CareerLeagueState::RegularSeason);
		ASSERT_EQ(static_cast<std::size_t>(190), League->Fixtures.size());
		ASSERT_EQ(static_cast<std::size_t>(20), League->Standings.size());
	}
	ASSERT_EQ(Division1Season + 1, Division1->Season);
	ASSERT_EQ(Division2Season + 1, Division2->Season);
	ASSERT_EQ(Division3Season + 1, Division3->Season);
	ASSERT_EQ(Division4Season + 1, Division4->Season);
	ASSERT_TRUE(std::find(Division1->Entrants.begin(), Division1->Entrants.end(), Division2Promoted) !=
		Division1->Entrants.end());
	ASSERT_TRUE(std::find(Division1->Entrants.begin(), Division1->Entrants.end(), Division1Relegated) ==
		Division1->Entrants.end());
	ASSERT_TRUE(std::find(Division2->Entrants.begin(), Division2->Entrants.end(), Division1Relegated) !=
		Division2->Entrants.end());
	ASSERT_TRUE(std::find(Division4->Entrants.begin(), Division4->Entrants.end(), AmateurPromotedA) !=
		Division4->Entrants.end());
	ASSERT_TRUE(std::find(Division4->Entrants.begin(), Division4->Entrants.end(), AmateurPromotedB) !=
		Division4->Entrants.end());
	const bool RelegatedIntoAmateur = std::any_of(careerData.Leagues.begin(), careerData.Leagues.end(),
		[Division4Relegated](const Game::Career::CareerLeague& League)
		{
			return !League.IsAmateurCup && League.Division == Game::Career::CareerLeagueDivision::Amateur &&
				std::find(League.Entrants.begin(), League.Entrants.end(), Division4Relegated) != League.Entrants.end();
		});
	ASSERT_TRUE(RelegatedIntoAmateur);
}

TEST_CASE(Competition_GeneratesEveryRoundRobinPairOnce)
{
	const std::vector<std::uint64_t> CompetitorIds{ 11, 12, 13, 14, 15 };
	const std::vector<SportRaceCore::LeagueFixture> Fixtures =
		SportRaceCore::GenerateRoundRobinFixtures(CompetitorIds);
	ASSERT_EQ(static_cast<std::size_t>(10), Fixtures.size());

	std::unordered_set<std::uint64_t> FixturePairs;
	std::vector<std::size_t> FixturesPerRound(5, 0);
	for (const SportRaceCore::LeagueFixture& Fixture : Fixtures)
	{
		const std::uint64_t First = std::min(Fixture.FirstCompetitorId, Fixture.SecondCompetitorId);
		const std::uint64_t Second = std::max(Fixture.FirstCompetitorId, Fixture.SecondCompetitorId);
		ASSERT_TRUE(FixturePairs.insert(First * 100 + Second).second);
		ASSERT_TRUE(Fixture.Round < FixturesPerRound.size());
		++FixturesPerRound[Fixture.Round];
	}
	ASSERT_EQ(static_cast<std::size_t>(10), FixturePairs.size());
	for (const std::size_t RoundFixtureCount : FixturesPerRound)
		ASSERT_EQ(static_cast<std::size_t>(2), RoundFixtureCount);
}

TEST_CASE(Competition_GeneratesAndAdvancesAnUnevenPlayoffBracket)
{
	const std::vector<std::uint64_t> CompetitorIds{ 101, 102, 103, 104, 105 };
	SportRaceCore::PlayoffBracket Bracket = SportRaceCore::GeneratePlayoffBracket(CompetitorIds);
	ASSERT_EQ(static_cast<std::size_t>(5), Bracket.EntrantCount);
	ASSERT_EQ(static_cast<std::size_t>(3), Bracket.Rounds.size());
	ASSERT_EQ(static_cast<std::size_t>(4), Bracket.Rounds.front().Matches.size());

	std::unordered_set<std::uint64_t> FirstRoundEntrants;
	for (const SportRaceCore::PlayoffMatch& Match : Bracket.Rounds.front().Matches)
		for (const std::optional<std::uint64_t>& CompetitorId : { Match.CompetitorAId, Match.CompetitorBId })
			if (CompetitorId.has_value() && *CompetitorId != 0)
				ASSERT_TRUE(FirstRoundEntrants.insert(*CompetitorId).second);
	ASSERT_EQ(CompetitorIds.size(), FirstRoundEntrants.size());

	for (std::size_t RoundIndex = 0; RoundIndex < Bracket.Rounds.size(); ++RoundIndex)
		for (std::size_t MatchIndex = 0; MatchIndex < Bracket.Rounds[RoundIndex].Matches.size(); ++MatchIndex)
		{
			const SportRaceCore::PlayoffMatch& Match = Bracket.Rounds[RoundIndex].Matches[MatchIndex];
			if (!Match.IsComplete && Match.CompetitorAId.has_value() && Match.CompetitorBId.has_value() &&
				*Match.CompetitorAId != 0 && *Match.CompetitorBId != 0)
				ASSERT_TRUE(SportRaceCore::SetPlayoffMatchWinner(Bracket, RoundIndex, MatchIndex,
					*Match.CompetitorAId));
		}
	ASSERT_TRUE(Bracket.IsComplete);
	ASSERT_TRUE(std::find(CompetitorIds.begin(), CompetitorIds.end(), Bracket.WinnerId) != CompetitorIds.end());
}

TEST_CASE(WorldClock_AppliesTimeScaleAndRejectsOverflow)
{
	SportRaceCore::WorldClock Clock(10);
	ASSERT_TRUE(Clock.Advance(123));
	ASSERT_EQ(static_cast<std::uint64_t>(1230), Clock.NowMs());
	ASSERT_FALSE(Clock.Advance(std::numeric_limits<std::uint64_t>::max()));
	ASSERT_EQ(static_cast<std::uint64_t>(1230), Clock.NowMs());
}

TEST_CASE(WorldClock_DefaultScaleMapsOneRealHourToOneWorldDay)
{
	SportRaceCore::WorldClock Clock;
	constexpr std::uint64_t RealHourMs = 60 * 60 * 1000;
	constexpr std::uint64_t WorldDayMs = 24 * RealHourMs;
	ASSERT_EQ(static_cast<std::uint32_t>(24), Clock.GetTimeScale());
	ASSERT_TRUE(Clock.Advance(RealHourMs));
	ASSERT_EQ(WorldDayMs, Clock.NowMs());
}

TEST_CASE(Career_WorldTrainingDurationUsesRealTime)
{
	srand(234);
	Game::Career::CareerData careerData;
	Game::Career::CareerProfile careerProfile;
	Game::Career::CareerOrchestrator orchestrator(&careerData, &careerProfile);
	orchestrator.Init();

	Game::Career::CareerRacerState& TrainingState = careerData.RacerStates.back();
	Racer* TrainingRunner = TrainingState.Runner;
	ASSERT_TRUE(TrainingRunner != nullptr);
	const auto Gym = std::find_if(careerData.CityPlots.begin(), careerData.CityPlots.end(),
		[](const Game::Career::CityPlot& Plot)
		{
			return Plot.Type == Game::Career::CityPlotType::Gym;
		});
	ASSERT_TRUE(Gym != careerData.CityPlots.end());
	TrainingState.LocationPlotIndex = static_cast<std::size_t>(Gym - careerData.CityPlots.begin());
	TrainingState.Activity = Game::Career::CareerRacerActivity::Training;
	TrainingState.ActionEndsAtRealMs = 20000;
	const int StartingCash = TrainingState.Cash;

	orchestrator.Update(19999);
	ASSERT_EQ(static_cast<std::uint64_t>(19999), careerData.RealTimeMs);
	ASSERT_EQ(static_cast<std::uint64_t>(19999 * 24), careerData.WorldClock.NowMs());
	const auto StillTraining = std::find_if(careerData.RacerStates.begin(), careerData.RacerStates.end(),
		[TrainingRunner](const Game::Career::CareerRacerState& State)
		{
			return State.Runner == TrainingRunner;
		});
	ASSERT_TRUE(StillTraining != careerData.RacerStates.end());
	ASSERT_TRUE(StillTraining->Activity == Game::Career::CareerRacerActivity::Training);

	orchestrator.Update(1);
	const auto TrainingComplete = std::find_if(careerData.RacerStates.begin(), careerData.RacerStates.end(),
		[TrainingRunner](const Game::Career::CareerRacerState& State)
		{
			return State.Runner == TrainingRunner;
		});
	ASSERT_TRUE(TrainingComplete != careerData.RacerStates.end());
	ASSERT_TRUE(TrainingComplete->Activity == Game::Career::CareerRacerActivity::Resting);
	ASSERT_EQ(StartingCash - 10, TrainingComplete->Cash);
}

TEST_CASE(Career_SelectMatchedOpponentsPrioritizesSimilarPositionPoints)
{
	srand(232);
	Game::Career::CareerData careerData;
	Game::Career::CareerProfile careerProfile;
	Game::Career::CareerOrchestrator orchestrator(&careerData, &careerProfile);
	careerProfile.MainFella.CareerRecord.PositionPoints = 10;
	careerData.Racers.Make("False Favorite")->CareerRecord.Wins = 1000;
	careerData.Racers.Make("Eight Points")->CareerRecord.PositionPoints = 8;
	careerData.Racers.Make("Nine Points")->CareerRecord.PositionPoints = 9;
	careerData.Racers.Make("Ten Points")->CareerRecord.PositionPoints = 10;
	careerData.Racers.Make("Eleven Points")->CareerRecord.PositionPoints = 11;
	careerData.Racers.Make("Twelve Points")->CareerRecord.PositionPoints = 12;

	const std::vector<Racer*> Opponents = orchestrator.SelectMatchedOpponents(2, 92);

	ASSERT_EQ(static_cast<std::size_t>(2), Opponents.size());
	ASSERT_EQ(static_cast<std::uint64_t>(10), Opponents[0]->CareerRecord.PositionPoints);
	ASSERT_TRUE(Opponents[1]->CareerRecord.PositionPoints == 9 ||
		Opponents[1]->CareerRecord.PositionPoints == 11);
}

TEST_CASE(Career_WorldSimulationReplacesBankruptRacersOneForOne)
{
	srand(24);
	Game::Career::CareerData careerData;
	Game::Career::CareerProfile careerProfile;
	Game::Career::CareerOrchestrator orchestrator(&careerData, &careerProfile);
	orchestrator.Init();
	const auto AmateurLeague = std::find_if(careerData.Leagues.begin(), careerData.Leagues.end(),
		[](const Game::Career::CareerLeague& League)
		{
			return League.Division == Game::Career::CareerLeagueDivision::Amateur &&
				League.State == Game::Career::CareerLeagueState::RegularSeason && League.Entrants.size() == 20;
		});
	ASSERT_TRUE(AmateurLeague != careerData.Leagues.end());
	for (Racer* Runner : AmateurLeague->Entrants)
	{
		const auto State = std::find_if(careerData.RacerStates.begin(), careerData.RacerStates.end(),
			[Runner](const Game::Career::CareerRacerState& Candidate) { return Candidate.Runner == Runner; });
		ASSERT_TRUE(State != careerData.RacerStates.end());
		State->Cash = 0;
	}

	for (unsigned int UpdateIndex = 0; UpdateIndex < 60; ++UpdateIndex)
		orchestrator.Update(1000);

	const std::size_t RetiredCount = static_cast<std::size_t>(std::count_if(
		careerData.RacerStates.begin(), careerData.RacerStates.end(),
		[](const Game::Career::CareerRacerState& State)
		{
			return State.Activity == Game::Career::CareerRacerActivity::Retired;
		}));
	ASSERT_TRUE(RetiredCount > 0);
	ASSERT_EQ(careerData.TargetAiPopulation, orchestrator.GetActiveRacerCount());
	ASSERT_EQ(careerData.TargetAiPopulation + RetiredCount, careerData.RacerStates.size());
	ASSERT_EQ(static_cast<std::uint64_t>(1440000), careerData.WorldClock.NowMs());
	ASSERT_TRUE(!careerData.WorldRaces.empty());
}

TEST_CASE(Race_GetByRankReturnsLeaderFirst)
{
	srand(3);
	Racer first("First");
	Racer second("Second");
	Racer third("Third");
	first.GUID = 1;
	second.GUID = 2;
	third.GUID = 3;
	first.Pos.X = 25;
	second.Pos.X = 100;
	third.Pos.X = 50;

	Race race;
	race.Racers.push_back(&first);
	race.Racers.push_back(&second);
	race.Racers.push_back(&third);

	ASSERT_TRUE(race.GetByRank(1) == &second);
	ASSERT_TRUE(race.GetByRank(2) == &third);
	ASSERT_TRUE(race.GetByRank(3) == &first);
	ASSERT_TRUE(race.GetByRank(0) == nullptr);
	ASSERT_TRUE(race.GetByRank(4) == nullptr);
}

TEST_CASE(Race_TickPlaysBackPrecomputedFinish)
{
	srand(4);
	Racer racer("Winner");
	ConfigureRacer(racer, 10);

	Race race;
	ASSERT_TRUE(ConfigureRace(race, 4, 1000));
	race.Racers.push_back(&racer);

	auto status = race.Tick(100000, RaceStatus::Racing);

	ASSERT_TRUE(status == RaceStatus::Finished);
	ASSERT_TRUE(race.IsComplete());
	ASSERT_EQ(1u, race.FinishedCount());
	ASSERT_TRUE(race.HasFinished(&racer));
	ASSERT_EQ(1u, race.Result.RacerResults[0].Position);
	ASSERT_EQ(1000u, static_cast<unsigned int>(racer.Pos.X));
	ASSERT_TRUE(race.Result.RacerResults[0].Ms > 0);
}

TEST_CASE(Race_PrecomputeLocksEntrantsTrackAndSeed)
{
	srand(41);
	Racer racer("Locked Racer");
	ConfigureRacer(racer, 41);
	Race race;
	ASSERT_TRUE(ConfigureRace(race, 42, 1000));
	race.Racers.push_back(&racer);
	ASSERT_EQ(RaceError::None, race.Precompute());
	RacerGUID lockedGuid = racer.GUID;

	race.ThisTrack->Length = static_cast<TrackLength>(2000);
	Racer lateEntrant("Late Entrant");
	ConfigureRacer(lateEntrant, 44);
	race.Racers.push_back(&lateEntrant);

	ASSERT_EQ(1000u, race.GetTrackLength());
	ASSERT_EQ(static_cast<std::size_t>(1), race.GetEntrants().size());
	ASSERT_TRUE(race.Get(lockedGuid) == &racer);
	ASSERT_TRUE(race.GetByRank(2) == nullptr);

	ASSERT_EQ(RaceError::SeedLocked, race.SetSeed(99));

	ASSERT_TRUE(race.Tick(100000, RaceStatus::Racing) == RaceStatus::Finished);
	ASSERT_EQ(1000, racer.Pos.X);
	ASSERT_EQ(1u, race.FinishedCount());
	ASSERT_TRUE(race.HasFinished());
}

TEST_CASE(Race_PrecomputeReportsInvalidTrack)
{
	Race race;
	std::shared_ptr<Track> OriginalTrack = race.ThisTrack;
	race.ThisTrack = nullptr;
	ASSERT_EQ(RaceError::MissingTrack, race.Precompute());
	ASSERT_EQ(RaceError::MissingTrack, race.Result.Error);

	race.ThisTrack = OriginalTrack;
	race.ThisTrack->Length = static_cast<TrackLength>(0);
	ASSERT_EQ(RaceError::ZeroLengthTrack, race.Precompute());
	ASSERT_EQ(RaceError::ZeroLengthTrack, race.Result.Error);

	race.ThisTrack->Length = TrackLength::Standard100m;
	ASSERT_EQ(RaceError::None, race.Precompute());
}

TEST_CASE(Race_TimelineContainsOrderedPhasesAndEvents)
{
	constexpr unsigned int TimelineConstructorSeed = 73;
	constexpr unsigned int TimelineRacerGuid = 73;
	constexpr unsigned int TimelineRaceSeed = 74;
	constexpr unsigned int TimelineTrackLength = 5000;
	constexpr double MaximumTestRating = 100.0;
	constexpr double TimelineRunnerEndurance = 40.0;
	constexpr double TimelineRunnerSpeed = 40.0;
	srand(TimelineConstructorSeed);
	Racer runner("Timeline Runner");
	ConfigureRacer(runner, TimelineRacerGuid, MaximumTestRating, MaximumTestRating,
		TimelineRunnerEndurance, TimelineRunnerSpeed);
	runner.Skills.Composure = 100.0;
	runner.Conditions.Confidence = 1.0;
	Race race;
	ASSERT_TRUE(ConfigureRace(race, TimelineRaceSeed, TimelineTrackLength));
	race.Racers.push_back(&runner);
	ASSERT_EQ(RaceError::None, race.Precompute());

	const RacerRaceTimeline* Timeline = race.GetTimelineFor(&runner);
	ASSERT_TRUE(Timeline != nullptr);
	auto Node = Timeline->Nodes.begin();
	ASSERT_TRUE(Node != Timeline->Nodes.end());
	ASSERT_EQ(RaceTimelineNodeType::InitialDelay, Node->Type);
	++Node;
	ASSERT_TRUE(Node != Timeline->Nodes.end());
	ASSERT_EQ(RaceTimelineNodeType::AccelerationToTopSpeed, Node->Type);
	ASSERT_TRUE(std::any_of(Timeline->Nodes.begin(), Timeline->Nodes.end(), [](const RaceTimelineNode& Entry)
	{
		return Entry.Type == RaceTimelineNodeType::RandomEvent;
	}));
	ASSERT_TRUE(std::any_of(Timeline->Nodes.begin(), Timeline->Nodes.end(), [](const RaceTimelineNode& Entry)
	{
		return Entry.Type == RaceTimelineNodeType::Finish;
	}));

	double PreviousTimeMs = 0.0;
	for (const RaceTimelineNode& Entry : Timeline->Nodes)
	{
		ASSERT_TRUE(Entry.StartTimeMs >= PreviousTimeMs);
		PreviousTimeMs = Entry.StartTimeMs;
	}

	ASSERT_EQ(RaceError::None, race.Sim().Error);
	const auto VisibleEvents = race.GetEventsThroughPlaybackTime();
	const auto TimelineEventCount = std::count_if(Timeline->Nodes.begin(), Timeline->Nodes.end(),
		[](const RaceTimelineNode& Entry)
		{
			return Entry.Type == RaceTimelineNodeType::RandomEvent || Entry.Type == RaceTimelineNodeType::Gassing;
		});
	ASSERT_EQ(TimelineEventCount, VisibleEvents.size());
}

TEST_CASE(Race_MotionSegmentsAreSparseAndSupportArbitraryPositionQueries)
{
	constexpr unsigned int RacerGuid = 211;
	constexpr unsigned int RaceSeed = 212;
	constexpr unsigned int TrackLength = 5000;
	srand(RacerGuid);
	Racer runner("Sparse Timeline Runner");
	ConfigureRacer(runner, RacerGuid);
	Race race;
	ASSERT_TRUE(ConfigureRace(race, RaceSeed, TrackLength));
	race.Racers.push_back(&runner);
	ASSERT_EQ(RaceError::None, race.Precompute());

	const RacerRaceTimeline* Timeline = race.GetTimelineFor(&runner);
	ASSERT_TRUE(Timeline != nullptr);
	ASSERT_TRUE(!Timeline->MotionSegments.empty());
	ASSERT_TRUE(Timeline->MotionSegments.size() <
		static_cast<std::size_t>(Timeline->FinishTimeMs / 50.0));

	double PreviousSegmentEndMs = 0.0;
	for (const RaceMotionSegment& Segment : Timeline->MotionSegments)
	{
		ASSERT_EQ(PreviousSegmentEndMs, Segment.StartTimeMs);
		ASSERT_TRUE(Segment.EndTimeMs > Segment.StartTimeMs);
		PreviousSegmentEndMs = Segment.EndTimeMs;
	}
	ASSERT_EQ(Timeline->FinishTimeMs, PreviousSegmentEndMs);

	const std::vector<double> SampleTimes =
	{
		0.0,
		0.5,
		250.25,
		Timeline->FinishTimeMs * 0.5,
		Timeline->FinishTimeMs,
		Timeline->FinishTimeMs + 100.0
	};
	double PreviousPosition = 0.0;
	for (double TimeMs : SampleTimes)
	{
		double Position = -1.0;
		ASSERT_TRUE(race.GetTimelinePositionAt(&runner, TimeMs, Position));
		ASSERT_TRUE(Position >= PreviousPosition);
		PreviousPosition = Position;
	}
	ASSERT_EQ(static_cast<double>(TrackLength), PreviousPosition);

	double PositionBeforeStart = -1.0;
	ASSERT_TRUE(race.GetTimelinePositionAt(&runner, -10.0, PositionBeforeStart));
	ASSERT_EQ(0.0, PositionBeforeStart);
	Racer outsider("Not In Race");
	double UnchangedPosition = -1.0;
	ASSERT_FALSE(race.GetTimelinePositionAt(&outsider, 0.0, UnchangedPosition));
	ASSERT_EQ(-1.0, UnchangedPosition);
}

TEST_CASE(Race_GassingDurationBeforePullUpUsesComposureAndConfidence)
{
	constexpr unsigned int RaceSeed = 81;
	constexpr unsigned int TrackLength = 1000000;
	Racer lowResilience("Low Resilience");
	Racer highComposure("High Composure");
	Racer highConfidence("High Confidence");
	Racer highResilience("High Resilience");
	ConfigureRacer(lowResilience, 81, 100.0, 100.0, 0.0, 40.0, 0.0);
	ConfigureRacer(highComposure, 82, 100.0, 100.0, 0.0, 40.0, 0.0);
	ConfigureRacer(highConfidence, 83, 100.0, 100.0, 0.0, 40.0, 1.0);
	ConfigureRacer(highResilience, 84, 100.0, 100.0, 0.0, 40.0, 1.0);
	lowResilience.Skills.Composure = 0.0;
	highComposure.Skills.Composure = 100.0;
	highConfidence.Skills.Composure = 0.0;
	highResilience.Skills.Composure = 100.0;

	Race race;
	ASSERT_TRUE(ConfigureRace(race, RaceSeed, TrackLength));
	race.Racers.push_back(&lowResilience);
	race.Racers.push_back(&highComposure);
	race.Racers.push_back(&highConfidence);
	race.Racers.push_back(&highResilience);
	ASSERT_EQ(RaceError::None, race.Precompute());

	const RacerRaceTimeline* LowTimeline = race.GetTimelineFor(&lowResilience);
	const RacerRaceTimeline* ComposureTimeline = race.GetTimelineFor(&highComposure);
	const RacerRaceTimeline* ConfidenceTimeline = race.GetTimelineFor(&highConfidence);
	const RacerRaceTimeline* HighTimeline = race.GetTimelineFor(&highResilience);
	ASSERT_TRUE(LowTimeline != nullptr);
	ASSERT_TRUE(ComposureTimeline != nullptr);
	ASSERT_TRUE(ConfidenceTimeline != nullptr);
	ASSERT_TRUE(HighTimeline != nullptr);
	ASSERT_TRUE(LowTimeline->PulledUp);
	ASSERT_TRUE(ComposureTimeline->PulledUp);
	ASSERT_TRUE(ConfidenceTimeline->PulledUp);
	ASSERT_TRUE(HighTimeline->PulledUp);
	ASSERT_TRUE(!LowTimeline->Finished);

	auto LastGassingNode = [](const RacerRaceTimeline& Timeline) -> const RaceTimelineNode*
	{
		const RaceTimelineNode* LastNode = nullptr;
		for (const RaceTimelineNode& Node : Timeline.Nodes)
		{
			if (Node.Type == RaceTimelineNodeType::Gassing)
				LastNode = &Node;
		}
		return LastNode;
	};
	const RaceTimelineNode* LowGassingNode = LastGassingNode(*LowTimeline);
	const RaceTimelineNode* ComposureGassingNode = LastGassingNode(*ComposureTimeline);
	const RaceTimelineNode* ConfidenceGassingNode = LastGassingNode(*ConfidenceTimeline);
	const RaceTimelineNode* HighGassingNode = LastGassingNode(*HighTimeline);
	ASSERT_TRUE(LowGassingNode != nullptr);
	ASSERT_TRUE(ComposureGassingNode != nullptr);
	ASSERT_TRUE(ConfidenceGassingNode != nullptr);
	ASSERT_TRUE(HighGassingNode != nullptr);
	ASSERT_EQ(1000u, LowGassingNode->Event.DurationMs);
	ASSERT_EQ(3000u, ComposureGassingNode->Event.DurationMs);
	ASSERT_EQ(3000u, ConfidenceGassingNode->Event.DurationMs);
	ASSERT_EQ(5000u, HighGassingNode->Event.DurationMs);
	ASSERT_TRUE(LowTimeline->PullUpTimeMs < ComposureTimeline->PullUpTimeMs);
	ASSERT_TRUE(LowTimeline->PullUpTimeMs < ConfidenceTimeline->PullUpTimeMs);
	ASSERT_TRUE(HighTimeline->PullUpTimeMs - HighGassingNode->StartTimeMs <= 5000.0);
	ASSERT_TRUE(std::any_of(LowTimeline->Nodes.begin(), LowTimeline->Nodes.end(), [](const RaceTimelineNode& Node)
	{
		return Node.Type == RaceTimelineNodeType::RandomEvent && Node.Event.Type == RaceEventType::PullingUp;
	}));

	RaceTimelineState OtherRacerState;
	ASSERT_TRUE(race.GetTimelineStateAt(&highComposure, LowTimeline->PullUpTimeMs + 1000.0, OtherRacerState));
	ASSERT_TRUE(OtherRacerState.Pace > 0.0);
	ASSERT_TRUE((OtherRacerState.ActiveEffects & RaceEffectPulledUp) == 0);

	ASSERT_EQ(RaceError::None, race.Sim().Error);
	ASSERT_TRUE(race.IsComplete());
	ASSERT_EQ(0u, race.FinishedCount());
	ASSERT_EQ(4u, race.PulledUpCount());
	ASSERT_TRUE(race.HasFinished());
	ASSERT_TRUE(race.GetByRank(FirstPlaceRank) == nullptr);
	for (const RacerRaceResult& Result : race.Result.RacerResults)
		ASSERT_EQ(PulledUpPosition, Result.Position);

	RaceTimelineState PulledUpState;
	ASSERT_TRUE(race.GetTimelineStateAt(&lowResilience, LowTimeline->PullUpTimeMs + 5000.0, PulledUpState));
	ASSERT_EQ(0.0, PulledUpState.Pace);
	ASSERT_EQ(0.1, PulledUpState.Endurance);
	ASSERT_TRUE((PulledUpState.ActiveEffects & RaceEffectPulledUp) != 0);
}

TEST_CASE(Race_PulledUpResultDoesNotTakeANumericPlace)
{
	constexpr unsigned int RaceSeed = 84;
	Racer pulledUpRunner("Pulled-up Runner");
	Racer finisher("Late Finisher");
	ConfigureRacer(pulledUpRunner, 84, 100.0, 100.0, 0.0, 40.0, 0.0);
	pulledUpRunner.Skills.Composure = 0.0;
	ConfigureRacer(finisher, 85, 100.0, 100.0, 100.0, 40.0);

	Race race;
	ASSERT_TRUE(ConfigureRace(race, RaceSeed, 20000));
	race.Racers.push_back(&pulledUpRunner);
	race.Racers.push_back(&finisher);
	ASSERT_EQ(RaceError::None, race.Precompute());

	const RacerRaceTimeline* PullUpTimeline = race.GetTimelineFor(&pulledUpRunner);
	const RacerRaceTimeline* FinishTimeline = race.GetTimelineFor(&finisher);
	ASSERT_TRUE(PullUpTimeline != nullptr);
	ASSERT_TRUE(FinishTimeline != nullptr);
	ASSERT_TRUE(PullUpTimeline->PulledUp);
	ASSERT_TRUE(FinishTimeline->Finished);
	ASSERT_TRUE(PullUpTimeline->PullUpTimeMs < FinishTimeline->FinishTimeMs);

	ASSERT_EQ(RaceError::None, race.Sim().Error);
	ASSERT_TRUE(race.IsComplete());
	ASSERT_EQ(1u, race.FinishedCount());
	ASSERT_EQ(1u, race.PulledUpCount());
	ASSERT_TRUE(!race.HasFinished(&pulledUpRunner));
	ASSERT_TRUE(race.HasFinished(&finisher));
	ASSERT_TRUE(race.GetByRank(FirstPlaceRank) == &finisher);
	ASSERT_TRUE(race.GetByRank(SecondPlaceRank) == nullptr);

	const auto PullUpResult = std::find_if(race.Result.RacerResults.begin(), race.Result.RacerResults.end(),
		[&pulledUpRunner](const RacerRaceResult& Result)
		{
			return Result.Racer == &pulledUpRunner;
		});
	ASSERT_TRUE(PullUpResult != race.Result.RacerResults.end());
	ASSERT_EQ(PulledUpPosition, PullUpResult->Position);
}

TEST_CASE(Race_TimelineSamplingRemainsStableAfterPersistentConditionsAreApplied)
{
	constexpr unsigned int RaceSeed = 75;
	constexpr unsigned int TrackLength = 10000;
	constexpr double InitialFatigue = 0.2;
	srand(RaceSeed);
	Racer runner("Timeline Sampling Runner");
	ConfigureRacer(runner, RaceSeed, 100.0, 100.0, 100.0, 100.0, 0.4, 0.0, InitialFatigue);

	Race race;
	ASSERT_TRUE(ConfigureRace(race, RaceSeed, TrackLength));
	race.Racers.push_back(&runner);
	ASSERT_EQ(RaceError::None, race.Precompute());

	const RacerRaceTimeline* Timeline = race.GetTimelineFor(&runner);
	ASSERT_TRUE(Timeline != nullptr);
	const double SampleTimeMs = Timeline->FinishTimeMs / 2.0;
	RaceTimelineState BeforeCompletion;
	ASSERT_TRUE(race.GetTimelineStateAt(&runner, SampleTimeMs, BeforeCompletion));

	ASSERT_EQ(RaceError::None, race.Sim().Error);
	ASSERT_TRUE(race.IsComplete());
	ASSERT_TRUE(runner.Conditions.Fatigue > InitialFatigue);

	RaceTimelineState AfterCompletion;
	ASSERT_TRUE(race.GetTimelineStateAt(&runner, SampleTimeMs, AfterCompletion));
	ASSERT_EQ(BeforeCompletion.Pace, AfterCompletion.Pace);
	ASSERT_EQ(BeforeCompletion.Endurance, AfterCompletion.Endurance);
	ASSERT_EQ(BeforeCompletion.ActiveEffects, AfterCompletion.ActiveEffects);
}

TEST_CASE(Race_ReflexControlsStartDelayAndAccelerationControlsPace)
{
	constexpr double ExpectedZeroReflexResponseDelayMs = 1000.0;
	constexpr double ExpectedFullReflexResponseDelayMs = 10.0;
	constexpr unsigned int InitialPlaybackMs = 100;
	constexpr unsigned int AdditionalPlaybackMs = 1000;
	srand(43);
	Racer slowResponse("Slow Response");
	Racer quickResponse("Quick Response");
	Racer slowAcceleration("Slow Acceleration");
	ConfigureRacer(slowResponse, 45, 0.0, 100.0, 100.0, 100.0);
	ConfigureRacer(quickResponse, 46, 100.0, 100.0, 100.0, 100.0);
	ConfigureRacer(slowAcceleration, 48, 100.0, 0.0, 100.0, 100.0);
	Race race;
	ASSERT_TRUE(ConfigureRace(race, 47, 10000));
	race.Racers = { &slowResponse, &quickResponse, &slowAcceleration };
	ASSERT_EQ(RaceError::None, race.Precompute());
	const RacerRaceTimeline* SlowResponseTimeline = race.GetTimelineFor(&slowResponse);
	const RacerRaceTimeline* QuickResponseTimeline = race.GetTimelineFor(&quickResponse);
	ASSERT_TRUE(SlowResponseTimeline != nullptr && QuickResponseTimeline != nullptr);
	ASSERT_EQ(ExpectedZeroReflexResponseDelayMs, SlowResponseTimeline->Nodes.front().EndTimeMs);
	ASSERT_EQ(ExpectedFullReflexResponseDelayMs, QuickResponseTimeline->Nodes.front().EndTimeMs);

	ASSERT_TRUE(race.GetEventsThroughPlaybackTime().empty());
	ASSERT_EQ(1.0, race.GetCurrentEndurance(&slowResponse));
	race.Tick(InitialPlaybackMs, RaceStatus::Racing);
	ASSERT_EQ(0, slowResponse.Pos.X);
	ASSERT_TRUE(quickResponse.Pos.Velocity > slowResponse.Pos.Velocity);
	ASSERT_TRUE(quickResponse.Pos.Velocity > slowAcceleration.Pos.Velocity);

	race.Tick(AdditionalPlaybackMs, RaceStatus::Racing);
	ASSERT_TRUE(slowResponse.Pos.Velocity > 0.0);
	ASSERT_TRUE(race.GetCurrentEndurance(&slowResponse) < 1.0);
}

TEST_CASE(Race_PlaybackStateIsIndependentOfTickChunking)
{
	constexpr unsigned int RacerInitializationSeed = 81;
	constexpr unsigned int RacerGuid = 82;
	constexpr unsigned int RaceSeed = 83;
	constexpr unsigned int TrackLength = 20000;
	constexpr unsigned int PlaybackDurationMs = 30000;
	constexpr double MaximumTestRating = 100.0;
	constexpr double RunnerEndurance = 0.0;
	constexpr double RunnerSpeed = 40.0;
	srand(RacerInitializationSeed);
	Racer SingleTickRunner("Single Tick Runner");
	Racer RepeatedTickRunner("Repeated Tick Runner");
	ConfigureRacer(SingleTickRunner, RacerGuid, MaximumTestRating, MaximumTestRating, RunnerEndurance, RunnerSpeed);
	ConfigureRacer(RepeatedTickRunner, RacerGuid, MaximumTestRating, MaximumTestRating, RunnerEndurance, RunnerSpeed);
	Race SingleTickRace;
	Race RepeatedTickRace;
	ASSERT_TRUE(ConfigureRace(SingleTickRace, RaceSeed, TrackLength));
	ASSERT_TRUE(ConfigureRace(RepeatedTickRace, RaceSeed, TrackLength));
	SingleTickRace.Racers.push_back(&SingleTickRunner);
	RepeatedTickRace.Racers.push_back(&RepeatedTickRunner);
	ASSERT_EQ(RaceError::None, SingleTickRace.Precompute());
	ASSERT_EQ(RaceError::None, RepeatedTickRace.Precompute());

	SingleTickRace.Tick(PlaybackDurationMs, RaceStatus::Racing);
	RaceStatus RepeatedTickStatus = RaceStatus::Racing;
	const unsigned int FullTicks = PlaybackDurationMs / RaceTickIntervalMs;
	const unsigned int RemainderMs = PlaybackDurationMs % RaceTickIntervalMs;
	for (unsigned int TickIndex = 0; TickIndex < FullTicks; ++TickIndex)
		RepeatedTickStatus = RepeatedTickRace.Tick(RaceTickIntervalMs, RepeatedTickStatus);
	if (RemainderMs > 0)
		RepeatedTickRace.Tick(RemainderMs, RepeatedTickStatus);

	ASSERT_EQ(SingleTickRunner.Pos.X, RepeatedTickRunner.Pos.X);
	ASSERT_EQ(SingleTickRunner.Pos.Velocity, RepeatedTickRunner.Pos.Velocity);
	ASSERT_EQ(SingleTickRace.GetCurrentEndurance(&SingleTickRunner), RepeatedTickRace.GetCurrentEndurance(&RepeatedTickRunner));
	ASSERT_EQ(SingleTickRace.GetActiveEffects(&SingleTickRunner), RepeatedTickRace.GetActiveEffects(&RepeatedTickRunner));
}

TEST_CASE(Race_EnduranceDepletionTimeDoesNotDependOnTrackLength)
{
	constexpr double FullEnduranceDurationMs = 90000.0;
	constexpr double GassingEnduranceThreshold = 0.1;
	constexpr double GassingDetectionStepMs = 50.0;
	constexpr unsigned int PlaybackTimeBeforeGassingMs = 14000;
	constexpr double ExpectedGassingTimeMs = FullEnduranceDurationMs * (1.0 - GassingEnduranceThreshold);
	srand(51);
	Racer shortTrackRunner("Short Track Runner");
	Racer longTrackRunner("Long Track Runner");
	ConfigureRacer(shortTrackRunner, 51, 100.0, 100.0, 0.0, 40.0, 0.0, 0.0);
	ConfigureRacer(longTrackRunner, 51, 100.0, 100.0, 0.0, 40.0, 0.0, 0.0);
	Race shortRace;
	Race longRace;
	ASSERT_TRUE(ConfigureRace(shortRace, 52, 10000));
	ASSERT_TRUE(ConfigureRace(longRace, 52, 100000));
	shortRace.Racers.push_back(&shortTrackRunner);
	longRace.Racers.push_back(&longTrackRunner);
	ASSERT_EQ(RaceError::None, shortRace.Precompute());
	ASSERT_EQ(RaceError::None, longRace.Precompute());

	shortRace.Tick(PlaybackTimeBeforeGassingMs, RaceStatus::Racing);
	longRace.Tick(PlaybackTimeBeforeGassingMs, RaceStatus::Racing);
	ASSERT_EQ(shortRace.GetCurrentEndurance(&shortTrackRunner), longRace.GetCurrentEndurance(&longTrackRunner));
	ASSERT_TRUE(shortRace.GetCurrentEndurance(&shortTrackRunner) < 1.0);
	ASSERT_TRUE(shortRace.GetCurrentEndurance(&shortTrackRunner) > GassingEnduranceThreshold);

	const unsigned int PlaybackTimeThroughGassingMs = static_cast<unsigned int>(
		ExpectedGassingTimeMs + GassingDetectionStepMs);
	shortRace.Tick(PlaybackTimeThroughGassingMs - PlaybackTimeBeforeGassingMs, RaceStatus::Racing);
	longRace.Tick(PlaybackTimeThroughGassingMs - PlaybackTimeBeforeGassingMs, RaceStatus::Racing);
	auto shortEvents = shortRace.GetEventsThroughPlaybackTime();
	auto longEvents = longRace.GetEventsThroughPlaybackTime();
	const auto FindGassingTime = [](const std::vector<RaceEvent>& Events)
	{
		const auto GassingEvent = std::find_if(Events.begin(), Events.end(), [](const RaceEvent& Event)
		{
			return Event.Type == RaceEventType::Gassing;
		});
		return GassingEvent == Events.end() ? -1.0 : GassingEvent->TimeMs;
	};
	const double ShortGassingTimeMs = FindGassingTime(shortEvents);
	const double LongGassingTimeMs = FindGassingTime(longEvents);
	ASSERT_TRUE(ShortGassingTimeMs >= ExpectedGassingTimeMs);
	ASSERT_TRUE(ShortGassingTimeMs <= ExpectedGassingTimeMs + GassingDetectionStepMs);
	ASSERT_EQ(ShortGassingTimeMs, LongGassingTimeMs);
}

TEST_CASE(Race_LowerEnduranceLetsSomeRacersGasOnLongerTrack)
{
	constexpr unsigned int RaceSeed = 56;
	constexpr unsigned int SlowRacerGuid = 57;
	constexpr unsigned int FastRacerGuid = 58;
	constexpr unsigned int TestTrackLength = 16000;
	constexpr double LowEndurance = 20.0;
	constexpr double HighEndurance = 100.0;
	Racer SlowRunner("Slow Runner");
	Racer FastRunner("Fast Runner");
	ConfigureRacer(SlowRunner, SlowRacerGuid, 100.0, 100.0, LowEndurance, 40.0);
	ConfigureRacer(FastRunner, FastRacerGuid, 100.0, 100.0, HighEndurance, 100.0);
	Race race;
	ASSERT_TRUE(ConfigureRace(race, RaceSeed, TestTrackLength));
	race.Racers.push_back(&SlowRunner);
	race.Racers.push_back(&FastRunner);
	ASSERT_EQ(RaceError::None, race.Precompute());

	const RacerRaceTimeline* SlowTimeline = race.GetTimelineFor(&SlowRunner);
	const RacerRaceTimeline* FastTimeline = race.GetTimelineFor(&FastRunner);
	ASSERT_TRUE(SlowTimeline != nullptr && FastTimeline != nullptr);
	const auto HasGassing = [](const RacerRaceTimeline& Timeline)
	{
		return std::any_of(Timeline.Nodes.begin(), Timeline.Nodes.end(), [](const RaceTimelineNode& Node)
		{
			return Node.Type == RaceTimelineNodeType::Gassing;
		});
	};
	ASSERT_TRUE(HasGassing(*SlowTimeline));
	ASSERT_FALSE(HasGassing(*FastTimeline));
}

TEST_CASE(Race_LowEnduranceDepletesMoreSlowlyWithoutChangingHighEndurance)
{
	constexpr unsigned int RaceSeed = 59;
	constexpr unsigned int LowEnduranceGuid = 60;
	constexpr unsigned int HighEnduranceGuid = 61;
	constexpr unsigned int TrackLength = 100000;
	constexpr unsigned int PlaybackBeforeLowEnduranceGassingMs = 32000;
	constexpr double GassingDetectionStepMs = 50.0;
	constexpr double GassingEnduranceThreshold = 0.1;
	constexpr double ExpectedLowEnduranceFullDurationMs = 162000.0;
	constexpr double ExpectedHighEnduranceFullDurationMs = 186000.0;
	constexpr double ExpectedLowEnduranceGassingTimeMs = ExpectedLowEnduranceFullDurationMs *
		(1.0 - GassingEnduranceThreshold);
	constexpr double ExpectedHighEnduranceGassingTimeMs = ExpectedHighEnduranceFullDurationMs *
		(1.0 - GassingEnduranceThreshold);
	srand(59);
	Racer LowEnduranceRunner("Low Endurance");
	Racer HighEnduranceRunner("High Endurance");
	ConfigureRacer(LowEnduranceRunner, LowEnduranceGuid, 100.0, 100.0, 40.0, 100.0);
	ConfigureRacer(HighEnduranceRunner, HighEnduranceGuid, 100.0, 100.0, 80.0, 100.0);
	Race LowEnduranceRace;
	Race HighEnduranceRace;
	ASSERT_TRUE(ConfigureRace(LowEnduranceRace, RaceSeed, TrackLength));
	ASSERT_TRUE(ConfigureRace(HighEnduranceRace, RaceSeed, TrackLength));
	LowEnduranceRace.Racers.push_back(&LowEnduranceRunner);
	HighEnduranceRace.Racers.push_back(&HighEnduranceRunner);
	ASSERT_EQ(RaceError::None, LowEnduranceRace.Precompute());
	ASSERT_EQ(RaceError::None, HighEnduranceRace.Precompute());

	LowEnduranceRace.Tick(PlaybackBeforeLowEnduranceGassingMs, RaceStatus::Racing);
	ASSERT_TRUE(LowEnduranceRace.GetCurrentEndurance(&LowEnduranceRunner) > GassingEnduranceThreshold);
	ASSERT_EQ(RaceEffectNone, LowEnduranceRace.GetActiveEffects(&LowEnduranceRunner));

	const auto FindGassingNode = [](const RacerRaceTimeline* Timeline)
	{
		return std::find_if(Timeline->Nodes.begin(), Timeline->Nodes.end(), [](const RaceTimelineNode& Node)
		{
			return Node.Type == RaceTimelineNodeType::Gassing;
		});
	};
	const RacerRaceTimeline* LowTimeline = LowEnduranceRace.GetTimelineFor(&LowEnduranceRunner);
	const RacerRaceTimeline* HighTimeline = HighEnduranceRace.GetTimelineFor(&HighEnduranceRunner);
	ASSERT_TRUE(LowTimeline != nullptr && HighTimeline != nullptr);
	const auto LowGassingNode = FindGassingNode(LowTimeline);
	const auto HighGassingNode = FindGassingNode(HighTimeline);
	ASSERT_TRUE(LowGassingNode != LowTimeline->Nodes.end());
	ASSERT_TRUE(HighGassingNode != HighTimeline->Nodes.end());
	ASSERT_TRUE(LowGassingNode->StartTimeMs >= ExpectedLowEnduranceGassingTimeMs);
	ASSERT_TRUE(LowGassingNode->StartTimeMs <= ExpectedLowEnduranceGassingTimeMs + GassingDetectionStepMs);
	ASSERT_TRUE(HighGassingNode->StartTimeMs >= ExpectedHighEnduranceGassingTimeMs);
	ASSERT_TRUE(HighGassingNode->StartTimeMs <= ExpectedHighEnduranceGassingTimeMs + GassingDetectionStepMs);
}

TEST_CASE(Race_EnduranceCurveReducesPaceWithoutChangingAcceleration)
{
	constexpr unsigned int RacerInitializationSeed = 53;
	constexpr unsigned int RacerGuid = 54;
	constexpr unsigned int RaceSeed = 55;
	constexpr unsigned int TrackLength = 8000;
	constexpr unsigned int PlaybackTimeMs = 7500;
	constexpr double MaximumTestRating = 100.0;
	constexpr double DepletedEndurance = 0.0;
	constexpr double FullEndurance = 100.0;
	constexpr double TestSpeed = 100.0;
	srand(RacerInitializationSeed);
	Racer FastDepletionRunner("Fast Depletion");
	Racer SlowDepletionRunner("Slow Depletion");
	Racer HighEnduranceRunner("High Endurance");
	ConfigureRacer(FastDepletionRunner, RacerGuid, MaximumTestRating, MaximumTestRating,
		DepletedEndurance, TestSpeed);
	ConfigureRacer(SlowDepletionRunner, RacerGuid, MaximumTestRating, MaximumTestRating,
		DepletedEndurance, TestSpeed);
	ConfigureRacer(HighEnduranceRunner, RacerGuid, MaximumTestRating, MaximumTestRating,
		FullEndurance, TestSpeed);
	FastDepletionRunner.EnduranceCurve = { 0.25, 0.35, 0.75, 0.05, true };
	SlowDepletionRunner.EnduranceCurve = { 0.25, 0.95, 0.75, 0.65, true };
	HighEnduranceRunner.EnduranceCurve = SlowDepletionRunner.EnduranceCurve;
	Race FastDepletionRace;
	Race SlowDepletionRace;
	Race HighEnduranceRace;
	ASSERT_TRUE(ConfigureRace(FastDepletionRace, RaceSeed, TrackLength));
	ASSERT_TRUE(ConfigureRace(SlowDepletionRace, RaceSeed, TrackLength));
	ASSERT_TRUE(ConfigureRace(HighEnduranceRace, RaceSeed, TrackLength));
	FastDepletionRace.Racers.push_back(&FastDepletionRunner);
	SlowDepletionRace.Racers.push_back(&SlowDepletionRunner);
	HighEnduranceRace.Racers.push_back(&HighEnduranceRunner);
	ASSERT_EQ(RaceError::None, FastDepletionRace.Precompute());
	ASSERT_EQ(RaceError::None, SlowDepletionRace.Precompute());
	ASSERT_EQ(RaceError::None, HighEnduranceRace.Precompute());

	const RacerRaceTimeline* FastTimeline = FastDepletionRace.GetTimelineFor(&FastDepletionRunner);
	const RacerRaceTimeline* SlowTimeline = SlowDepletionRace.GetTimelineFor(&SlowDepletionRunner);
	ASSERT_TRUE(FastTimeline != nullptr && SlowTimeline != nullptr);
	const auto FindAccelerationNode = [](const RacerRaceTimeline* Timeline)
	{
		return std::find_if(Timeline->Nodes.begin(), Timeline->Nodes.end(), [](const RaceTimelineNode& Node)
		{
			return Node.Type == RaceTimelineNodeType::AccelerationToTopSpeed;
		});
	};
	const auto FastAcceleration = FindAccelerationNode(FastTimeline);
	const auto SlowAcceleration = FindAccelerationNode(SlowTimeline);
	ASSERT_TRUE(FastAcceleration != FastTimeline->Nodes.end());
	ASSERT_TRUE(SlowAcceleration != SlowTimeline->Nodes.end());
	ASSERT_EQ(FastAcceleration->StartTimeMs, SlowAcceleration->StartTimeMs);
	ASSERT_EQ(FastAcceleration->EndPace, SlowAcceleration->EndPace);

	FastDepletionRace.Tick(PlaybackTimeMs, RaceStatus::Racing);
	SlowDepletionRace.Tick(PlaybackTimeMs, RaceStatus::Racing);
	HighEnduranceRace.Tick(PlaybackTimeMs, RaceStatus::Racing);
	ASSERT_TRUE(FastDepletionRace.GetCurrentEndurance(&FastDepletionRunner) <
		SlowDepletionRace.GetCurrentEndurance(&SlowDepletionRunner));
	ASSERT_TRUE(FastDepletionRunner.Pos.Velocity < SlowDepletionRunner.Pos.Velocity);
	ASSERT_TRUE(HighEnduranceRace.GetCurrentEndurance(&HighEnduranceRunner) >
		SlowDepletionRace.GetCurrentEndurance(&SlowDepletionRunner));
}

TEST_CASE(Race_EnduranceReserveScalesPaceLinearly)
{
	constexpr unsigned int RacerGuid = 72;
	constexpr unsigned int RaceSeed = 73;
	constexpr unsigned int TrackLength = 100000;
	constexpr unsigned int PlaybackTimeMs = 950;
	constexpr double TestSpeed = 100.0;
	constexpr double PaceUnitsPerSpeedPoint = 5.0;
	constexpr double ResponseDelayMs = 10.0;
	constexpr double AccelerationDurationMs = 1500.0;
	constexpr double EmptyReservePaceMultiplier = 0.2;
	srand(71);
	Racer Runner("Endurance Scale");
	ConfigureRacer(Runner, RacerGuid, 100.0, 100.0, 0.0, TestSpeed);
	Runner.EnduranceCurve = { 0.0, 0.0, 0.0, 0.0, true };
	Race race;
	ASSERT_TRUE(ConfigureRace(race, RaceSeed, TrackLength));
	race.Racers.push_back(&Runner);
	ASSERT_EQ(RaceError::None, race.Precompute());

	race.Tick(PlaybackTimeMs, RaceStatus::Racing);
	const double EnduranceReserve = race.GetCurrentEndurance(&Runner);
	ASSERT_TRUE(EnduranceReserve > 0.0 && EnduranceReserve < 0.5);
	ASSERT_EQ(RaceEffectNone, race.GetActiveEffects(&Runner));
	const double NormalPace = TestSpeed * PaceUnitsPerSpeedPoint *
		Runner.AccelerationCurve.Evaluate((PlaybackTimeMs - ResponseDelayMs) / AccelerationDurationMs);
	const double ExpectedPace = NormalPace * (EmptyReservePaceMultiplier +
		(1.0 - EmptyReservePaceMultiplier) * EnduranceReserve);
	ASSERT_EQ(ExpectedPace, Runner.Pos.Velocity);
}

TEST_CASE(Race_GassingStartsAtTenPercentAndHoldsEnduranceReserve)
{
	constexpr unsigned int RacerGuid = 74;
	constexpr unsigned int RaceSeed = 75;
	constexpr unsigned int TrackLength = 100000;
	constexpr double TestSpeed = 100.0;
	constexpr double PaceUnitsPerSpeedPoint = 5.0;
	constexpr double EmptyReservePaceMultiplier = 0.2;
	constexpr double GassingEnduranceThreshold = 0.1;
	constexpr double FullEnduranceDurationMs = 90000.0;
	constexpr double GassingDetectionStepMs = 50.0;
	constexpr double PlaybackTimeAfterGassingStartMs = 200.0;
	constexpr double ExpectedGassingStartMs = FullEnduranceDurationMs * (1.0 - GassingEnduranceThreshold);
	srand(73);
	Racer Runner("Empty Endurance");
	ConfigureRacer(Runner, RacerGuid, 100.0, 100.0, 0.0, TestSpeed);
	Race race;
	ASSERT_TRUE(ConfigureRace(race, RaceSeed, TrackLength));
	race.Racers.push_back(&Runner);
	ASSERT_EQ(RaceError::None, race.Precompute());

	const RacerRaceTimeline* Timeline = race.GetTimelineFor(&Runner);
	ASSERT_TRUE(Timeline != nullptr);
	const auto GassingNode = std::find_if(Timeline->Nodes.begin(), Timeline->Nodes.end(),
		[](const RaceTimelineNode& Node)
		{
			return Node.Type == RaceTimelineNodeType::Gassing;
		});
	ASSERT_TRUE(GassingNode != Timeline->Nodes.end());
	ASSERT_TRUE(GassingNode->StartTimeMs >= ExpectedGassingStartMs);
	ASSERT_TRUE(GassingNode->StartTimeMs <= ExpectedGassingStartMs + GassingDetectionStepMs);
	ASSERT_EQ(0.0, Runner.EnduranceCurve.Evaluate(1.0));
	const double GassingPaceMultiplier = EmptyReservePaceMultiplier +
		(1.0 - EmptyReservePaceMultiplier) * GassingEnduranceThreshold;
	ASSERT_EQ(TestSpeed * PaceUnitsPerSpeedPoint * GassingPaceMultiplier, GassingNode->StartPace);

	const unsigned int PlaybackTimeMs = static_cast<unsigned int>(GassingNode->StartTimeMs + PlaybackTimeAfterGassingStartMs);
	race.Tick(PlaybackTimeMs, RaceStatus::Racing);
	ASSERT_TRUE(Runner.EnduranceCurve.Evaluate(static_cast<double>(PlaybackTimeMs) / FullEnduranceDurationMs) <
		GassingEnduranceThreshold);
	ASSERT_EQ(GassingEnduranceThreshold, race.GetCurrentEndurance(&Runner));
	ASSERT_TRUE((race.GetActiveEffects(&Runner) & RaceEffectGassing) != 0);
}

TEST_CASE(Race_IdenticalInputsAndSeedProduceIdenticalEventsAndResults)
{
	srand(61);
	Racer firstA("First A"), secondA("Second A");
	Racer firstB("First B"), secondB("Second B");
	ConfigureRacer(firstA, 61, 80.0, 70.0, 100.0, 90.0, 0.65, 75.0, 0.1);
	ConfigureRacer(secondA, 62, 70.0, 85.0, 90.0, 80.0, 0.4, 35.0, 0.2);
	ConfigureRacer(firstB, 61, 80.0, 70.0, 100.0, 90.0, 0.65, 75.0, 0.1);
	ConfigureRacer(secondB, 62, 70.0, 85.0, 90.0, 80.0, 0.4, 35.0, 0.2);
	Race firstRace;
	Race secondRace;
	ASSERT_TRUE(ConfigureRace(firstRace, 63, 30000));
	ASSERT_TRUE(ConfigureRace(secondRace, 63, 30000));
	firstRace.Racers = { &firstA, &secondA };
	secondRace.Racers = { &firstB, &secondB };
	firstRace.Sim();
	ASSERT_EQ(RaceError::None, secondRace.Precompute());
	ASSERT_TRUE(secondRace.GetEventsThroughPlaybackTime().empty());
	ASSERT_TRUE(secondRace.Tick(0xFFFFFFFFu, RaceStatus::Racing) == RaceStatus::Finished);

	auto firstEvents = firstRace.GetEventsThroughPlaybackTime();
	auto secondEvents = secondRace.GetEventsThroughPlaybackTime();
	ASSERT_EQ(firstEvents.size(), secondEvents.size());
	for (std::size_t Index = 0; Index < firstEvents.size(); ++Index)
	{
		ASSERT_TRUE(firstEvents[Index].Type == secondEvents[Index].Type);
		ASSERT_EQ(firstEvents[Index].RacerIndex, secondEvents[Index].RacerIndex);
		ASSERT_EQ(firstEvents[Index].TimeMs, secondEvents[Index].TimeMs);
		ASSERT_EQ(firstEvents[Index].DurationMs, secondEvents[Index].DurationMs);
		ASSERT_EQ(firstEvents[Index].Magnitude, secondEvents[Index].Magnitude);
	}
	ASSERT_EQ(firstRace.Result.RacerResults.size(), secondRace.Result.RacerResults.size());
	for (std::size_t Index = 0; Index < firstRace.Result.RacerResults.size(); ++Index)
	{
		ASSERT_EQ(firstRace.Result.RacerResults[Index].Position, secondRace.Result.RacerResults[Index].Position);
		ASSERT_EQ(firstRace.Result.RacerResults[Index].Ms, secondRace.Result.RacerResults[Index].Ms);
		ASSERT_EQ(firstRace.Result.RacerResults[Index].Racer->GUID.GUID, secondRace.Result.RacerResults[Index].Racer->GUID.GUID);
	}
}

TEST_CASE(Race_SecondWindAndRaceEventsArePlaybackBoundAndDoNotChangeBaseStats)
{
	constexpr std::size_t MinimumStumblesForHighFatigueRace = 40;
	constexpr std::size_t EventRunnerCount = 32;
	constexpr std::size_t SecondWindRunnerCount = 2048;
	srand(71);
	Racer runner("Event Runner");
	ConfigureRacer(runner, 71, 100.0, 100.0, 0.0, 100.0, 0.8, 100.0, 1.0);
	runner.Skills.Composure = 100.0;
	std::vector<Racer> AdditionalRunners;
	AdditionalRunners.reserve(EventRunnerCount - 1);
	std::vector<Racer*> EventRunners{ &runner };
	EventRunners.reserve(EventRunnerCount + SecondWindRunnerCount);
	for (std::size_t Index = 1; Index < EventRunnerCount; ++Index)
	{
		AdditionalRunners.emplace_back("Event Runner " + std::to_string(Index + 1));
		Racer& AdditionalRunner = AdditionalRunners.back();
		ConfigureRacer(AdditionalRunner, static_cast<unsigned int>(71 + Index),
			100.0, 100.0, 0.0, 100.0, 0.8, 100.0, 1.0);
		AdditionalRunner.Skills.Composure = 100.0;
		EventRunners.push_back(&AdditionalRunner);
	}
	std::vector<Racer> SecondWindRunners;
	SecondWindRunners.reserve(SecondWindRunnerCount);
	for (std::size_t Index = 0; Index < SecondWindRunnerCount; ++Index)
	{
		SecondWindRunners.emplace_back("Second Wind Runner " + std::to_string(Index + 1));
		Racer& SecondWindRunner = SecondWindRunners.back();
		ConfigureRacer(SecondWindRunner, static_cast<unsigned int>(1000 + Index),
		100.0, 100.0, 0.0, 100.0, 0.8, 100.0);
		SecondWindRunner.Skills.Composure = 100.0;
		EventRunners.push_back(&SecondWindRunner);
	}
	Race race;
	ASSERT_TRUE(ConfigureRace(race, 72, 60000));
	race.Racers = EventRunners;
	ASSERT_EQ(RaceError::None, race.Precompute());

	ASSERT_TRUE(race.GetEventsThroughPlaybackTime().empty());
	ASSERT_EQ(0.0, race.GetTimelineDurationMs());

	bool observedStumbleEffect = false;
	bool observedSecondWindPace = false;
	bool observedSecondWindLabelEffect = false;
	const RacerRaceTimeline* StumbleTimeline = nullptr;
	const RaceTimelineNode* StumbleNode = nullptr;
	const Racer* StumbleEntrant = nullptr;
	double StumbleTimelineEndTimeMs = 0.0;
	const RacerRaceTimeline* SecondWindTimeline = nullptr;
	const RaceTimelineNode* SecondWindNode = nullptr;
	const Racer* SecondWindEntrant = nullptr;
	for (const Racer* Entrant : race.GetEntrants())
	{
		const RacerRaceTimeline* Timeline = race.GetTimelineFor(Entrant);
		ASSERT_TRUE(Timeline != nullptr);
		const double TimelineEndTimeMs = Timeline->Finished ? Timeline->FinishTimeMs :
			Timeline->PulledUp ? Timeline->PullUpTimeMs : Timeline->MotionSegments.back().EndTimeMs;
		for (const RaceTimelineNode& Node : Timeline->Nodes)
		{
			if (Node.Type != RaceTimelineNodeType::RandomEvent)
				continue;
			if (Node.Event.Type == RaceEventType::Stumble && Node.StartTimeMs < TimelineEndTimeMs &&
				(StumbleNode == nullptr || Node.StartTimeMs < StumbleNode->StartTimeMs))
			{
				StumbleTimeline = Timeline;
				StumbleNode = &Node;
				StumbleEntrant = Entrant;
				StumbleTimelineEndTimeMs = TimelineEndTimeMs;
			}
			if (Node.Event.Type == RaceEventType::SecondWind &&
				(SecondWindNode == nullptr || Node.StartTimeMs < SecondWindNode->StartTimeMs))
			{
				SecondWindTimeline = Timeline;
				SecondWindNode = &Node;
				SecondWindEntrant = Entrant;
			}
		}
	}
	ASSERT_TRUE(StumbleTimeline != nullptr);
	ASSERT_TRUE(StumbleNode != nullptr);
	ASSERT_TRUE(StumbleEntrant != nullptr);
	ASSERT_TRUE(SecondWindTimeline != nullptr);
	ASSERT_TRUE(SecondWindNode != nullptr);
	ASSERT_TRUE(SecondWindEntrant != nullptr);

	const double StumbleSampleTimeMs = std::min(
		(StumbleNode->StartTimeMs + StumbleNode->EndTimeMs) * 0.5,
		(StumbleNode->StartTimeMs + StumbleTimelineEndTimeMs) * 0.5);
	RaceTimelineState AtStumble;
	ASSERT_TRUE(race.GetTimelineStateAt(StumbleEntrant, StumbleSampleTimeMs, AtStumble));
	ASSERT_TRUE((AtStumble.ActiveEffects & RaceEffectStumble) != 0);
	observedStumbleEffect = true;

	const RaceTimelineNode* LastGassing = nullptr;
	for (const RaceTimelineNode& Node : SecondWindTimeline->Nodes)
	{
		if (Node.Type == RaceTimelineNodeType::Gassing &&
			Node.StartTimeMs <= SecondWindNode->StartTimeMs &&
			(LastGassing == nullptr || Node.StartTimeMs > LastGassing->StartTimeMs))
			LastGassing = &Node;
	}
	ASSERT_TRUE(LastGassing != nullptr);
	RaceTimelineState AtSecondWind;
	ASSERT_TRUE(race.GetTimelineStateAt(SecondWindEntrant, SecondWindNode->Event.TimeMs, AtSecondWind));
	ASSERT_TRUE((AtSecondWind.ActiveEffects & RaceEffectSecondWind) != 0);
	observedSecondWindPace = std::abs(AtSecondWind.Pace - LastGassing->Event.Magnitude) < 1e-9;

	RaceTimelineState AtSecondWindLabelTime;
	ASSERT_TRUE(race.GetTimelineStateAt(SecondWindEntrant, SecondWindNode->Event.TimeMs + 500.0,
		AtSecondWindLabelTime));
	observedSecondWindLabelEffect =
		(AtSecondWindLabelTime.ActiveEffects & RaceEffectSecondWind) != 0;

	RaceStatus status = RaceStatus::Racing;
	const unsigned int SecondWindPlaybackTimeMs =
		static_cast<unsigned int>(std::ceil(SecondWindNode->Event.TimeMs));
	status = race.Tick(SecondWindPlaybackTimeMs, status);
	ASSERT_TRUE(race.GetCurrentTimeMs() >= SecondWindNode->Event.TimeMs);
	ASSERT_TRUE((race.GetActiveEffects(SecondWindEntrant) & RaceEffectSecondWind) != 0);
	const auto VisibleSecondWindEvents = race.GetEventsThroughPlaybackTime();
	ASSERT_TRUE(std::any_of(VisibleSecondWindEvents.begin(), VisibleSecondWindEvents.end(),
		[SecondWindNode, SecondWindEntrant](const RaceEvent& Event)
		{
			return Event.Type == RaceEventType::SecondWind && Event.Entrant == SecondWindEntrant &&
				Event.TimeMs == SecondWindNode->Event.TimeMs;
		}));
	status = race.Tick(500, status);
	ASSERT_TRUE((race.GetActiveEffects(SecondWindEntrant) & RaceEffectSecondWind) != 0);
	status = race.Tick(0xFFFFFFFFu, status);

	ASSERT_TRUE(race.IsComplete());
	ASSERT_TRUE(status == RaceStatus::Finished);
	ASSERT_TRUE(race.GetTimelineDurationMs() > 0.0);
	auto events = race.GetEventsThroughPlaybackTime();
	bool observedGassing = false;
	bool observedSecondWind = false;
	bool observedLostBelief = false;
	bool observedBelief = false;
	std::size_t stumbleEventCount = 0;
	for (const RaceEvent& Event : events)
	{
		observedGassing |= Event.Type == RaceEventType::Gassing;
		observedSecondWind |= Event.Type == RaceEventType::SecondWind;
		observedLostBelief |= Event.Type == RaceEventType::LostBelief;
		observedBelief |= Event.Type == RaceEventType::BelievesInSelf;
		if (Event.Type == RaceEventType::Stumble)
			++stumbleEventCount;
		if (Event.Type == RaceEventType::SecondWind)
			ASSERT_TRUE(Event.Magnitude > 0.0 && Event.Magnitude <= 0.5);
	}
	ASSERT_TRUE(observedGassing);
	ASSERT_TRUE(observedSecondWind);
	ASSERT_TRUE(observedStumbleEffect);
	ASSERT_TRUE(observedSecondWindPace);
	ASSERT_TRUE(observedSecondWindLabelEffect);
	ASSERT_TRUE(stumbleEventCount >= MinimumStumblesForHighFatigueRace);
	ASSERT_TRUE(!observedLostBelief);
	ASSERT_TRUE(!observedBelief);
	ASSERT_EQ(100.0, runner.Skills.Speed);
	ASSERT_EQ(0.0, runner.Skills.Endurance);
	ASSERT_TRUE(runner.Conditions.Fatigue > 0.0);
	ASSERT_TRUE(runner.Conditions.Confidence > 0.5);
}

TEST_CASE(Race_StumbleContinuesAccelerationCurveAfterEvent)
{
	constexpr std::size_t StumbleRunnerCount = 64;
	constexpr unsigned int RaceSeed = 119;
	constexpr unsigned int TrackLength = 5000;
	constexpr unsigned int StumbleSampleIntervalMs = 50;
	constexpr double ResponseDelayMs = 10.0;
	constexpr double AccelerationDurationMs = 6000.0;
	constexpr double EmptyEndurancePaceMultiplier = 0.2;
	srand(118);
	std::vector<Racer> Runners;
	Runners.reserve(StumbleRunnerCount);
	std::vector<Racer*> Entrants;
	Entrants.reserve(StumbleRunnerCount);
	for (std::size_t Index = 0; Index < StumbleRunnerCount; ++Index)
	{
		Runners.emplace_back("Curve Stumble Runner " + std::to_string(Index + 1));
		Racer& Runner = Runners.back();
		ConfigureRacer(Runner, static_cast<unsigned int>(1200 + Index),
			100.0, 0.0, 100.0, 100.0, 0.8, 0.0, 0.5);
		Entrants.push_back(&Runner);
	}

	Race race;
	ASSERT_TRUE(ConfigureRace(race, RaceSeed, TrackLength));
	race.Racers = Entrants;
	ASSERT_EQ(RaceError::None, race.Precompute());

	const Racer* StumbleRunner = nullptr;
	const RacerRaceTimeline* StumbleTimeline = nullptr;
	const RaceTimelineNode* StumbleNode = nullptr;
	for (const Racer& Runner : Runners)
	{
		const RacerRaceTimeline* Timeline = race.GetTimelineFor(&Runner);
		ASSERT_TRUE(Timeline != nullptr);
		const auto FirstStumble = std::find_if(Timeline->Nodes.begin(), Timeline->Nodes.end(),
			[](const RaceTimelineNode& Node)
			{
				return Node.Type == RaceTimelineNodeType::RandomEvent &&
					Node.Event.Type == RaceEventType::Stumble;
			});
		if (FirstStumble == Timeline->Nodes.end() ||
			FirstStumble->EndTimeMs + StumbleSampleIntervalMs >= Timeline->FinishTimeMs)
			continue;

		const bool GassingBeforeRecoverySample = std::any_of(Timeline->Nodes.begin(), Timeline->Nodes.end(),
			[&FirstStumble](const RaceTimelineNode& Node)
			{
				return Node.Type == RaceTimelineNodeType::Gassing &&
					Node.StartTimeMs <= FirstStumble->EndTimeMs + StumbleSampleIntervalMs;
			});
		if (GassingBeforeRecoverySample)
			continue;

		StumbleRunner = &Runner;
		StumbleTimeline = Timeline;
		StumbleNode = &*FirstStumble;
		break;
	}
	ASSERT_TRUE(StumbleRunner != nullptr);
	ASSERT_TRUE(StumbleTimeline != nullptr);
	ASSERT_TRUE(StumbleNode != nullptr);
	ASSERT_TRUE(StumbleNode->AccelerationCurveTimeOffsetMs > 0.0);

	const double StumbleEndTimeMs = StumbleNode->EndTimeMs;
	RaceTimelineState AtStumbleStart;
	RaceTimelineState AtStumbleEnd;
	RaceTimelineState ContinuingAcceleration;
	ASSERT_TRUE(race.GetTimelineStateAt(StumbleRunner, StumbleNode->StartTimeMs, AtStumbleStart));
	ASSERT_TRUE(race.GetTimelineStateAt(StumbleRunner, StumbleEndTimeMs, AtStumbleEnd));
	ASSERT_TRUE(race.GetTimelineStateAt(StumbleRunner,
		StumbleEndTimeMs + StumbleSampleIntervalMs, ContinuingAcceleration));
	ASSERT_TRUE((AtStumbleStart.ActiveEffects & RaceEffectStumble) != 0);
	ASSERT_TRUE((AtStumbleEnd.ActiveEffects & RaceEffectStumble) == 0);

	const double StartProgress = (StumbleNode->StartTimeMs - ResponseDelayMs) / AccelerationDurationMs;
	const double EndProgress = (StumbleEndTimeMs - ResponseDelayMs) / AccelerationDurationMs;
	const double StartEndurancePaceMultiplier = EmptyEndurancePaceMultiplier +
		(1.0 - EmptyEndurancePaceMultiplier) * AtStumbleStart.Endurance;
	const double MaximumPace = StumbleNode->StartPace / (StumbleNode->Event.Magnitude *
		StumbleRunner->AccelerationCurve.Evaluate(StartProgress) * StartEndurancePaceMultiplier);
	const double EndEndurancePaceMultiplier = EmptyEndurancePaceMultiplier +
		(1.0 - EmptyEndurancePaceMultiplier) * AtStumbleEnd.Endurance;
	const double UnstumbledPaceAtEnd = MaximumPace * StumbleRunner->AccelerationCurve.Evaluate(EndProgress) *
		EndEndurancePaceMultiplier;
	ASSERT_TRUE(AtStumbleEnd.Pace < UnstumbledPaceAtEnd);
	ASSERT_TRUE(ContinuingAcceleration.Pace > AtStumbleEnd.Pace);

	const unsigned int StumbleEndPlaybackTimeMs = static_cast<unsigned int>(std::ceil(StumbleEndTimeMs));
	race.Tick(StumbleEndPlaybackTimeMs, RaceStatus::Racing);
	RaceTimelineState PlaybackAtStumbleEnd;
	ASSERT_TRUE(race.GetTimelineStateAt(StumbleRunner, race.GetCurrentTimeMs(), PlaybackAtStumbleEnd));
	ASSERT_EQ(PlaybackAtStumbleEnd.Pace, StumbleRunner->Pos.Velocity);
	race.Tick(StumbleSampleIntervalMs, RaceStatus::Racing);
	RaceTimelineState PlaybackDuringAcceleration;
	ASSERT_TRUE(race.GetTimelineStateAt(StumbleRunner, race.GetCurrentTimeMs(), PlaybackDuringAcceleration));
	ASSERT_EQ(PlaybackDuringAcceleration.Pace, StumbleRunner->Pos.Velocity);
	ASSERT_TRUE(PlaybackDuringAcceleration.Pace > AtStumbleEnd.Pace);
}

TEST_CASE(Race_SecondWindRestoresUpToHalfEndurance)
{
	constexpr unsigned int RacerInitializationSeed = 75;
	constexpr unsigned int RacerGuid = 1000;
	constexpr unsigned int RaceSeed = 72;
	constexpr unsigned int TrackLength = 1000000;
	constexpr unsigned int SecondWindDisplayTimeMs = 500;
	constexpr double MaximumRestoredEndurance = 0.5;
	constexpr std::size_t SecondWindRunnerCount = 2048;
	srand(RacerInitializationSeed);
	std::vector<Racer> Runners;
	Runners.reserve(SecondWindRunnerCount);
	std::vector<Racer*> Entrants;
	Entrants.reserve(SecondWindRunnerCount);
	for (std::size_t Index = 0; Index < SecondWindRunnerCount; ++Index)
	{
		Runners.emplace_back("Second Wind Runner " + std::to_string(Index + 1));
		Racer& Runner = Runners.back();
		ConfigureRacer(Runner, RacerGuid + static_cast<unsigned int>(Index),
			100.0, 100.0, 0.0, 100.0, 1.0, 100.0);
		Runner.Skills.Composure = 100.0;
		Entrants.push_back(&Runner);
	}
	Race race;
	ASSERT_TRUE(ConfigureRace(race, RaceSeed, TrackLength));
	race.Racers = Entrants;
	ASSERT_EQ(RaceError::None, race.Precompute());

	Racer* SecondWindRunner = nullptr;
	const RacerRaceTimeline* Timeline = nullptr;
	const RaceTimelineNode* SecondWindNode = nullptr;
	for (Racer& Runner : Runners)
	{
		const RacerRaceTimeline* CandidateTimeline = race.GetTimelineFor(&Runner);
		ASSERT_TRUE(CandidateTimeline != nullptr);
		const auto CandidateNode = std::find_if(CandidateTimeline->Nodes.begin(), CandidateTimeline->Nodes.end(),
			[](const RaceTimelineNode& Node)
			{
				return Node.Type == RaceTimelineNodeType::RandomEvent &&
					Node.Event.Type == RaceEventType::SecondWind;
			});
		if (CandidateNode == CandidateTimeline->Nodes.end())
			continue;

		SecondWindRunner = &Runner;
		Timeline = CandidateTimeline;
		SecondWindNode = &*CandidateNode;
		break;
	}
	ASSERT_TRUE(SecondWindRunner != nullptr);
	ASSERT_TRUE(Timeline != nullptr);
	ASSERT_TRUE(SecondWindNode != nullptr);
	ASSERT_EQ(MaximumRestoredEndurance, SecondWindNode->Event.Magnitude);

	RaceTimelineState AtSecondWindStart;
	ASSERT_TRUE(race.GetTimelineStateAt(SecondWindRunner, SecondWindNode->Event.TimeMs, AtSecondWindStart));
	ASSERT_EQ(MaximumRestoredEndurance, AtSecondWindStart.Endurance);
	ASSERT_TRUE((AtSecondWindStart.ActiveEffects & RaceEffectSecondWind) != 0);
	const unsigned int SecondWindPlaybackTimeMs =
		static_cast<unsigned int>(std::ceil(SecondWindNode->Event.TimeMs));
	RaceStatus status = RaceStatus::Racing;
	status = race.Tick(SecondWindPlaybackTimeMs, status);
	ASSERT_EQ(SecondWindPlaybackTimeMs, race.GetCurrentTimeMs());
	ASSERT_TRUE(std::abs(MaximumRestoredEndurance - race.GetCurrentEndurance(SecondWindRunner)) < 1e-4);
	ASSERT_TRUE((race.GetActiveEffects(SecondWindRunner) & RaceEffectSecondWind) != 0);

	race.Tick(SecondWindDisplayTimeMs, status);
	ASSERT_TRUE((race.GetActiveEffects(SecondWindRunner) & RaceEffectSecondWind) != 0);
}

TEST_CASE(Race_ThoughtEventsRequireExtremeConfidence)
{
	constexpr std::size_t ExtremeConfidenceRunnerCount = 8;
	constexpr double LowConfidence = 0.19;
	constexpr double LowConfidenceThreshold = 0.2;
	constexpr double HighConfidenceThreshold = 0.8;
	constexpr double HighConfidence = 0.81;
	srand(73);
	std::vector<Racer> LowConfidenceRunners;
	LowConfidenceRunners.reserve(ExtremeConfidenceRunnerCount);
	std::vector<Racer> HighConfidenceRunners;
	HighConfidenceRunners.reserve(ExtremeConfidenceRunnerCount);
	Racer LowThresholdRunner("Low Threshold");
	Racer HighThresholdRunner("High Threshold");
	Race race;
	ASSERT_TRUE(ConfigureRace(race, 72, 60000));
	for (std::size_t Index = 0; Index < ExtremeConfidenceRunnerCount; ++Index)
	{
		LowConfidenceRunners.emplace_back("Low Confidence " + std::to_string(Index + 1));
		Racer& LowConfidenceRunner = LowConfidenceRunners.back();
		ConfigureRacer(LowConfidenceRunner, static_cast<unsigned int>(7100 + Index),
			100.0, 100.0, 100.0, 100.0, LowConfidence);
		LowConfidenceRunner.Skills.Composure = 100.0;
		race.Racers.push_back(&LowConfidenceRunner);

		HighConfidenceRunners.emplace_back("High Confidence " + std::to_string(Index + 1));
		Racer& HighConfidenceRunner = HighConfidenceRunners.back();
		ConfigureRacer(HighConfidenceRunner, static_cast<unsigned int>(7200 + Index),
			100.0, 100.0, 100.0, 100.0, HighConfidence);
		HighConfidenceRunner.Skills.Composure = 100.0;
		race.Racers.push_back(&HighConfidenceRunner);
	}
	ConfigureRacer(LowThresholdRunner, 7300, 100.0, 100.0, 100.0, 100.0, LowConfidenceThreshold);
	ConfigureRacer(HighThresholdRunner, 7301, 100.0, 100.0, 100.0, 100.0, HighConfidenceThreshold);
	LowThresholdRunner.Skills.Composure = 100.0;
	HighThresholdRunner.Skills.Composure = 100.0;
	race.Racers.push_back(&LowThresholdRunner);
	race.Racers.push_back(&HighThresholdRunner);
	ASSERT_EQ(RaceError::None, race.Sim().Error);

	bool observedLostBelief = false;
	bool observedBeliefInSelf = false;
	for (const RaceEvent& Event : race.GetEventsThroughPlaybackTime())
	{
		if (Event.Type == RaceEventType::LostBelief)
		{
			const bool IsLowConfidenceRunner = std::any_of(LowConfidenceRunners.begin(),
				LowConfidenceRunners.end(), [&Event](const Racer& Runner)
				{
					return Event.Entrant == &Runner;
				});
			ASSERT_TRUE(IsLowConfidenceRunner);
			observedLostBelief = true;
		}
		else if (Event.Type == RaceEventType::BelievesInSelf)
		{
			const bool IsHighConfidenceRunner = std::any_of(HighConfidenceRunners.begin(),
				HighConfidenceRunners.end(), [&Event](const Racer& Runner)
				{
					return Event.Entrant == &Runner;
				});
			ASSERT_TRUE(IsHighConfidenceRunner);
			observedBeliefInSelf = true;
		}
	}
	ASSERT_TRUE(observedLostBelief);
	ASSERT_TRUE(observedBeliefInSelf);
}

TEST_CASE(RaceFinancials_CalculatesLowerPrizesFromFirstPlace)
{
	RaceFinancials financials(1000, 50);

	ASSERT_EQ(1000u, financials.FirstPlacePrize);
	ASSERT_EQ(500u, financials.SecondPlacePrize);
	ASSERT_EQ(250u, financials.ThirdPlacePrize);
	ASSERT_EQ(50u, financials.EntranceFee);
}

TEST_CASE(Racer_GeneratedDetailsAndConditions)
{
	constexpr double PreviousGeneratedEnduranceCeiling = 60.0;
	srand(17);
	double totalHeight = 0.0, totalWeight = 0.0, totalHeightSquared = 0.0, totalWeightSquared = 0.0, totalHeightWeight = 0.0;
	double totalReflex = 0.0, totalComposure = 0.0, totalAcceleration = 0.0, totalEndurance = 0.0, totalSpeed = 0.0;
	double totalPotential = 0.0, totalTrainability = 0.0, totalLuck = 0.0;
	unsigned int ReflexCenterBandCount = 0;
	unsigned int ReflexLowerTailCount = 0;
	unsigned int ReflexUpperTailCount = 0;
	bool GeneratedEnduranceAbovePreviousCeiling = false;
	for (int i = 0; i < 1000; ++i)
	{
		Racer racer;
		ASSERT_TRUE(racer.Details.HeightCm > 0);
		ASSERT_TRUE(racer.Details.WeightKg > 0);
		ASSERT_TRUE(racer.Details.Age >= 18);
		ASSERT_TRUE(racer.Details.Age <= 40);
		ASSERT_EQ(0.5, racer.Conditions.Confidence);
		ASSERT_EQ(0.0, racer.Conditions.Fatigue);
		ASSERT_EQ(0.0, racer.Conditions.Injury);
		ASSERT_TRUE(racer.Skills.Reflex >= 20 && racer.Skills.Reflex <= 100);
		ASSERT_TRUE(racer.Skills.Composure >= 20 && racer.Skills.Composure <= 100);
		ASSERT_TRUE(racer.Skills.Potential >= 70 && racer.Skills.Potential <= 100);
		ASSERT_TRUE(racer.Skills.Acceleration <= racer.Skills.Potential);
		ASSERT_TRUE(racer.Skills.Endurance <= racer.Skills.Potential);
		GeneratedEnduranceAbovePreviousCeiling |= racer.Skills.Endurance > PreviousGeneratedEnduranceCeiling;
		ASSERT_TRUE(racer.Skills.Speed <= racer.Skills.Potential);
		ASSERT_TRUE(racer.Skills.Acceleration >= 20 && racer.Skills.Endurance >= 20 && racer.Skills.Speed >= 20);
		ASSERT_TRUE(racer.Skills.Trainability >= 1 && racer.Skills.Trainability <= 100);
		ASSERT_TRUE(racer.Skills.Luck >= 0 && racer.Skills.Luck <= 100);
		ASSERT_EQ(0u, racer.Skills.Xp);
		totalReflex += racer.Skills.Reflex;
		totalComposure += racer.Skills.Composure;
		totalAcceleration += racer.Skills.Acceleration;
		totalEndurance += racer.Skills.Endurance;
		totalSpeed += racer.Skills.Speed;
		totalPotential += racer.Skills.Potential;
		totalTrainability += racer.Skills.Trainability;
		totalLuck += racer.Skills.Luck;
		ReflexCenterBandCount += racer.Skills.Reflex >= 40.0 && racer.Skills.Reflex <= 60.0;
		ReflexLowerTailCount += racer.Skills.Reflex >= 20.0 && racer.Skills.Reflex <= 30.0;
		ReflexUpperTailCount += racer.Skills.Reflex >= 70.0 && racer.Skills.Reflex <= 80.0;

		totalHeight += racer.Details.HeightCm;
		totalWeight += racer.Details.WeightKg;
		totalHeightSquared += racer.Details.HeightCm * racer.Details.HeightCm;
		totalWeightSquared += racer.Details.WeightKg * racer.Details.WeightKg;
		totalHeightWeight += racer.Details.HeightCm * racer.Details.WeightKg;
	}
	ASSERT_TRUE(GeneratedEnduranceAbovePreviousCeiling);

	const double sampleCount = 1000.0;
	ASSERT_TRUE(totalReflex / sampleCount > 48.0 && totalReflex / sampleCount < 52.0);
	ASSERT_TRUE(totalComposure / sampleCount > 48.0 && totalComposure / sampleCount < 52.0);
	ASSERT_TRUE(totalAcceleration / sampleCount > 47.0 && totalAcceleration / sampleCount < 53.0);
	ASSERT_TRUE(totalEndurance / sampleCount > 47.0 && totalEndurance / sampleCount < 53.0);
	ASSERT_TRUE(totalSpeed / sampleCount > 47.0 && totalSpeed / sampleCount < 53.0);
	ASSERT_TRUE(totalTrainability / sampleCount > 48.0 && totalTrainability / sampleCount < 52.0);
	ASSERT_TRUE(totalLuck / sampleCount > 48.0 && totalLuck / sampleCount < 52.0);
	ASSERT_TRUE(totalPotential / sampleCount > 83.0 && totalPotential / sampleCount < 87.0);
	ASSERT_TRUE(ReflexCenterBandCount > ReflexLowerTailCount * 2);
	ASSERT_TRUE(ReflexCenterBandCount > ReflexUpperTailCount * 2);
	const double correlation = (sampleCount * totalHeightWeight - totalHeight * totalWeight) /
		std::sqrt((sampleCount * totalHeightSquared - totalHeight * totalHeight) *
			(sampleCount * totalWeightSquared - totalWeight * totalWeight));
	ASSERT_TRUE(totalHeight / sampleCount > 170.0 && totalHeight / sampleCount < 178.0);
	ASSERT_TRUE(totalWeight / sampleCount > 63.0 && totalWeight / sampleCount < 71.0);
	ASSERT_TRUE(correlation > 0.6);
}

TEST_CASE(RaceCurve_ReproducibleMonotonicAndBounded)
{
	constexpr double MinimumAccelerationP1Y = 0.4;
	for (unsigned int seed = 0; seed < 100; ++seed)
	{
		for (bool descending : { false, true })
		{
			auto curve = RaceCurve::Generate(seed, descending);
			auto repeat = RaceCurve::Generate(seed, descending);
			ASSERT_EQ(curve.X1, repeat.X1);
			ASSERT_EQ(curve.X2, repeat.X2);
			ASSERT_EQ(curve.Y1, repeat.Y1);
			ASSERT_EQ(curve.Y2, repeat.Y2);
			ASSERT_TRUE(curve.X1 >= 0 && curve.X1 <= curve.X2 && curve.X2 <= 1);
			ASSERT_TRUE(curve.Y1 >= 0 && curve.Y1 <= 1 && curve.Y2 >= 0 && curve.Y2 <= 1);
			ASSERT_TRUE(descending || curve.Y1 >= MinimumAccelerationP1Y);
			ASSERT_EQ(descending ? 1.0 : 0.0, curve.Evaluate(0));
			ASSERT_EQ(descending ? 0.0 : 1.0, curve.Evaluate(1));
			double previous = curve.Evaluate(0);
			for (int i = 1; i <= 100; ++i)
			{
				double value = curve.Evaluate(i / 100.0);
				ASSERT_TRUE(descending ? value <= previous : value >= previous);
				previous = value;
			}
		}
	}
	ASSERT_TRUE(RaceCurve::Generate(1, false).X1 != RaceCurve::Generate(2, false).X1);
}

TEST_CASE(RaceCurve_IntegrateAndValueInverseAreConsistent)
{
	constexpr double OneThird = 1.0 / 3.0;
	const RaceCurve LinearAscending{ OneThird, OneThird, 2.0 * OneThird, 2.0 * OneThird, false };
	for (double Value : { 0.1, 0.25, 0.5, 0.9 })
	{
		const double Time = LinearAscending.TimeAtValue(Value);
		ASSERT_TRUE(std::abs(Time - Value) < 1e-10);
		ASSERT_TRUE(std::abs(LinearAscending.Evaluate(Time) - Value) < 1e-10);
	}
	ASSERT_TRUE(std::abs(LinearAscending.Integrate(0.0, 1.0) - 0.5) < 1e-12);
	ASSERT_TRUE(std::abs(LinearAscending.Integrate(0.2, 0.8) - 0.3) < 1e-12);
	ASSERT_TRUE(std::abs(LinearAscending.Integrate(0.8, 0.2) + 0.3) < 1e-12);

	const RaceCurve LinearDescending{ OneThird, 2.0 * OneThird, 2.0 * OneThird, OneThird, true };
	const double DescendingTime = LinearDescending.TimeAtValue(0.37);
	ASSERT_TRUE(std::abs(DescendingTime - 0.63) < 1e-10);
	ASSERT_TRUE(std::abs(LinearDescending.Integrate(0.2, 0.8) - 0.3) < 1e-12);

	const RaceCurve Nonlinear{ 0.12, 0.2, 0.84, 0.9, false };
	constexpr unsigned int IntegrationIntervals = 2000;
	const double Step = 1.0 / IntegrationIntervals;
	double SimpsonSum = Nonlinear.Evaluate(0.0) + Nonlinear.Evaluate(1.0);
	for (unsigned int Index = 1; Index < IntegrationIntervals; ++Index)
		SimpsonSum += (Index % 2 == 0 ? 2.0 : 4.0) * Nonlinear.Evaluate(Index * Step);
	const double NumericalIntegral = SimpsonSum * Step / 3.0;
	ASSERT_TRUE(std::abs(Nonlinear.Integrate(0.0, 1.0) - NumericalIntegral) < 1e-8);
	for (double Value : { 0.1, 0.35, 0.7, 0.95 })
	{
		const double Time = Nonlinear.TimeAtValue(Value);
		ASSERT_TRUE(std::abs(Nonlinear.Evaluate(Time) - Value) < 1e-10);
	}
}

int main()
{
	return Test::RunAll();
}