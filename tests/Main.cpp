#include "TestHarness.hpp"
#include "App\CommandLine.hpp"
#include "Race.h"
#include "Racer.h"

#include <cstdlib>
#include <string>

using namespace Game::App;
using namespace Game::Race;

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

TEST_CASE(Race_TickFinishesRacerPastTrackLength)
{
	srand(4);
	Racer racer("Winner");
	racer.GUID = 10;
	racer.Pos.X = static_cast<unsigned int>(TrackLength::Standard100m) + 1;

	Race race;
	race.GUID.GUID = 99;
	race.Racers.push_back(&racer);

	auto status = race.Tick(33, RaceStatus::Racing);

	ASSERT_TRUE(status == RaceStatus::Finishing);
	ASSERT_EQ(1u, race.FinishedCount());
	ASSERT_TRUE(race.HasFinished(&racer));
	ASSERT_EQ(1u, race.Result.RacerResults[0].Position);
	ASSERT_EQ(99u, race.Result.RacerResults[0].GUID.GUID);
}

TEST_CASE(RaceFinancials_CalculatesLowerPrizesFromFirstPlace)
{
	RaceFinancials financials(1000, 50);

	ASSERT_EQ(1000u, financials.FirstPlacePrize);
	ASSERT_EQ(500u, financials.SecondPlacePrize);
	ASSERT_EQ(250u, financials.ThirdPlacePrize);
	ASSERT_EQ(50u, financials.EntranceFee);
}

int main()
{
	return Test::RunAll();
}