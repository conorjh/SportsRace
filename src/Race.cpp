#include "Race.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <random>
#include <utility>

namespace
{
	constexpr unsigned int SimulationStepMs = 50;
	constexpr unsigned int EventIntervalMs = 1000;
	constexpr unsigned int MillisecondsPerSecond = 1000;
	constexpr unsigned int SecondsPerMinute = 60;
	constexpr unsigned int MaximumRaceDurationMinutes = 30;
	constexpr unsigned int MaximumRaceDurationMs = MaximumRaceDurationMinutes * SecondsPerMinute * MillisecondsPerSecond;
	constexpr double MaximumRacerRating = 100.0;
	constexpr double ConfidencePerformanceBonusScale = 0.1;
	constexpr double FatiguePerformancePenalty = 0.45;
	constexpr double InjuryPerformancePenalty = 0.35;
	constexpr double PaceUnitsPerSpeedPoint = 5.0;
	constexpr double MinimumReflexResponseDelayMs = 10.0;
	constexpr double MaximumReflexResponseDelayMs = 1000.0;
	constexpr double ReflexDelayReductionMsPerPoint =
		(MaximumReflexResponseDelayMs - MinimumReflexResponseDelayMs) / MaximumRacerRating;
	constexpr double MinimumAccelerationDurationMs = 1500.0;
	constexpr double MaximumAccelerationDurationMs = 6000.0;
	constexpr double AccelerationDurationReductionMsPerPoint = 45.0;
	constexpr unsigned int AccelerationCurveRewindIterations = 32;
	constexpr double BaseEnduranceDurationMs = 90000.0;
	constexpr double EnduranceDurationMsPerPoint = 1200.0;
	constexpr double EnduranceDurationReferenceRating = 80.0;
	constexpr double EmptyEndurancePaceMultiplier = 0.2;
	constexpr double TrapezoidalIntegrationFactor = 0.5;
	constexpr double MaximumFatigueGain = 0.6;
	constexpr double BaseFatigueGain = 0.1;
	constexpr double FatigueGainPerSecond = 0.004;
	constexpr double MinimumComposureMultiplier = 0.5;
	constexpr double WinnerConfidenceChange = 0.08;
	constexpr double NonWinnerConfidenceChange = -0.03;
	constexpr double AnimationBaseFrameTimeMs = 350.0;
	constexpr double AnimationFrameTimeReductionPerPace = 0.1;
	constexpr double MinimumAnimationFrameTimeMs = 80.0;
	constexpr unsigned int AnimationFrameCount = 4;
	constexpr unsigned int SecondPlacePrizeDivisor = 2;
	constexpr unsigned int ThirdPlacePrizeDivisor = 4;
	constexpr unsigned int SeedMixInitialShift = 16;
	constexpr unsigned int SeedMixMiddleShift = 15;
	constexpr std::uint32_t RacerIndexSeedSalt = 0x9e3779b9u;
	constexpr std::uint32_t SeedMixFirstMultiplier = 0x7feb352du;
	constexpr std::uint32_t SeedMixSecondMultiplier = 0x846ca68bu;

	double Clamp01(double Value)
	{
		return std::clamp(std::isfinite(Value) ? Value : 0.0, 0.0, 1.0);
	}

	double ClampRating(double Value)
	{
		return std::clamp(std::isfinite(Value) ? Value : 0.0, 0.0, MaximumRacerRating);
	}

	double EvaluateEnduranceReserve(const Game::Race::RaceCurve& Curve, double TimeMs, double CycleStartMs,
		double CycleDurationMs, double CycleReserve)
	{
		// Restart the endurance curve from the reserve available at this cycle's start.
		const double DurationMs = std::max<double>(SimulationStepMs, CycleDurationMs);
		const double Progress = (TimeMs - CycleStartMs) / DurationMs;
		return CycleReserve * Curve.Evaluate(Progress);
	}

	double ApplyEndurancePaceMultiplier(double Pace, double EnduranceReserve)
	{
		const double NormalizedReserve = std::clamp(EnduranceReserve, 0.0, 1.0);
		const double PaceMultiplier = EmptyEndurancePaceMultiplier +
			(1.0 - EmptyEndurancePaceMultiplier) * NormalizedReserve;
		return std::max(0.0, Pace) * PaceMultiplier;
	}

	unsigned int MixSeed(unsigned int RaceSeed, unsigned int RacerGuid, std::size_t RacerIndex)
	{
		// Keep each entrant's random stream reproducible for a race while distinct from other entrants.
		std::uint32_t Value = RaceSeed ^ RacerGuid ^ (static_cast<std::uint32_t>(RacerIndex) + RacerIndexSeedSalt);
		Value ^= Value >> SeedMixInitialShift;
		Value *= SeedMixFirstMultiplier;
		Value ^= Value >> SeedMixMiddleShift;
		Value *= SeedMixSecondMultiplier;
		Value ^= Value >> SeedMixInitialShift;
		return Value;
	}

	struct EffectiveRacerStats
	{
		double Confidence = 0.0;
		double Fatigue = 0.0;
		double Injury = 0.0;
		double Luck = 0.0;
		double MaximumPace = 0.0;
		double ResponseDelayMs = 0.0;
		double AccelerationDurationMs = 0.0;
		double FullEnduranceDurationMs = 0.0;
	};

	EffectiveRacerStats CalculateEffectiveRacerStats(const Game::Race::Racer& Runner,
		double ConfidenceValue, double FatigueValue, double InjuryValue)
	{
		using namespace Game::Race;

		EffectiveRacerStats Stats;
		Stats.Confidence = Clamp01(ConfidenceValue);
		Stats.Fatigue = Clamp01(FatigueValue);
		Stats.Injury = Clamp01(InjuryValue);
		// Apply race-day conditions before deriving the ratings used for pace and event chances.
		const double ConditionScale = (1.0 + (Stats.Confidence - RacerConditions::InitialConfidence) * ConfidencePerformanceBonusScale) *
			(1.0 - Stats.Fatigue * FatiguePerformancePenalty) * (1.0 - Stats.Injury * InjuryPerformancePenalty);
		const double EffectiveReflex = ClampRating(ClampRating(Runner.Skills.Reflex) * ConditionScale);
		const double EffectiveAcceleration = ClampRating(ClampRating(Runner.Skills.Acceleration) * ConditionScale);
		const double EnduranceRating = ClampRating(Runner.Skills.Endurance);
		Stats.MaximumPace = ClampRating(ClampRating(Runner.Skills.Speed) * ConditionScale) * PaceUnitsPerSpeedPoint;
		Stats.Luck = Clamp01(ClampRating(Runner.Skills.Luck) * ConditionScale / MaximumRacerRating);
		Stats.ResponseDelayMs = std::clamp(MaximumReflexResponseDelayMs - EffectiveReflex * ReflexDelayReductionMsPerPoint,
			MinimumReflexResponseDelayMs, MaximumReflexResponseDelayMs);
		Stats.AccelerationDurationMs = std::clamp(MaximumAccelerationDurationMs - EffectiveAcceleration * AccelerationDurationReductionMsPerPoint,
			MinimumAccelerationDurationMs, MaximumAccelerationDurationMs);
		const double DurationEnduranceRating = EnduranceRating < EnduranceDurationReferenceRating
			? EnduranceRating + EnduranceRating * (EnduranceDurationReferenceRating - EnduranceRating) /
			EnduranceDurationReferenceRating
			: EnduranceRating;
		const double EffectiveDurationEndurance = ClampRating(DurationEnduranceRating * ConditionScale);
		Stats.FullEnduranceDurationMs = BaseEnduranceDurationMs +
			EffectiveDurationEndurance * EnduranceDurationMsPerPoint;
		return Stats;
	}

	EffectiveRacerStats CalculateEffectiveRacerStats(const Game::Race::Racer& Runner)
	{
		return CalculateEffectiveRacerStats(Runner, Runner.Conditions.Confidence,
			Runner.Conditions.Fatigue, Runner.Conditions.Injury);
	}

