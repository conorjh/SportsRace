#include "TestHarness.hpp"
#include "App\CommandLine.hpp"
#include "Race.h"
#include "Racer.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
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
	Track* OriginalTrack = race.ThisTrack;
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
		return Entry.Type == RaceTimelineNodeType::Gassing;
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
	ASSERT_TRUE(ConfigureRace(race, RaceSeed, 4000));
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
	constexpr double FullEnduranceDurationMs = 22500.0;
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

TEST_CASE(Race_LowerEnduranceLetsSomeRacersGasOnStandardTrack)
{
	constexpr unsigned int RaceSeed = 56;
	constexpr unsigned int SlowRacerGuid = 57;
	constexpr unsigned int FastRacerGuid = 58;
	constexpr double LowEndurance = 20.0;
	constexpr double HighEndurance = 100.0;
	Racer SlowRunner("Slow Runner");
	Racer FastRunner("Fast Runner");
	ConfigureRacer(SlowRunner, SlowRacerGuid, 100.0, 100.0, LowEndurance, 40.0);
	ConfigureRacer(FastRunner, FastRacerGuid, 100.0, 100.0, HighEndurance, 100.0);
	Race race;
	ASSERT_TRUE(ConfigureRace(race, RaceSeed, static_cast<unsigned int>(TrackLength::Standard100m)));
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
	constexpr unsigned int PlaybackBeforeOldLowEnduranceGassingMs = 32000;
	constexpr double GassingDetectionStepMs = 50.0;
	constexpr double GassingEnduranceThreshold = 0.1;
	constexpr double ExpectedLowEnduranceFullDurationMs = 40500.0;
	constexpr double ExpectedHighEnduranceFullDurationMs = 46500.0;
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

	LowEnduranceRace.Tick(PlaybackBeforeOldLowEnduranceGassingMs, RaceStatus::Racing);
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
	constexpr unsigned int TrackLength = 100000;
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
	ASSERT_EQ(FastAcceleration->EndTimeMs, SlowAcceleration->EndTimeMs);
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
	constexpr unsigned int PlaybackTimeMs = 500;
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
	constexpr double FullEnduranceDurationMs = 22500.0;
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
	secondRace.Sim();

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
	constexpr std::size_t SecondWindRunnerCount = 512;
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
	RaceStatus status = RaceStatus::Racing;
	for (unsigned int Tick = 0; Tick < 36000 && !race.IsComplete(); ++Tick)
	{
		status = race.Tick(50, status);
		auto visibleEvents = race.GetEventsThroughPlaybackTime();
		for (const RaceEvent& Event : visibleEvents)
		{
			if (Event.Type == RaceEventType::Stumble && Event.Entrant != nullptr &&
				Event.TimeMs <= race.GetCurrentTimeMs() &&
				race.GetCurrentTimeMs() < Event.TimeMs + Event.DurationMs &&
				(race.GetActiveEffects(Event.Entrant) & RaceEffectStumble) != 0)
				observedStumbleEffect = true;

			if (Event.Type == RaceEventType::SecondWind && Event.Entrant != nullptr &&
				Event.TimeMs == race.GetCurrentTimeMs())
			{
				ASSERT_TRUE((race.GetActiveEffects(Event.Entrant) & RaceEffectSecondWind) != 0);
				const RaceEvent* LastGassing = nullptr;
				for (const RaceEvent& Candidate : visibleEvents)
				{
					if (Candidate.Type == RaceEventType::Gassing && Candidate.Entrant == Event.Entrant &&
						Candidate.TimeMs <= Event.TimeMs &&
						(LastGassing == nullptr || Candidate.TimeMs > LastGassing->TimeMs))
						LastGassing = &Candidate;
				}
				if (LastGassing != nullptr &&
					std::abs(Event.Entrant->Pos.Velocity - LastGassing->Magnitude) < 1e-9)
					observedSecondWindPace = true;
			}
			if (Event.Type == RaceEventType::SecondWind && Event.Entrant != nullptr &&
				Event.TimeMs + 500.0 == race.GetCurrentTimeMs() &&
				(race.GetActiveEffects(Event.Entrant) & RaceEffectSecondWind) != 0)
			{
				observedSecondWindLabelEffect = true;
			}
		}
	}

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

TEST_CASE(Race_SecondWindRestoresUpToHalfEndurance)
{
	constexpr unsigned int RacerInitializationSeed = 75;
	constexpr unsigned int RacerGuid = 76;
	constexpr unsigned int RaceSeed = 77;
	constexpr unsigned int TrackLength = 1000000;
	constexpr unsigned int SecondWindDisplayTimeMs = 500;
	constexpr double MaximumRestoredEndurance = 0.5;
	constexpr std::size_t SecondWindRunnerCount = 512;
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

	RaceStatus status = RaceStatus::Racing;
	status = race.Tick(static_cast<unsigned int>(SecondWindNode->Event.TimeMs), status);
	ASSERT_EQ(static_cast<unsigned int>(SecondWindNode->Event.TimeMs), race.GetCurrentTimeMs());
	ASSERT_EQ(MaximumRestoredEndurance, race.GetCurrentEndurance(SecondWindRunner));
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

int main()
{
	return Test::RunAll();
}