	double AccelerationCurveTimeOffsetAfterStumble(const Game::Race::Racer& Runner,
		const EffectiveRacerStats& Stats, double StumbleEndTimeMs, double CurrentTimeOffsetMs,
		double StumblePaceMultiplier)
	{
		if (Stats.MaximumPace <= 0.0 || Stats.AccelerationDurationMs <= 0.0)
			return CurrentTimeOffsetMs;

		const double CurveTimeMs = StumbleEndTimeMs - CurrentTimeOffsetMs;
		const double CurveProgress = std::clamp((CurveTimeMs - Stats.ResponseDelayMs) /
			Stats.AccelerationDurationMs, 0.0, 1.0);
		if (CurveProgress == 0.0)
			return CurrentTimeOffsetMs;

		const double TargetPace = Runner.AccelerationCurve.Evaluate(CurveProgress) *
			Clamp01(StumblePaceMultiplier);
		// Find the curve progress that matches the reduced pace so recovery does not jump back to normal pace.
		double LowerProgress = 0.0;
		double UpperProgress = CurveProgress;
		for (unsigned int Iteration = 0; Iteration < AccelerationCurveRewindIterations; ++Iteration)
		{
			const double MiddleProgress = (LowerProgress + UpperProgress) * 0.5;
			if (Runner.AccelerationCurve.Evaluate(MiddleProgress) < TargetPace)
				LowerProgress = MiddleProgress;
			else
				UpperProgress = MiddleProgress;
		}

		const double ReducedCurveTimeMs = Stats.ResponseDelayMs + UpperProgress * Stats.AccelerationDurationMs;
		return StumbleEndTimeMs - ReducedCurveTimeMs;
	}

	Game::Race::RaceEvent CreateRaceEvent(Game::Race::Racer* Runner, unsigned int RacerIndex,
		Game::Race::RaceEventType Type, double TimeMs, unsigned int DurationMs, double Magnitude)
	{
		Game::Race::RaceEvent Event;
		Event.Entrant = Runner;
		Event.RacerIndex = RacerIndex;
		Event.Type = Type;
		Event.TimeMs = TimeMs;
		Event.DurationMs = DurationMs;
		Event.Magnitude = Magnitude;
		return Event;
	}

	class GassingEvent
	{
	public:
		static constexpr double EnduranceThreshold = 0.1;
		static constexpr double DeclineDurationMs = 8000.0;
		static constexpr double PaceDeclineFraction = 0.8;

		bool ShouldStart(double EnduranceReserve) const
		{
			return EnduranceReserve <= EnduranceThreshold;
		}

		double HeldEnduranceReserve() const
		{
			return EnduranceThreshold;
		}

		double EntryPace(double NormalPace) const
		{
			return ApplyEndurancePaceMultiplier(NormalPace, EnduranceThreshold);
		}

		static double PaceAt(double EntryPace, double ElapsedTimeMs)
		{
			const double Decline = std::clamp(ElapsedTimeMs / DeclineDurationMs, 0.0, 1.0);
			return EntryPace * (1.0 - PaceDeclineFraction * Decline);
		}
	};

	class PullingUpEvent
	{
		static constexpr double MinimumGassingDurationMs = 1000.0;
		static constexpr double MaximumGassingDurationMs = 5000.0;

	public:
		double GassingTimeLimitMs(double Confidence, double Composure) const
		{
			const double NormalizedComposure = ClampRating(Composure) / MaximumRacerRating;
			const double Resilience = (NormalizedComposure + Clamp01(Confidence)) * 0.5;
			return MinimumGassingDurationMs +
				(MaximumGassingDurationMs - MinimumGassingDurationMs) * Resilience;
		}

		bool ShouldPullUp(double GassingDurationMs, double Confidence, double Composure) const
		{
			return GassingDurationMs >= GassingTimeLimitMs(Confidence, Composure);
		}
	};

	class StumbleEvent
	{
		static constexpr double BaseChance = 0.02;
		static constexpr double FatigueChanceWeight = 0.45;
		static constexpr double LowConfidenceChanceWeight = 0.35;
		static constexpr double ChanceScale = 0.2;
		static constexpr double MaximumChance = 0.85;
		static constexpr double MinimumPaceMultiplier = 0.4;
		static constexpr double MaximumPaceMultiplier = 0.65;
		static constexpr unsigned int MinimumDurationSteps = 6;
		static constexpr unsigned int MaximumDurationSteps = 18;

		double Chance(double Confidence, double Fatigue) const
		{
			return std::clamp(BaseChance + FatigueChanceWeight * Fatigue +
				LowConfidenceChanceWeight * (1.0 - Confidence), 0.0, MaximumChance) * ChanceScale;
		}

	public:
		bool TryRoll(double Confidence, double Fatigue, std::mt19937& Random,
			unsigned int& DurationMs, double& PaceMultiplier) const
		{
			std::uniform_real_distribution<double> Unit(0.0, 1.0);
			if (Unit(Random) >= Chance(Confidence, Fatigue))
				return false;

			std::uniform_int_distribution<unsigned int> DurationSteps(MinimumDurationSteps, MaximumDurationSteps);
			std::uniform_real_distribution<double> PaceMultipliers(MinimumPaceMultiplier, MaximumPaceMultiplier);
			DurationMs = DurationSteps(Random) * SimulationStepMs;
			PaceMultiplier = PaceMultipliers(Random);
			return true;
		}
	};

	class LostBeliefEvent
	{
		static constexpr double ConfidenceThreshold = 0.2;
		static constexpr double BaseChance = 0.02;
		static constexpr double ConfidenceChanceWeight = 0.40;
		static constexpr double ChanceScale = 0.1;
		static constexpr double MinimumPaceMultiplier = 0.85;
		static constexpr double MaximumPaceMultiplier = 0.93;
		static constexpr unsigned int MinimumDurationSteps = 40;
		static constexpr unsigned int MaximumDurationSteps = 80;

		double Chance(double Confidence) const
		{
			return Confidence < ConfidenceThreshold
				? (BaseChance + ConfidenceChanceWeight * (1.0 - Confidence)) * ChanceScale
				: 0.0;
		}

	public:
		double ChanceFor(double Confidence) const
		{
			return Chance(Confidence);
		}

		bool TryRoll(double Confidence, double Roll, std::mt19937& Random,
			unsigned int& DurationMs, double& PaceMultiplier) const
		{
			if (Roll >= Chance(Confidence))
				return false;

			std::uniform_int_distribution<unsigned int> DurationSteps(MinimumDurationSteps, MaximumDurationSteps);
			std::uniform_real_distribution<double> PaceMultipliers(MinimumPaceMultiplier, MaximumPaceMultiplier);
			DurationMs = DurationSteps(Random) * SimulationStepMs;
			PaceMultiplier = PaceMultipliers(Random);
			return true;
		}
	};

	class BelievesInSelfEvent
	{
		static constexpr double ConfidenceThreshold = 0.8;
		static constexpr double BaseChance = 0.02;
		static constexpr double ConfidenceChanceWeight = 0.40;
		static constexpr double ChanceScale = 0.1;
		static constexpr double MinimumPaceMultiplier = 1.03;
		static constexpr double MaximumPaceMultiplier = 1.08;
		static constexpr unsigned int MinimumDurationSteps = 40;
		static constexpr unsigned int MaximumDurationSteps = 80;

		double Chance(double Confidence) const
		{
			return Confidence > ConfidenceThreshold
				? (BaseChance + ConfidenceChanceWeight * Confidence) * ChanceScale
				: 0.0;
		}

	public:
		bool TryRoll(double Confidence, double Roll, std::mt19937& Random,
			double PreviousEventChance, unsigned int& DurationMs, double& PaceMultiplier) const
		{
			if (Roll >= PreviousEventChance + Chance(Confidence))
				return false;

			std::uniform_int_distribution<unsigned int> DurationSteps(MinimumDurationSteps, MaximumDurationSteps);
			std::uniform_real_distribution<double> PaceMultipliers(MinimumPaceMultiplier, MaximumPaceMultiplier);
			DurationMs = DurationSteps(Random) * SimulationStepMs;
			PaceMultiplier = PaceMultipliers(Random);
			return true;
		}
	};

	class SecondWindEvent
	{
		static constexpr double BaseTriggerChance = 0.001;
		static constexpr double ConfidenceChanceWeight = 0.0125;
		static constexpr double ChanceScale = 0.1;
		static constexpr double MaximumLuckChanceBonus = 0.0025;
		static constexpr double BaseRestoredReserve = 0.35;
		static constexpr double LuckRestoredReserveWeight = 0.05;
		static constexpr double ConfidenceRestoredReserveWeight = 0.1;
		static constexpr double MaximumRestoredReserve = 0.5;

		double Chance(double Confidence, double Luck) const
		{
			const double WeightedChance = (BaseTriggerChance + ConfidenceChanceWeight * Confidence) * ChanceScale;
			return std::clamp(WeightedChance + Clamp01(Luck) * MaximumLuckChanceBonus, 0.0, 1.0);
		}

	public:
		static constexpr double EffectDisplayDurationMs = 1000.0;
		static constexpr double PaceOverrideDurationMs = SimulationStepMs;

		bool TryRoll(double Confidence, double Luck, std::mt19937& Random) const
		{
			std::uniform_real_distribution<double> Unit(0.0, 1.0);
			return Unit(Random) < Chance(Confidence, Luck);
		}

		double RestoredReserve(double Confidence, double Luck) const
		{
			return std::min(MaximumRestoredReserve, BaseRestoredReserve +
				LuckRestoredReserveWeight * Luck + ConfidenceRestoredReserveWeight * Confidence);
		}

	};

	class InjuryEvent
	{
		static constexpr double BaseChance = 0.01;
		static constexpr double FatigueChanceWeight = 0.05;
		static constexpr double ChanceScale = 0.1;
		static constexpr double Gain = 0.1;
		static constexpr std::uint32_t SeedSalt = 0xa511e9b3u;

		double Chance(double Fatigue) const
		{
			return (BaseChance + FatigueChanceWeight * Fatigue) * ChanceScale;
		}

	public:
		std::mt19937 CreateRandom(unsigned int RaceSeed, unsigned int RacerGuid, std::size_t RacerIndex) const
		{
			return std::mt19937(MixSeed(RaceSeed, RacerGuid, RacerIndex) ^ SeedSalt);
		}

		bool TryRoll(double Fatigue, std::mt19937& Random) const
		{
			std::uniform_real_distribution<double> Unit(0.0, 1.0);
			return Unit(Random) < Chance(Fatigue);
		}

		double Magnitude() const
		{
			return Gain;
		}
	};

	unsigned int ToMilliseconds(double TimeMs)
	{
		if (TimeMs <= 0)
			return 0;
		return static_cast<unsigned int>(std::lround(TimeMs));
	}

	using TimelinePlaybackState = Game::Race::RaceTimelineState;

	TimelinePlaybackState EvaluateTimelinePlaybackState(const Game::Race::RacerRaceTimeline& Timeline, double TimeMs,
		bool IncludeEventsAtTime = true)
	{
		using namespace Game::Race;

		// Rebuild pace and effects from the precomputed nodes so playback can be sampled at any time.
		const Racer& Runner = *Timeline.Entrant;
		const EffectiveRacerStats Stats = CalculateEffectiveRacerStats(Runner,
			Timeline.PreRaceConfidence, Timeline.PreRaceFatigue, Timeline.PreRaceInjury);

		const bool IsAtFinish = Timeline.Finished && TimeMs >= Timeline.FinishTimeMs;
		const bool IsPulledUp = Timeline.PulledUp &&
			(TimeMs > Timeline.PullUpTimeMs || (IncludeEventsAtTime && TimeMs >= Timeline.PullUpTimeMs));
		const RaceTimelineNode* GassingNode = nullptr;
		const RaceTimelineNode* WindOverrideNode = nullptr;
		bool SecondWindActive = false;
		double StumbleMultiplier = 1.0;
		double AccelerationCurveTimeOffsetMs = 0.0;
		double LostBeliefMultiplier = 1.0;
		double BeliefMultiplier = 1.0;
		double EnduranceCycleStartMs = 0.0;
		double EnduranceCycleReserve = 1.0;

		// IncludeEventsAtTime selects which side of an exact event timestamp this sample represents.
		for (const RaceTimelineNode& Node : Timeline.Nodes)
		{
			if (Node.StartTimeMs > TimeMs || (!IncludeEventsAtTime && Node.StartTimeMs >= TimeMs))
				break;

			if (Node.Type == RaceTimelineNodeType::Gassing)
			{
				if (TimeMs < Node.EndTimeMs || (!IncludeEventsAtTime && TimeMs == Node.EndTimeMs) ||
					(IsAtFinish && TimeMs == Node.EndTimeMs))
					GassingNode = &Node;
				continue;
			}

			if (Node.Type != RaceTimelineNodeType::RandomEvent)
				continue;

			const RaceEvent& Event = Node.Event;
			if (Event.Type == RaceEventType::SecondWind)
			{
				EnduranceCycleStartMs = Event.TimeMs;
				EnduranceCycleReserve = Event.Magnitude;
				if (TimeMs < Event.TimeMs + SecondWindEvent::EffectDisplayDurationMs)
				{
					SecondWindActive = true;
				}
				if (TimeMs < Node.EndTimeMs)
					WindOverrideNode = &Node;
				continue;
			}
			if (Event.Type == RaceEventType::Stumble && TimeMs >= Node.EndTimeMs)
			{
				AccelerationCurveTimeOffsetMs = Node.AccelerationCurveTimeOffsetMs;
				continue;
			}

			if (TimeMs >= Node.EndTimeMs)
				continue;

			switch (Event.Type)
			{
			case RaceEventType::Stumble:
				StumbleMultiplier = Event.Magnitude;
				break;
			case RaceEventType::LostBelief:
				LostBeliefMultiplier = Event.Magnitude;
				break;
			case RaceEventType::BelievesInSelf:
				BeliefMultiplier = Event.Magnitude;
				break;
			default:
				break;
			}
		}

		const double AccelerationCurveTimeMs = TimeMs - AccelerationCurveTimeOffsetMs;
		const double NormalPace = AccelerationCurveTimeMs <= Stats.ResponseDelayMs
			? 0.0
			: Stats.MaximumPace * Runner.AccelerationCurve.Evaluate(
				(AccelerationCurveTimeMs - Stats.ResponseDelayMs) / Stats.AccelerationDurationMs);
		TimelinePlaybackState State;
		if (SecondWindActive)
		{
			State.ActiveEffects |= RaceEffectSecondWind;
		}
		if (IsPulledUp)
		{
			State.Endurance = GassingEvent::EnduranceThreshold;
		}
		else if (GassingNode != nullptr)
		{
			State.Endurance = GassingEvent::EnduranceThreshold;
		}
		else
		{
			const double EnduranceCycleDurationMs = std::max<double>(SimulationStepMs,
				Stats.FullEnduranceDurationMs * EnduranceCycleReserve);
			State.Endurance = EvaluateEnduranceReserve(Runner.EnduranceCurve, TimeMs, EnduranceCycleStartMs,
				EnduranceCycleDurationMs, EnduranceCycleReserve);
		}
		const double EnduranceAdjustedPace = ApplyEndurancePaceMultiplier(NormalPace, State.Endurance);
		if (GassingNode != nullptr)
		{
			State.Pace = GassingEvent::PaceAt(GassingNode->StartPace, TimeMs - GassingNode->StartTimeMs);
			State.ActiveEffects |= RaceEffectGassing;
		}
		else
		{
			State.Pace = EnduranceAdjustedPace;
		}

		// The brief second-wind override takes precedence; otherwise active pace effects multiply together.
		if (WindOverrideNode != nullptr)
			State.Pace = WindOverrideNode->StartPace;
		else
			State.Pace *= StumbleMultiplier * LostBeliefMultiplier * BeliefMultiplier;

		for (const RaceTimelineNode& Node : Timeline.Nodes)
		{
			if (Node.StartTimeMs > TimeMs || (!IncludeEventsAtTime && Node.StartTimeMs >= TimeMs))
				break;
			if (Node.Type != RaceTimelineNodeType::RandomEvent || TimeMs >= Node.EndTimeMs)
				continue;

			switch (Node.Event.Type)
			{
			case RaceEventType::Stumble:
				State.ActiveEffects |= RaceEffectStumble;
				break;
			case RaceEventType::LostBelief:
				State.ActiveEffects |= RaceEffectLostBelief;
				break;
			case RaceEventType::BelievesInSelf:
				State.ActiveEffects |= RaceEffectBelievesInSelf;
				break;
			default:
				break;
			}
		}

		if (IsPulledUp)
		{
			State.Pace = 0.0;
			State.ActiveEffects = RaceEffectPulledUp;
		}

		return State;
	}

	class RacerTimelinePrecomputer
	{
	public:
		RacerTimelinePrecomputer(Game::Race::Racer* Runner, std::size_t RacerIndex,
			unsigned int RaceSeed, unsigned int TrackLength)
			: Runner(Runner), RacerIndex(RacerIndex), LockedSeed(RaceSeed), TrackLength(TrackLength)
		{
		}

		Game::Race::RacerRaceTimeline Build()
		{
			// Precompute a complete outcome so playback can sample it without rolling events again.
			Runner->Reset();
			Stats = CalculateEffectiveRacerStats(*Runner);
			Timeline.PreRaceConfidence = Stats.Confidence;
			Timeline.PreRaceFatigue = Stats.Fatigue;
			Timeline.PreRaceInjury = Stats.Injury;
			EventRandom.seed(MixSeed(LockedSeed, Runner->GUID.GUID, RacerIndex));
			Timeline.Entrant = Runner;
			Timeline.RacerIndex = static_cast<unsigned int>(RacerIndex);
			InitializeTimelineNodes();
			Simulate();
			Timeline.FinalDistance = Distance;
			return std::move(Timeline);
		}

	private:
		void InitializeTimelineNodes()
		{
			using namespace Game::Race;

			RaceTimelineNode InitialDelayNode;
			InitialDelayNode.Type = RaceTimelineNodeType::InitialDelay;
			InitialDelayNode.EndTimeMs = Stats.ResponseDelayMs;
			Timeline.Nodes.push_back(InitialDelayNode);

			RaceTimelineNode AccelerationNode;
			AccelerationNode.Type = RaceTimelineNodeType::AccelerationToTopSpeed;
			AccelerationNode.StartTimeMs = Stats.ResponseDelayMs;
			AccelerationNode.EndTimeMs = Stats.ResponseDelayMs + Stats.AccelerationDurationMs;
			AccelerationNode.EndPace = Stats.MaximumPace;
			Timeline.Nodes.push_back(AccelerationNode);
			AccelerationTimelineNode = &Timeline.Nodes.back();
			EnduranceCycleDurationMs = Stats.FullEnduranceDurationMs;
		}

		double NormalPace(double TimeMs) const
		{
			const double AccelerationCurveTimeMs = TimeMs - AccelerationCurveTimeOffsetMs;
			if (AccelerationCurveTimeMs <= Stats.ResponseDelayMs)
				return 0.0;
			return Stats.MaximumPace * Runner->AccelerationCurve.Evaluate(
				(AccelerationCurveTimeMs - Stats.ResponseDelayMs) / Stats.AccelerationDurationMs);
		}

		double EnduranceReserve(double TimeMs) const
		{
			if (Gassing)
				return GassingRule.HeldEnduranceReserve();
			return EvaluateEnduranceReserve(Runner->EnduranceCurve, TimeMs, EnduranceCycleStartMs,
				EnduranceCycleDurationMs, EnduranceCycleReserve);
		}

		double PaceAfterEndurance(double TimeMs) const
		{
			return ApplyEndurancePaceMultiplier(NormalPace(TimeMs), EnduranceReserve(TimeMs));
		}

		double EffectMultiplier(double TimeMs) const
		{
			double Multiplier = 1.0;
			if (TimeMs < StumbleUntilMs)
				Multiplier *= StumblePaceMultiplier;
			if (TimeMs < LostBeliefUntilMs)
				Multiplier *= LostBeliefPaceMultiplier;
			if (TimeMs < BeliefUntilMs)
				Multiplier *= BeliefPaceMultiplier;
			return Multiplier;
		}

		double CurrentPace(double TimeMs) const
		{
			// The wind override wins; otherwise gassing replaces the endurance-adjusted pace.
			if (WindOverrideActive && TimeMs < WindOverrideUntilMs)
				return WindOverridePace;

			double Pace = PaceAfterEndurance(TimeMs);
			if (Gassing)
				Pace = GassingEvent::PaceAt(GassingEntryBasePace, TimeMs - GassingStartMs);
			return Pace * EffectMultiplier(TimeMs);
		}

		Game::Race::RaceEvent CreateEventRecord(Game::Race::RaceEventType Type,
			double TimeMs, unsigned int DurationMs, double Magnitude) const
		{
			return CreateRaceEvent(Runner, static_cast<unsigned int>(RacerIndex), Type,
				TimeMs, DurationMs, Magnitude);
		}

		Game::Race::RaceTimelineNode* AddEvent(const Game::Race::RaceEvent& Event)
		{
			using namespace Game::Race;

			// Capture onset pace and distance so playback can reconstruct the event without rerunning the simulation.
			RaceTimelineNode Node;
			Node.Type = Event.Type == RaceEventType::Gassing
				? RaceTimelineNodeType::Gassing : RaceTimelineNodeType::RandomEvent;
			Node.StartTimeMs = Event.TimeMs;
			Node.EndTimeMs = Event.TimeMs + Event.DurationMs;
			Node.Distance = Distance;
			Node.StartPace = CurrentPace(Event.TimeMs);
			Node.Event = Event;
			Timeline.Nodes.push_back(Node);
			return &Timeline.Nodes.back();
		}

		void CloseGassingEvent(double EndTimeMs)
		{
			if (GassingTimelineNode == nullptr)
				return;

			GassingTimelineNode->EndTimeMs = EndTimeMs;
			GassingTimelineNode->EndPace = CurrentPace(EndTimeMs);
			GassingTimelineNode->Event.DurationMs = ToMilliseconds(EndTimeMs - GassingStartMs);
			GassingTimelineNode = nullptr;
		}

		void StartGassingIfDue(double TimeMs)
		{
			using namespace Game::Race;

			if (Gassing || !GassingRule.ShouldStart(EnduranceReserve(TimeMs)))
				return;

			Gassing = true;
			GassingStartMs = TimeMs;
			GassingEntryBasePace = GassingRule.EntryPace(NormalPace(TimeMs));
			GassingEntryPace = GassingEntryBasePace * EffectMultiplier(TimeMs);
			const RaceEvent Event = CreateEventRecord(RaceEventType::Gassing, TimeMs, 0, GassingEntryPace);
			GassingTimelineNode = AddEvent(Event);
			GassingTimelineNode->StartPace = GassingEntryBasePace;
		}

		void TryStumble(double TimeMs)
		{
			using namespace Game::Race;

			if (TimeMs < StumbleUntilMs)
				return;

			unsigned int DurationMs = 0;
			double PaceMultiplier = 1.0;
			if (!StumbleRule.TryRoll(Stats.Confidence, Stats.Fatigue, EventRandom, DurationMs, PaceMultiplier))
				return;

			StumbleUntilMs = TimeMs + DurationMs;
			StumblePaceMultiplier = PaceMultiplier;
			StumbleTimelineNode = AddEvent(CreateEventRecord(RaceEventType::Stumble,
				TimeMs, DurationMs, PaceMultiplier));
		}

		void CompleteStumbleIfDue(double TimeMs)
		{
			if (StumbleTimelineNode == nullptr || TimeMs < StumbleUntilMs)
				return;

			// Store the adjusted curve clock on the event so replay reproduces the same recovery.
			AccelerationCurveTimeOffsetMs = AccelerationCurveTimeOffsetAfterStumble(*Runner,
				Stats, StumbleUntilMs, AccelerationCurveTimeOffsetMs, StumblePaceMultiplier);
			StumbleTimelineNode->AccelerationCurveTimeOffsetMs = AccelerationCurveTimeOffsetMs;
			AccelerationTimelineNode->EndTimeMs = std::max(AccelerationTimelineNode->EndTimeMs,
				Stats.ResponseDelayMs + Stats.AccelerationDurationMs + AccelerationCurveTimeOffsetMs);
			StumbleTimelineNode = nullptr;
			StumbleUntilMs = 0.0;
			StumblePaceMultiplier = 1.0;
		}

		void CloseThoughtNode(Game::Race::RaceTimelineNode* Node, double TimeMs)
		{
			if (Node != nullptr && TimeMs < Node->EndTimeMs)
				Node->EndTimeMs = TimeMs;
		}

		void TryThoughtEvent(double TimeMs)
		{
			using namespace Game::Race;

			std::uniform_real_distribution<double> Unit(0.0, 1.0);
			// Reuse one roll to make the two thought events mutually exclusive.
			const double ThoughtRoll = Unit(EventRandom);
			const double LostBeliefChance = LostBeliefRule.ChanceFor(Stats.Confidence);
			unsigned int DurationMs = 0;
			double PaceMultiplier = 1.0;
			if (LostBeliefRule.TryRoll(Stats.Confidence, ThoughtRoll, EventRandom, DurationMs, PaceMultiplier))
			{
				CloseThoughtNode(BeliefTimelineNode, TimeMs);
				CloseThoughtNode(LostBeliefTimelineNode, TimeMs);
				LostBeliefUntilMs = TimeMs + DurationMs;
				LostBeliefPaceMultiplier = PaceMultiplier;
				BeliefUntilMs = 0.0;
				BeliefTimelineNode = nullptr;
				LostBeliefTimelineNode = AddEvent(CreateEventRecord(RaceEventType::LostBelief,
					TimeMs, DurationMs, PaceMultiplier));
			}
			else if (BeliefRule.TryRoll(Stats.Confidence, ThoughtRoll, EventRandom,
				LostBeliefChance, DurationMs, PaceMultiplier))
			{
				CloseThoughtNode(LostBeliefTimelineNode, TimeMs);
				CloseThoughtNode(BeliefTimelineNode, TimeMs);
				BeliefUntilMs = TimeMs + DurationMs;
				BeliefPaceMultiplier = PaceMultiplier;
				LostBeliefUntilMs = 0.0;
				LostBeliefTimelineNode = nullptr;
				BeliefTimelineNode = AddEvent(CreateEventRecord(RaceEventType::BelievesInSelf,
					TimeMs, DurationMs, PaceMultiplier));
			}
		}

		void TrySecondWind(double TimeMs)
		{
			using namespace Game::Race;

			if (!Gassing || !SecondWindRule.TryRoll(Stats.Confidence, Stats.Luck, EventRandom))
				return;

			// Restart endurance from the restored reserve, using a brief pace override to bridge the change.
			CloseGassingEvent(TimeMs);
			const double RestoredReserve = SecondWindRule.RestoredReserve(Stats.Confidence, Stats.Luck);
			WindOverridePace = GassingEntryPace;
			WindOverrideUntilMs = TimeMs + SecondWindEvent::PaceOverrideDurationMs;
			WindOverrideActive = true;
			Gassing = false;
			EnduranceCycleStartMs = TimeMs;
			EnduranceCycleDurationMs = std::max<double>(SimulationStepMs,
				Stats.FullEnduranceDurationMs * RestoredReserve);
			EnduranceCycleReserve = RestoredReserve;
			RaceTimelineNode* SecondWindNode = AddEvent(CreateEventRecord(RaceEventType::SecondWind,
				TimeMs, 0, RestoredReserve));
			SecondWindNode->EndTimeMs = TimeMs + SecondWindEvent::PaceOverrideDurationMs;
		}

		bool TryPullUp(double TimeMs)
		{
			using namespace Game::Race;

			// A racer can pull up only while gassing, after a confidence- and composure-dependent limit.
			if (!Gassing || !PullingUpRule.ShouldPullUp(TimeMs - GassingStartMs,
				Stats.Confidence, Runner->Skills.Composure))
				return false;

			const double GassingDurationMs = TimeMs - GassingStartMs;
			const double PaceAtPullUp = CurrentPace(TimeMs);
			CloseGassingEvent(TimeMs);
			Gassing = false;
			Timeline.PulledUp = true;
			Timeline.PullUpTimeMs = TimeMs;
			Timeline.PullUpDistance = Distance;

			RaceTimelineNode* PullUpNode = AddEvent(CreateEventRecord(RaceEventType::PullingUp,
				TimeMs, 0, GassingDurationMs));
			PullUpNode->StartPace = PaceAtPullUp;
			PullUpNode->EndPace = 0.0;
			return true;
		}

		void ProcessEventChecks(double TimeMs)
		{
			// Run event rolls on the fixed one-second cadence independently of movement integration steps.
			while (NextEventCheckMs <= TimeMs)
			{
				TryStumble(TimeMs);
				TryThoughtEvent(TimeMs);
				TrySecondWind(TimeMs);
				NextEventCheckMs += EventIntervalMs;
			}
		}

		bool AdvanceRace(double TimeMs)
		{
			using namespace Game::Race;

			const double NextTimeMs = std::min(TimeMs + SimulationStepMs, static_cast<double>(MaximumRaceDurationMs));
			const double StepDurationMs = NextTimeMs - TimeMs;
			const double PaceAtStart = CurrentPace(TimeMs);
			const double PaceAtEnd = CurrentPace(NextTimeMs);
			// Average endpoint paces, then interpolate a track crossing to its exact time within the step.
			const double DistanceStep = (PaceAtStart + PaceAtEnd) * TrapezoidalIntegrationFactor *
				StepDurationMs / MillisecondsPerSecond;

			if (DistanceStep > 0.0 && Distance + DistanceStep >= TrackLength)
			{
				const double Fraction = (TrackLength - Distance) / DistanceStep;
				const double FinishTimeMs = TimeMs + StepDurationMs * Fraction;
				Timeline.Finished = true;
				Timeline.FinishTimeMs = FinishTimeMs;
				if (AccelerationTimelineNode->EndTimeMs > FinishTimeMs)
				{
					AccelerationTimelineNode->EndTimeMs = FinishTimeMs;
					AccelerationTimelineNode->EndPace = NormalPace(FinishTimeMs);
				}
				Distance = TrackLength;
				if (Gassing)
					CloseGassingEvent(FinishTimeMs);

				RaceTimelineNode FinishNode;
				FinishNode.Type = RaceTimelineNodeType::Finish;
				FinishNode.StartTimeMs = FinishTimeMs;
				FinishNode.EndTimeMs = FinishTimeMs;
				FinishNode.Distance = static_cast<double>(TrackLength);
				FinishNode.StartPace = CurrentPace(FinishTimeMs);
				FinishNode.EndPace = FinishNode.StartPace;
				Timeline.Nodes.push_back(FinishNode);
				return true;
			}

			Distance += DistanceStep;
			return false;
		}

		void Simulate()
		{
			// Resolve event transitions before the pull-up check and distance integration at each fixed step.
			for (double TimeMs = 0.0; TimeMs < MaximumRaceDurationMs; TimeMs += SimulationStepMs)
			{
				CompleteStumbleIfDue(TimeMs);
				StartGassingIfDue(TimeMs);
				ProcessEventChecks(TimeMs);
				if (TryPullUp(TimeMs))
					break;
				if (AdvanceRace(TimeMs))
					break;
			}

			if (Gassing)
				CloseGassingEvent(Timeline.Finished ? Timeline.FinishTimeMs : MaximumRaceDurationMs);
		}

		Game::Race::Racer* Runner;
		std::size_t RacerIndex;
		unsigned int LockedSeed;
		unsigned int TrackLength;
		Game::Race::RacerRaceTimeline Timeline;
		EffectiveRacerStats Stats;
		std::mt19937 EventRandom;
		GassingEvent GassingRule;
		PullingUpEvent PullingUpRule;
		StumbleEvent StumbleRule;
		LostBeliefEvent LostBeliefRule;
		BelievesInSelfEvent BeliefRule;
		SecondWindEvent SecondWindRule;
		double StumbleUntilMs = 0.0;
		double StumblePaceMultiplier = 1.0;
		double AccelerationCurveTimeOffsetMs = 0.0;
		double LostBeliefUntilMs = 0.0;
		double LostBeliefPaceMultiplier = 1.0;
		double BeliefUntilMs = 0.0;
		double BeliefPaceMultiplier = 1.0;
		double WindOverrideUntilMs = 0.0;
		double WindOverridePace = 0.0;
		bool WindOverrideActive = false;
		double EnduranceCycleStartMs = 0.0;
		double EnduranceCycleDurationMs = 0.0;
		double EnduranceCycleReserve = 1.0;
		double GassingStartMs = 0.0;
		double GassingEntryPace = 0.0;
		double GassingEntryBasePace = 0.0;
		bool Gassing = false;
		Game::Race::RaceTimelineNode* AccelerationTimelineNode = nullptr;
		Game::Race::RaceTimelineNode* StumbleTimelineNode = nullptr;
		Game::Race::RaceTimelineNode* GassingTimelineNode = nullptr;
		Game::Race::RaceTimelineNode* LostBeliefTimelineNode = nullptr;
		Game::Race::RaceTimelineNode* BeliefTimelineNode = nullptr;
		unsigned int NextEventCheckMs = EventIntervalMs;
		double Distance = 0.0;
	};
}

void Game::Race::Race::Finished(Racer* Runner, unsigned int FinishTimeMs)
{
	Result.RacerResults.emplace_back(Runner, FinishedCount() + FirstPlaceRank, FinishTimeMs, GUID);
}

void Game::Race::Race::AddPulledUpResult(Racer* Runner, unsigned int PullUpTimeMs)
{
	Result.RacerResults.emplace_back(Runner, PulledUpPosition, PullUpTimeMs, GUID);
}

bool Game::Race::Race::HasResult(Racer* Runner) const
{
	if (Runner == nullptr)
		return false;

	return std::any_of(Result.RacerResults.begin(), Result.RacerResults.end(),
		[Runner](const RacerRaceResult& Result)
		{
			return Result.Racer == Runner;
		});
}

unsigned int Game::Race::Race::FinishedCount() const
{
	return static_cast<unsigned int>(std::count_if(Result.RacerResults.begin(), Result.RacerResults.end(),
		[](const RacerRaceResult& Entry)
		{
			return Entry.Position != PulledUpPosition;
		}));
}

unsigned int Game::Race::Race::PulledUpCount() const
{
	return static_cast<unsigned int>(std::count_if(Result.RacerResults.begin(), Result.RacerResults.end(),
		[](const RacerRaceResult& Entry)
		{
			return Entry.Position == PulledUpPosition;
		}));
}

bool Game::Race::Race::HasFinished(Racer* Runner) const
{
	if (Runner == nullptr)
		return false;

	return std::any_of(Result.RacerResults.begin(), Result.RacerResults.end(),
		[Runner](const RacerRaceResult& Result)
		{
			return Result.Racer == Runner && Result.Position != PulledUpPosition;
		});
}

bool Game::Race::Race::HasFinished() const
{
	if (TimelineReady)
		return Result.RacerResults.size() >= LockedEntrants.size();

	const auto EntrantCount = std::count_if(Racers.begin(), Racers.end(), [](const Racer* Runner)
		{
			return Runner != nullptr;
		});
	return Result.RacerResults.size() >= static_cast<std::size_t>(EntrantCount);
}

Game::Race::Racer* Game::Race::Race::CurrentWinner()
{
	return GetByRank(FirstPlaceRank);
}

unsigned int Game::Race::Race::CurrentWinnerDistance()
{
	const Racer* Winner = CurrentWinner();
	return Winner == nullptr ? 0u : static_cast<unsigned int>(std::max(0, Winner->Pos.X));
}

Game::Race::Racer* Game::Race::Race::Get(RacerGUID Guid)
{
	if (TimelineReady)
	{
		for (const RacerRaceTimeline& Timeline : Timelines)
		{
			if (Timeline.Entrant != nullptr && Timeline.Entrant->GUID.GUID == Guid.GUID)
				return Timeline.Entrant;
		}
		return nullptr;
	}

	for (Racer* Runner : Racers)
	{
		if (Runner != nullptr && Runner->GUID == Guid)
			return Runner;
	}
	return nullptr;
}

Game::Race::Racer* Game::Race::Race::GetByRank(unsigned int Rank)
{
	if (Rank == 0)
		return nullptr;

	const auto RankedResult = std::find_if(Result.RacerResults.begin(), Result.RacerResults.end(),
		[Rank](const RacerRaceResult& Entry)
		{
			return Entry.Position == Rank;
		});
	if (RankedResult != Result.RacerResults.end())
		return RankedResult->Racer;
	if (Rank <= FinishedCount())
		return nullptr;

	std::vector<Racer*> ActiveRacers;
	for (Racer* Runner : GetEntrants())
	{
		if (Runner != nullptr && !HasResult(Runner))
			ActiveRacers.push_back(Runner);
	}
	std::stable_sort(ActiveRacers.begin(), ActiveRacers.end(),
		[](const Racer* Left, const Racer* Right)
		{
			return Left->Pos.X > Right->Pos.X;
		});

	const std::size_t ActiveRank = Rank - FinishedCount();
	return ActiveRank <= ActiveRacers.size() ? ActiveRacers[ActiveRank - 1] : nullptr;
}

bool Game::Race::Race::Contains(RacerGUID Guid)
{
	return Get(Guid) != nullptr;
}

Game::Race::Race::Race()
	: TimelineReady(false), ConditionsApplied(false), Complete(false), PlaybackTimeMs(0.0), TimelineDurationMs(0.0),
	LockedSeed(0), LockedTrackLength(0), Seed(static_cast<unsigned int>(std::rand())), CurrentTick(0), ThisTrack(new Track())
{
	GUID.GUID = Seed;
	Result.GUID = GUID;
}

Game::Race::RaceError Game::Race::Race::SetSeed(unsigned int RaceSeed)
{
	if (TimelineReady)
		return RaceError::SeedLocked;
	Seed = RaceSeed;
	return RaceError::None;
}

unsigned int Game::Race::Race::GetTrackLength() const
{
	if (TimelineReady)
		return LockedTrackLength;
	return ThisTrack == nullptr ? 0u : static_cast<unsigned int>(ThisTrack->Length);
}

std::vector<Game::Race::RaceEvent> Game::Race::Race::GetEventsThroughPlaybackTime() const
{
	std::vector<RaceEvent> VisibleEvents;
	for (const RaceEvent& Event : Events)
	{
		if (Event.TimeMs > PlaybackTimeMs)
			break;
		VisibleEvents.push_back(Event);
	}
	return VisibleEvents;
}

unsigned int Game::Race::Race::GetActiveEffects(const Racer* Runner) const
{
	for (const RacerRaceTimeline& Timeline : Timelines)
	{
		if (Timeline.Entrant == Runner)
			return Timeline.ActiveEffects;
	}
	return RaceEffectNone;
}

double Game::Race::Race::GetCurrentEndurance(const Racer* Runner) const
{
	for (const RacerRaceTimeline& Timeline : Timelines)
	{
		if (Timeline.Entrant == Runner)
			return Timeline.CurrentEndurance;
	}
	return 0.0;
}

const Game::Race::RacerRaceTimeline* Game::Race::Race::GetTimelineFor(const Racer* Runner) const
{
	const auto Timeline = std::find_if(Timelines.begin(), Timelines.end(), [Runner](const RacerRaceTimeline& Entry)
		{
			return Entry.Entrant == Runner;
		});
	return Timeline == Timelines.end() ? nullptr : &*Timeline;
}

bool Game::Race::Race::GetTimelineStateAt(const Racer* Runner, double TimeMs, RaceTimelineState& State) const
{
	const RacerRaceTimeline* Timeline = GetTimelineFor(Runner);
	if (Timeline == nullptr)
		return false;

	const double TimelineEndTimeMs = Timeline->Finished ? Timeline->FinishTimeMs :
		Timeline->PulledUp ? Timeline->PullUpTimeMs : TimelineDurationMs;
	State = EvaluateTimelinePlaybackState(*Timeline, std::clamp(TimeMs, 0.0, TimelineEndTimeMs));
	return true;
}

Game::Race::RaceError Game::Race::Race::Precompute()
{
	// Lock race inputs, precompute every entrant, and assemble shared playback data before playback begins.
	if (TimelineReady)
		return RaceError::None;
	if (ThisTrack == nullptr)
	{
		Result.Error = RaceError::MissingTrack;
		return Result.Error;
	}

	LockedTrackLength = static_cast<unsigned int>(ThisTrack->Length);
	if (LockedTrackLength == 0)
	{
		Result.Error = RaceError::ZeroLengthTrack;
		return Result.Error;
	}

	LockedSeed = Seed;
	Result.GUID = GUID;
	Result.Error = RaceError::None;
	Result.RacerResults.clear();
	LockedEntrants.clear();
	Timelines.clear();
	Events.clear();
	PlaybackTimeMs = 0.0;
	TimelineDurationMs = 0.0;
	CurrentTick = 0;
	ConditionsApplied = false;
	Complete = false;

	PrecomputeRacerTimelines();
	AssignTimelineFinishPositions();
	CalculateTimelineConditions();
	SortTimelineNodesAndCollectEvents();

	TimelineReady = true;
	Complete = Timelines.empty();
	UpdatePlaybackStateAt(0.0);
	return RaceError::None;
}

void Game::Race::Race::PrecomputeRacerTimelines()
{
	// Preserve each entrant's original index for deterministic random seeds and tie-breaking.
	for (std::size_t RacerIndex = 0; RacerIndex < Racers.size(); ++RacerIndex)
	{
		Racer* Runner = Racers[RacerIndex];
		if (Runner == nullptr)
			continue;
		LockedEntrants.push_back(Runner);
		RacerTimelinePrecomputer Precomputer(Runner, RacerIndex, LockedSeed, LockedTrackLength);
		Timelines.push_back(Precomputer.Build());
	}
}

void Game::Race::Race::AssignTimelineFinishPositions()
{
	// Playback lasts through the latest finish or pull-up, but only finishers receive ranked places.
	for (const RacerRaceTimeline& Timeline : Timelines)
	{
		const double TimelineEndTimeMs = Timeline.Finished ? Timeline.FinishTimeMs :
			Timeline.PulledUp ? Timeline.PullUpTimeMs : static_cast<double>(MaximumRaceDurationMs);
		TimelineDurationMs = std::max(TimelineDurationMs, TimelineEndTimeMs);
	}

	std::vector<std::size_t> FinishOrder;
	for (std::size_t Index = 0; Index < Timelines.size(); ++Index)
	{
		if (Timelines[Index].Finished)
			FinishOrder.push_back(Index);
	}
	// Stable sorting preserves entrant order when finish times are identical.
	std::stable_sort(FinishOrder.begin(), FinishOrder.end(), [&](std::size_t Left, std::size_t Right)
		{
			return Timelines[Left].FinishTimeMs < Timelines[Right].FinishTimeMs;
		});
	for (std::size_t Index = 0; Index < FinishOrder.size(); ++Index)
		Timelines[FinishOrder[Index]].FinishPosition = static_cast<unsigned int>(Index + FirstPlaceRank);
}


void Game::Race::Race::CalculateTimelineConditions()
{
	// Calculate post-race changes now, but apply them only after playback has completed.
	for (std::size_t Index = 0; Index < Timelines.size(); ++Index)
	{
		RacerRaceTimeline& Timeline = Timelines[Index];
		const double RaceDurationMs = Timeline.Finished ? Timeline.FinishTimeMs :
			Timeline.PulledUp ? Timeline.PullUpTimeMs : TimelineDurationMs;
		const double FatigueGain = std::min(MaximumFatigueGain, BaseFatigueGain +
			(RaceDurationMs / MillisecondsPerSecond) * FatigueGainPerSecond);
		const EffectiveRacerStats Stats = CalculateEffectiveRacerStats(*Timeline.Entrant);
		const double Composure = ClampRating(Timeline.Entrant->Skills.Composure);
		Timeline.FatigueAfter = Clamp01(Stats.Fatigue + FatigueGain);

		const double ComposureScale = MinimumComposureMultiplier + Composure / MaximumRacerRating;
		const double ConfidenceChange = Timeline.Finished && Timeline.FinishPosition == FirstPlaceRank
			? WinnerConfidenceChange : NonWinnerConfidenceChange;
		Timeline.ConfidenceAfter = Clamp01(Stats.Confidence + ConfidenceChange * ComposureScale);
		Timeline.InjuryAfter = Stats.Injury;

		const InjuryEvent InjuryRule;
		// Isolate injury rolls so other event-roll counts cannot change this outcome.
		std::mt19937 InjuryRandom = InjuryRule.CreateRandom(LockedSeed, Timeline.Entrant->GUID.GUID, Timeline.RacerIndex);
		if (InjuryRule.TryRoll(Stats.Fatigue, InjuryRandom))
		{
			const double InjuryGain = InjuryRule.Magnitude();
			Timeline.InjuryAfter = Clamp01(Stats.Injury + InjuryGain);
			const double InjuryTimeMs = Timeline.Finished ? Timeline.FinishTimeMs :
				Timeline.PulledUp ? Timeline.PullUpTimeMs : TimelineDurationMs;
			RaceEvent InjuryEvent = CreateRaceEvent(Timeline.Entrant, Timeline.RacerIndex,
				RaceEventType::Injury, InjuryTimeMs, 0, InjuryGain);
			RaceTimelineNode InjuryNode;
			InjuryNode.Type = RaceTimelineNodeType::RandomEvent;
			InjuryNode.StartTimeMs = InjuryEvent.TimeMs;
			InjuryNode.EndTimeMs = InjuryEvent.TimeMs + InjuryEvent.DurationMs;
			InjuryNode.Distance = Timeline.FinalDistance;
			InjuryNode.Event = InjuryEvent;
			Timeline.Nodes.push_back(InjuryNode);
		}
	}
}


void Game::Race::Race::SortTimelineNodesAndCollectEvents()
{
	Events.clear();
	for (RacerRaceTimeline& Timeline : Timelines)
	{
		// At equal timestamps, order event nodes before the finish marker.
		Timeline.Nodes.sort([](const RaceTimelineNode& Left, const RaceTimelineNode& Right)
			{
				if (Left.StartTimeMs != Right.StartTimeMs)
					return Left.StartTimeMs < Right.StartTimeMs;
				if (Left.Type == RaceTimelineNodeType::Finish)
					return false;
				return Right.Type == RaceTimelineNodeType::Finish;
			});

		for (const RaceTimelineNode& Node : Timeline.Nodes)
		{
			if (Node.Type == RaceTimelineNodeType::RandomEvent || Node.Type == RaceTimelineNodeType::Gassing)
				Events.push_back(Node.Event);
		}
	}

	// Order simultaneous events by entrant index for deterministic playback and display.
	std::stable_sort(Events.begin(), Events.end(), [](const RaceEvent& Left, const RaceEvent& Right)
		{
			if (Left.TimeMs != Right.TimeMs)
				return Left.TimeMs < Right.TimeMs;
			return Left.RacerIndex < Right.RacerIndex;
		});
}

void Game::Race::Race::RecordResultsThrough(double TimeMs)
{
	// Reveal each precomputed result only when playback reaches its finish or pull-up time.
	struct NewlyResolvedResult
	{
		std::size_t TimelineIndex;
		double TimeMs;
		bool PulledUp;
	};

	std::vector<NewlyResolvedResult> NewlyResolved;
	for (std::size_t Index = 0; Index < Timelines.size(); ++Index)
	{
		const RacerRaceTimeline& Timeline = Timelines[Index];
		if (Timeline.Finished && Timeline.FinishTimeMs <= TimeMs && !HasResult(Timeline.Entrant))
			NewlyResolved.push_back({ Index, Timeline.FinishTimeMs, false });
		else if (Timeline.PulledUp && Timeline.PullUpTimeMs <= TimeMs && !HasResult(Timeline.Entrant))
			NewlyResolved.push_back({ Index, Timeline.PullUpTimeMs, true });
	}
	std::stable_sort(NewlyResolved.begin(), NewlyResolved.end(), [&](const NewlyResolvedResult& Left,
		const NewlyResolvedResult& Right)
		{
			if (Left.TimeMs != Right.TimeMs)
				return Left.TimeMs < Right.TimeMs;
			return Timelines[Left.TimelineIndex].RacerIndex < Timelines[Right.TimelineIndex].RacerIndex;
		});

	for (const NewlyResolvedResult& Entry : NewlyResolved)
	{
		const RacerRaceTimeline& Timeline = Timelines[Entry.TimelineIndex];
		const unsigned int ResultTimeMs = ToMilliseconds(Entry.TimeMs);
		if (Entry.PulledUp)
			AddPulledUpResult(Timeline.Entrant, ResultTimeMs);
		else
			Finished(Timeline.Entrant, ResultTimeMs);
	}
}

void Game::Race::Race::UpdatePlaybackStateAt(double TimeMs)
{
	for (RacerRaceTimeline& Timeline : Timelines)
	{
		if (Timeline.Entrant == nullptr)
			continue;

		const bool HasTerminalTime = Timeline.Finished || Timeline.PulledUp;
		const double TerminalTimeMs = Timeline.Finished ? Timeline.FinishTimeMs : Timeline.PullUpTimeMs;
		const double TargetTimeMs = HasTerminalTime ? std::min(TimeMs, TerminalTimeMs) : TimeMs;
		// Consume complete fixed intervals once, regardless of the caller's playback tick size.
		while (Timeline.PlaybackIntegratedTimeMs + SimulationStepMs <= TargetTimeMs)
		{
			const double StepStartMs = Timeline.PlaybackIntegratedTimeMs;
			const double StepEndMs = StepStartMs + SimulationStepMs;
			if (HasTerminalTime && StepEndMs > TerminalTimeMs)
				break;

			const TimelinePlaybackState StartState = EvaluateTimelinePlaybackState(Timeline, StepStartMs);
			const TimelinePlaybackState EndState = EvaluateTimelinePlaybackState(Timeline, StepEndMs, false);
			Timeline.PlaybackDistance += (StartState.Pace + EndState.Pace) * TrapezoidalIntegrationFactor *
				SimulationStepMs / MillisecondsPerSecond;
			Timeline.PlaybackIntegratedTimeMs = StepEndMs;
		}

		double Distance = Timeline.PlaybackDistance;
		TimelinePlaybackState State;
		if (Timeline.Finished && TimeMs >= Timeline.FinishTimeMs)
		{
			Distance = static_cast<double>(LockedTrackLength);
			Timeline.PlaybackDistance = Distance;
			Timeline.PlaybackIntegratedTimeMs = Timeline.FinishTimeMs;
			State = EvaluateTimelinePlaybackState(Timeline, Timeline.FinishTimeMs);
		}
		else if (Timeline.PulledUp && TimeMs >= Timeline.PullUpTimeMs)
		{
			Distance = Timeline.PullUpDistance;
			Timeline.PlaybackDistance = Distance;
			Timeline.PlaybackIntegratedTimeMs = Timeline.PullUpTimeMs;
			State = EvaluateTimelinePlaybackState(Timeline, Timeline.PullUpTimeMs);
		}
		else if (Timeline.PlaybackIntegratedTimeMs < TargetTimeMs)
		{
			// Interpolate the remaining partial interval while respecting the entrant's terminal time.
			const double StepStartMs = Timeline.PlaybackIntegratedTimeMs;
			const double GridStepEndMs = StepStartMs + SimulationStepMs;
			const bool EndsDuringStep = HasTerminalTime && GridStepEndMs > TerminalTimeMs;
			const double StepEndMs = EndsDuringStep ? TerminalTimeMs : GridStepEndMs;
			const double StepDurationMs = StepEndMs - StepStartMs;
			const double Fraction = StepDurationMs > 0.0 ? (TargetTimeMs - StepStartMs) / StepDurationMs : 0.0;
			const TimelinePlaybackState StartState = EvaluateTimelinePlaybackState(Timeline, StepStartMs);
			const TimelinePlaybackState EndState = EvaluateTimelinePlaybackState(Timeline, StepEndMs);
			const TimelinePlaybackState IntegrationEndState = EvaluateTimelinePlaybackState(Timeline, StepEndMs, false);
			if (Timeline.Finished && EndsDuringStep)
			{
				Distance += (static_cast<double>(LockedTrackLength) - Distance) * Fraction;
			}
			else
			{
				const double StepDistance = (StartState.Pace + IntegrationEndState.Pace) * TrapezoidalIntegrationFactor *
					StepDurationMs / MillisecondsPerSecond;
				Distance += StepDistance * Fraction;
			}
			State.Pace = StartState.Pace + (EndState.Pace - StartState.Pace) * Fraction;
			State.Endurance = StartState.Endurance + (EndState.Endurance - StartState.Endurance) * Fraction;
			State.ActiveEffects = StartState.ActiveEffects;
		}
		else
		{
			State = EvaluateTimelinePlaybackState(Timeline, TargetTimeMs);
		}

		Timeline.Entrant->Pos.X = static_cast<int>(std::lround(std::clamp(Distance, 0.0, static_cast<double>(LockedTrackLength))));
		Timeline.Entrant->Pos.Velocity = State.Pace;
		Timeline.CurrentEndurance = State.Endurance;
		Timeline.ActiveEffects = State.ActiveEffects;
		Timeline.Entrant->CurrentTick = static_cast<unsigned int>(std::clamp(std::floor(TimeMs), 0.0,
			static_cast<double>(std::numeric_limits<unsigned int>::max())));
		const double FrameTimeMs = std::clamp(AnimationBaseFrameTimeMs - State.Pace * AnimationFrameTimeReductionPerPace,
			MinimumAnimationFrameTimeMs, AnimationBaseFrameTimeMs);
		Timeline.Entrant->RunFrame = static_cast<unsigned int>(TimeMs / FrameTimeMs) % AnimationFrameCount;
		Timeline.Entrant->LastRunFrameEnd = Timeline.Entrant->CurrentTick;
	}
}

void Game::Race::Race::ApplyPersistentConditions()
{
	// Commit the precomputed post-race conditions once, after all timelines have played out.
	if (ConditionsApplied)
		return;

	for (const RacerRaceTimeline& Timeline : Timelines)
	{
		if (Timeline.Entrant == nullptr)
			continue;
		Timeline.Entrant->Conditions.Fatigue = Timeline.FatigueAfter;
		Timeline.Entrant->Conditions.Confidence = Timeline.ConfidenceAfter;
		Timeline.Entrant->Conditions.Injury = Timeline.InjuryAfter;
	}
	ConditionsApplied = true;
}

Game::Race::RaceStatus Game::Race::Race::Tick(unsigned int Ms, RaceStatus Type)
{
	// Advance the precomputed replay, revealing results at their event times and conditions at completion.
	if (Type == RaceStatus::Failed)
		return RaceStatus::Failed;
	if (Complete || Type == RaceStatus::Finished)
		return RaceStatus::Finished;

	if (Precompute() != RaceError::None)
		return RaceStatus::Failed;
	if (Complete)
		return RaceStatus::Finished;

	PlaybackTimeMs = std::min(PlaybackTimeMs + Ms, TimelineDurationMs);
	CurrentTick = ToMilliseconds(PlaybackTimeMs);
	UpdatePlaybackStateAt(PlaybackTimeMs);
	RecordResultsThrough(PlaybackTimeMs);

	if (PlaybackTimeMs >= TimelineDurationMs)
	{
		Complete = true;
		ApplyPersistentConditions();
		return RaceStatus::Finished;
	}
	return FinishedCount() > 0 ? RaceStatus::Finishing : RaceStatus::Racing;
}

Game::Race::RaceResult Game::Race::Race::Sim()
{
	// Reuse Tick so one-shot simulation and animated playback resolve the same timeline identically.
	if (Precompute() != RaceError::None)
		return Result;
	if (Complete)
		return Result;

	RaceStatus Status = FinishedCount() > 0 ? RaceStatus::Finishing : RaceStatus::Racing;
	while (Status != RaceStatus::Finished && Status != RaceStatus::Failed)
		Status = Tick(RaceTickIntervalMs, Status);
	return Result;
}

void Game::Race::Race::Reset()
{
	Result.RacerResults.clear();
	Result.GUID = GUID;
	Result.Error = RaceError::None;
	Timelines.clear();
	LockedEntrants.clear();
	Events.clear();
	TimelineReady = false;
	ConditionsApplied = false;
	Complete = false;
	PlaybackTimeMs = 0.0;
	TimelineDurationMs = 0.0;
	LockedSeed = 0;
	LockedTrackLength = 0;
	CurrentTick = 0;

	for (Racer* Runner : Racers)
	{
		if (Runner != nullptr)
			Runner->Reset();
	}
}

Game::Race::RaceFinancials::RaceFinancials()
	: FirstPlacePrize(0), SecondPlacePrize(0), ThirdPlacePrize(0), EntranceFee(0)
{
}

Game::Race::RaceFinancials::RaceFinancials(unsigned int _FirstPlacePrize, unsigned int _EntranceFee)
	: FirstPlacePrize(_FirstPlacePrize), SecondPlacePrize(_FirstPlacePrize / SecondPlacePrizeDivisor),
	ThirdPlacePrize(_FirstPlacePrize / ThirdPlacePrizeDivisor), EntranceFee(_EntranceFee)
{
}

std::string Game::Race::RaceErrorToString(RaceError Error)
{
	switch (Error)
	{
	case RaceError::None:
		return "None";
	case RaceError::SeedLocked:
		return "Race seed is locked after precomputation.";
	case RaceError::MissingTrack:
		return "Cannot precompute a race without a track.";
	case RaceError::ZeroLengthTrack:
		return "Cannot precompute a race with a zero-length track.";
	default:
		return "Unknown race error.";
	}
}
