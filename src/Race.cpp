#include "Race.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <random>
#include <utility>

namespace RaceSimulationDetail
{
	constexpr unsigned int MillisecondsPerSecond = 1000;
	constexpr double MillisecondsPerSecondDouble = 1000.0;
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
	constexpr double BaseEnduranceDurationMs = 90000.0;
	constexpr double EnduranceDurationMsPerPoint = 1200.0;
	constexpr double EnduranceDurationReferenceRating = 80.0;
	constexpr double EmptyEndurancePaceMultiplier = 0.2;
	constexpr double MinimumMotionDurationMs = 1.0;
	constexpr double MotionIntegrationTolerance = 1e-5;
	constexpr unsigned int MaximumIntegrationDepth = 8;
	constexpr double FinishTimeToleranceMs = 0.01;
	constexpr double TimeBoundaryToleranceMs = 1e-8;
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
	constexpr std::uint32_t StumbleSeedSalt = 0x1b873593u;
	constexpr std::uint32_t ThoughtSeedSalt = 0x85ebca6bu;
	constexpr std::uint32_t SecondWindSeedSalt = 0xc2b2ae35u;
	constexpr unsigned int MinimumStumbleDurationMs = 300;
	constexpr unsigned int MaximumStumbleDurationMs = 900;
	constexpr unsigned int MinimumThoughtDurationMs = 2000;
	constexpr unsigned int MaximumThoughtDurationMs = 4000;

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
		const double DurationMs = std::max(MinimumMotionDurationMs, CycleDurationMs);
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

	template <typename Function>
	double GaussLegendre4(const Function& Evaluate, double StartTimeMs, double EndTimeMs)
	{
		static constexpr std::array<double, 2> Nodes = { 0.8611363115940526, 0.3399810435848563 };
		static constexpr std::array<double, 2> Weights = { 0.3478548451374538, 0.6521451548625461 };
		const double Midpoint = (StartTimeMs + EndTimeMs) * 0.5;
		const double HalfWidth = (EndTimeMs - StartTimeMs) * 0.5;
		double Sum = 0.0;
		for (std::size_t Index = 0; Index < Nodes.size(); ++Index)
			Sum += Weights[Index] * (Evaluate(Midpoint - HalfWidth * Nodes[Index]) +
				Evaluate(Midpoint + HalfWidth * Nodes[Index]));
		return Sum * HalfWidth;
	}

	template <typename Function>
	double GaussLegendre8(const Function& Evaluate, double StartTimeMs, double EndTimeMs)
	{
		static constexpr std::array<double, 4> Nodes =
			{ 0.9602898564975363, 0.7966664774136267, 0.5255324099163290, 0.1834346424956498 };
		static constexpr std::array<double, 4> Weights =
			{ 0.1012285362903763, 0.2223810344533745, 0.3137066458778870, 0.3626837833783620 };
		const double Midpoint = (StartTimeMs + EndTimeMs) * 0.5;
		const double HalfWidth = (EndTimeMs - StartTimeMs) * 0.5;
		double Sum = 0.0;
		for (std::size_t Index = 0; Index < Nodes.size(); ++Index)
			Sum += Weights[Index] * (Evaluate(Midpoint - HalfWidth * Nodes[Index]) +
				Evaluate(Midpoint + HalfWidth * Nodes[Index]));
		return Sum * HalfWidth;
	}

	template <typename Function>
	double IntegrateMotionSegment(const Function& Evaluate, double StartTimeMs, double EndTimeMs,
		double Tolerance = MotionIntegrationTolerance, unsigned int Depth = MaximumIntegrationDepth)
	{
		if (EndTimeMs <= StartTimeMs)
			return 0.0;

		const double CoarseEstimate = GaussLegendre4(Evaluate, StartTimeMs, EndTimeMs);
		const double FineEstimate = GaussLegendre8(Evaluate, StartTimeMs, EndTimeMs);
		if (Depth == 0 || std::abs(FineEstimate - CoarseEstimate) <= Tolerance)
			return FineEstimate;

		const double Midpoint = (StartTimeMs + EndTimeMs) * 0.5;
		return IntegrateMotionSegment(Evaluate, StartTimeMs, Midpoint, Tolerance * 0.5, Depth - 1) +
			IntegrateMotionSegment(Evaluate, Midpoint, EndTimeMs, Tolerance * 0.5, Depth - 1);
	}

	double EventRatePerSecond(double ProbabilityPerSecond)
	{
		const double Probability = std::clamp(ProbabilityPerSecond, 0.0,
			std::nextafter(1.0, 0.0));
		return Probability == 0.0 ? 0.0 : -std::log1p(-Probability);
	}

	double SampleNextEventTimeMs(double CurrentTimeMs, double ProbabilityPerSecond, std::mt19937& Random)
	{
		const double Rate = EventRatePerSecond(ProbabilityPerSecond);
		if (Rate == 0.0)
			return std::numeric_limits<double>::infinity();

		std::exponential_distribution<double> WaitingTime(Rate);
		const double WaitMs = std::max(MinimumMotionDurationMs, WaitingTime(Random) * MillisecondsPerSecondDouble);
		return CurrentTimeMs + WaitMs;
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
		const double ReducedCurveProgress = Runner.AccelerationCurve.TimeAtValue(TargetPace);
		const double ReducedCurveTimeMs = Stats.ResponseDelayMs + ReducedCurveProgress * Stats.AccelerationDurationMs;
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
		double ChancePerSecond(double Confidence, double Fatigue) const
		{
			return Chance(Confidence, Fatigue);
		}

		void RollEffect(std::mt19937& Random, unsigned int& DurationMs, double& PaceMultiplier) const
		{
			std::uniform_int_distribution<unsigned int> Duration(MinimumStumbleDurationMs, MaximumStumbleDurationMs);
			std::uniform_real_distribution<double> PaceMultipliers(MinimumPaceMultiplier, MaximumPaceMultiplier);
			DurationMs = Duration(Random);
			PaceMultiplier = PaceMultipliers(Random);
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

		void RollEffect(std::mt19937& Random, unsigned int& DurationMs, double& PaceMultiplier) const
		{
			std::uniform_int_distribution<unsigned int> Duration(MinimumThoughtDurationMs, MaximumThoughtDurationMs);
			std::uniform_real_distribution<double> PaceMultipliers(MinimumPaceMultiplier, MaximumPaceMultiplier);
			DurationMs = Duration(Random);
			PaceMultiplier = PaceMultipliers(Random);
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

		double Chance(double Confidence) const
		{
			return Confidence > ConfidenceThreshold
				? (BaseChance + ConfidenceChanceWeight * Confidence) * ChanceScale
				: 0.0;
		}

	public:
		double ChancePerSecond(double Confidence) const
		{
			return Chance(Confidence);
		}

		void RollEffect(std::mt19937& Random, unsigned int& DurationMs, double& PaceMultiplier) const
		{
			std::uniform_int_distribution<unsigned int> Duration(MinimumThoughtDurationMs, MaximumThoughtDurationMs);
			std::uniform_real_distribution<double> PaceMultipliers(MinimumPaceMultiplier, MaximumPaceMultiplier);
			DurationMs = Duration(Random);
			PaceMultiplier = PaceMultipliers(Random);
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
		static constexpr double PaceOverrideDurationMs = 50.0;

		double ChancePerSecond(double Confidence, double Luck) const
		{
			return Chance(Confidence, Luck);
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
			const double EnduranceCycleDurationMs = std::max<double>(MinimumMotionDurationMs,
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
			// Build one immutable race outcome for both background simulation and renderer playback.
			Runner->Reset();
			Stats = CalculateEffectiveRacerStats(*Runner);
			Timeline.PreRaceConfidence = Stats.Confidence;
			Timeline.PreRaceFatigue = Stats.Fatigue;
			Timeline.PreRaceInjury = Stats.Injury;
			const unsigned int EventSeed = MixSeed(LockedSeed, Runner->GUID.GUID, RacerIndex);
			StumbleRandom.seed(EventSeed ^ StumbleSeedSalt);
			ThoughtRandom.seed(EventSeed ^ ThoughtSeedSalt);
			SecondWindRandom.seed(EventSeed ^ SecondWindSeedSalt);
			Timeline.Entrant = Runner;
			Timeline.RacerIndex = static_cast<unsigned int>(RacerIndex);
			InitializeTimelineNodes();
			ScheduleNextStumbleEvent(0.0);
			ScheduleNextThoughtEvent(0.0);
			NextGassingTimeMs = CalculateNextGassingTime();
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
			EnduranceCycleDurationMs = std::max(MinimumMotionDurationMs, Stats.FullEnduranceDurationMs);
		}

		double CalculateNextGassingTime() const
		{
			if (Gassing)
				return std::numeric_limits<double>::infinity();
			if (EnduranceCycleReserve <= GassingRule.EnduranceThreshold)
				return EnduranceCycleStartMs;

			const double TargetCurveReserve = GassingRule.EnduranceThreshold / EnduranceCycleReserve;
			const double CurveProgress = Runner->EnduranceCurve.TimeAtValue(TargetCurveReserve);
			return EnduranceCycleStartMs + EnduranceCycleDurationMs * CurveProgress;
		}

		void ScheduleNextStumbleEvent(double TimeMs)
		{
			NextStumbleEventTimeMs = SampleNextEventTimeMs(TimeMs,
				StumbleRule.ChancePerSecond(Stats.Confidence, Stats.Fatigue), StumbleRandom);
		}

		void ScheduleNextThoughtEvent(double TimeMs)
		{
			const double ChancePerSecond = LostBeliefRule.ChanceFor(Stats.Confidence) +
				BeliefRule.ChancePerSecond(Stats.Confidence);
			NextThoughtEventTimeMs = SampleNextEventTimeMs(TimeMs, ChancePerSecond, ThoughtRandom);
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

		double IntegratePace(double StartTimeMs, double EndTimeMs) const
		{
			if (EndTimeMs <= StartTimeMs)
				return 0.0;
			if (WindOverrideActive && (StartTimeMs + EndTimeMs) * 0.5 < WindOverrideUntilMs)
				return WindOverridePace * (EndTimeMs - StartTimeMs) / MillisecondsPerSecondDouble;

			const double EffectScale = EffectMultiplier((StartTimeMs + EndTimeMs) * 0.5);
			if (Gassing)
			{
				const double StartElapsedMs = std::max(0.0, StartTimeMs - GassingStartMs);
				const double EndElapsedMs = std::max(0.0, EndTimeMs - GassingStartMs);
				const double DeclineEndMs = std::min(EndElapsedMs, GassingRule.DeclineDurationMs);
				double Distance = 0.0;
				if (DeclineEndMs > StartElapsedMs)
				{
					const double LinearAreaMs = DeclineEndMs - StartElapsedMs -
						GassingRule.PaceDeclineFraction *
						(DeclineEndMs * DeclineEndMs - StartElapsedMs * StartElapsedMs) /
						(2.0 * GassingRule.DeclineDurationMs);
					Distance += GassingEntryBasePace * LinearAreaMs / MillisecondsPerSecondDouble;
				}
				if (EndElapsedMs > GassingRule.DeclineDurationMs)
				{
					const double PlateauStartMs = std::max(StartElapsedMs, GassingRule.DeclineDurationMs);
					Distance += GassingEntryBasePace * (1.0 - GassingRule.PaceDeclineFraction) *
						(EndElapsedMs - PlateauStartMs) / MillisecondsPerSecondDouble;
				}
				return EffectScale * Distance;
			}

			const double AccelerationDistance = IntegrateAccelerationPace(StartTimeMs, EndTimeMs);
			const auto EndurancePacePerMillisecond = [this](double TimeMs)
			{
				return NormalPace(TimeMs) * EnduranceReserve(TimeMs) / MillisecondsPerSecondDouble;
			};
			const double EnduranceDistance = IntegrateMotionSegment(EndurancePacePerMillisecond,
				StartTimeMs, EndTimeMs);
			return EffectScale * (EmptyEndurancePaceMultiplier * AccelerationDistance +
				(1.0 - EmptyEndurancePaceMultiplier) * EnduranceDistance);
		}

		double IntegrateAccelerationPace(double StartTimeMs, double EndTimeMs) const
		{
			if (Stats.MaximumPace <= 0.0)
				return 0.0;

			const double CurveStartTimeMs = Stats.ResponseDelayMs + AccelerationCurveTimeOffsetMs;
			const double CurveEndTimeMs = CurveStartTimeMs + Stats.AccelerationDurationMs;
			const double CurvePartStartMs = std::max(StartTimeMs, CurveStartTimeMs);
			const double CurvePartEndMs = std::min(EndTimeMs, CurveEndTimeMs);
			double NormalizedCurveAreaMs = 0.0;
			if (CurvePartEndMs > CurvePartStartMs)
			{
				const double StartProgress = (CurvePartStartMs - CurveStartTimeMs) / Stats.AccelerationDurationMs;
				const double EndProgress = (CurvePartEndMs - CurveStartTimeMs) / Stats.AccelerationDurationMs;
				NormalizedCurveAreaMs = Stats.AccelerationDurationMs *
					Runner->AccelerationCurve.Integrate(StartProgress, EndProgress);
			}
			const double PlateauStartMs = std::max(StartTimeMs, CurveEndTimeMs);
			if (EndTimeMs > PlateauStartMs)
				NormalizedCurveAreaMs += EndTimeMs - PlateauStartMs;
			return Stats.MaximumPace * NormalizedCurveAreaMs / MillisecondsPerSecondDouble;
		}

		double FindFinishTime(double StartTimeMs, double EndTimeMs, double RemainingDistance) const
		{
			double Low = StartTimeMs;
			double High = EndTimeMs;
			while (High - Low > FinishTimeToleranceMs)
			{
				const double Midpoint = (Low + High) * 0.5;
				if (IntegratePace(StartTimeMs, Midpoint) < RemainingDistance)
					Low = Midpoint;
				else
					High = Midpoint;
			}
			return (Low + High) * 0.5;
		}

		void AppendMotionSegment(double StartTimeMs, double EndTimeMs)
		{
			if (EndTimeMs > StartTimeMs)
				Timeline.MotionSegments.push_back({ StartTimeMs, EndTimeMs, Distance });
		}

		double GetNextBoundaryTime(double TimeMs) const
		{
			double NextTimeMs = static_cast<double>(MaximumRaceDurationMs);
			const auto Consider = [&NextTimeMs, TimeMs](double CandidateTimeMs)
			{
				if (CandidateTimeMs > TimeMs + TimeBoundaryToleranceMs)
					NextTimeMs = std::min(NextTimeMs, CandidateTimeMs);
			};

			Consider(NextStumbleEventTimeMs);
			Consider(NextThoughtEventTimeMs);
			if (StumbleTimelineNode != nullptr)
				Consider(StumbleUntilMs);
			if (LostBeliefUntilMs > TimeMs)
				Consider(LostBeliefUntilMs);
			if (BeliefUntilMs > TimeMs)
				Consider(BeliefUntilMs);
			if (WindOverrideActive)
				Consider(WindOverrideUntilMs);

			if (Gassing)
			{
				Consider(NextSecondWindEventTimeMs);
				Consider(GassingPullUpTimeMs);
			}
			else
			{
				Consider(NextGassingTimeMs);
			}

			Consider(Stats.ResponseDelayMs + AccelerationCurveTimeOffsetMs);
			Consider(Stats.ResponseDelayMs + Stats.AccelerationDurationMs + AccelerationCurveTimeOffsetMs);
			return NextTimeMs;
		}

		bool ProcessEventsAtTime(double TimeMs)
		{
			CompleteStumbleIfDue(TimeMs);
			if (!Gassing && NextGassingTimeMs <= TimeMs + TimeBoundaryToleranceMs)
				StartGassingIfDue(TimeMs);

			if (NextStumbleEventTimeMs <= TimeMs + TimeBoundaryToleranceMs)
			{
				TryStumble(TimeMs);
				ScheduleNextStumbleEvent(TimeMs);
			}
			if (NextThoughtEventTimeMs <= TimeMs + TimeBoundaryToleranceMs)
			{
				TryThoughtEvent(TimeMs);
				ScheduleNextThoughtEvent(TimeMs);
			}
			if (Gassing && NextSecondWindEventTimeMs <= TimeMs + TimeBoundaryToleranceMs)
				TrySecondWind(TimeMs);

			return TryPullUp(TimeMs);
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

			if (Gassing)
				return;

			Gassing = true;
			NextGassingTimeMs = std::numeric_limits<double>::infinity();
			GassingStartMs = TimeMs;
			GassingPullUpTimeMs = TimeMs + PullingUpRule.GassingTimeLimitMs(
				Stats.Confidence, Runner->Skills.Composure);
			NextSecondWindEventTimeMs = SampleNextEventTimeMs(TimeMs,
				SecondWindRule.ChancePerSecond(Stats.Confidence, Stats.Luck), SecondWindRandom);
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
			StumbleRule.RollEffect(StumbleRandom, DurationMs, PaceMultiplier);

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

			const double LostBeliefChance = LostBeliefRule.ChanceFor(Stats.Confidence);
			const double BeliefChance = BeliefRule.ChancePerSecond(Stats.Confidence);
			const double TotalChance = LostBeliefChance + BeliefChance;
			if (TotalChance <= 0.0)
				return;

			std::uniform_real_distribution<double> Unit(0.0, TotalChance);
			const bool IsLostBelief = Unit(ThoughtRandom) < LostBeliefChance;
			unsigned int DurationMs = 0;
			double PaceMultiplier = 1.0;
			if (IsLostBelief)
			{
				LostBeliefRule.RollEffect(ThoughtRandom, DurationMs, PaceMultiplier);
				CloseThoughtNode(BeliefTimelineNode, TimeMs);
				CloseThoughtNode(LostBeliefTimelineNode, TimeMs);
				LostBeliefUntilMs = TimeMs + DurationMs;
				LostBeliefPaceMultiplier = PaceMultiplier;
				BeliefUntilMs = 0.0;
				BeliefTimelineNode = nullptr;
				LostBeliefTimelineNode = AddEvent(CreateEventRecord(RaceEventType::LostBelief,
					TimeMs, DurationMs, PaceMultiplier));
			}
			else
			{
				BeliefRule.RollEffect(ThoughtRandom, DurationMs, PaceMultiplier);
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

			if (!Gassing)
				return;

			// Restart endurance from the restored reserve, using a brief pace override to bridge the change.
			CloseGassingEvent(TimeMs);
			NextSecondWindEventTimeMs = std::numeric_limits<double>::infinity();
			const double RestoredReserve = SecondWindRule.RestoredReserve(Stats.Confidence, Stats.Luck);
			WindOverridePace = GassingEntryPace;
			WindOverrideUntilMs = TimeMs + SecondWindEvent::PaceOverrideDurationMs;
			WindOverrideActive = true;
			Gassing = false;
			EnduranceCycleStartMs = TimeMs;
			EnduranceCycleDurationMs = std::max<double>(MinimumMotionDurationMs,
				Stats.FullEnduranceDurationMs * RestoredReserve);
			EnduranceCycleReserve = RestoredReserve;
			NextGassingTimeMs = CalculateNextGassingTime();
			RaceTimelineNode* SecondWindNode = AddEvent(CreateEventRecord(RaceEventType::SecondWind,
				TimeMs, 0, RestoredReserve));
			SecondWindNode->EndTimeMs = TimeMs + SecondWindEvent::PaceOverrideDurationMs;
		}

		bool TryPullUp(double TimeMs)
		{
			using namespace Game::Race;

			// A racer can pull up only while gassing, after a confidence- and composure-dependent limit.
			if (!Gassing || TimeMs + TimeBoundaryToleranceMs < GassingPullUpTimeMs)
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

		void Simulate()
		{
			using namespace Game::Race;

			double TimeMs = 0.0;
			while (TimeMs < MaximumRaceDurationMs)
			{
				if (ProcessEventsAtTime(TimeMs))
					break;

				const double NextTimeMs = GetNextBoundaryTime(TimeMs);
				if (NextTimeMs <= TimeMs + TimeBoundaryToleranceMs)
					break;

				const double DistanceStep = IntegratePace(TimeMs, NextTimeMs);
				if (DistanceStep > 0.0 && Distance + DistanceStep >= TrackLength)
				{
					const double FinishTimeMs = FindFinishTime(TimeMs, NextTimeMs, TrackLength - Distance);
					AppendMotionSegment(TimeMs, FinishTimeMs);
					TimeMs = FinishTimeMs;
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
					break;
				}

				AppendMotionSegment(TimeMs, NextTimeMs);
				Distance += DistanceStep;
				TimeMs = NextTimeMs;
			}

			if (Gassing)
				CloseGassingEvent(Timeline.Finished ? Timeline.FinishTimeMs :
					Timeline.PulledUp ? Timeline.PullUpTimeMs : MaximumRaceDurationMs);
		}

		Game::Race::Racer* Runner;
		std::size_t RacerIndex;
		unsigned int LockedSeed;
		unsigned int TrackLength;
		Game::Race::RacerRaceTimeline Timeline;
		EffectiveRacerStats Stats;
		std::mt19937 StumbleRandom;
		std::mt19937 ThoughtRandom;
		std::mt19937 SecondWindRandom;
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
		double GassingPullUpTimeMs = 0.0;
		double NextStumbleEventTimeMs = std::numeric_limits<double>::infinity();
		double NextThoughtEventTimeMs = std::numeric_limits<double>::infinity();
		double NextSecondWindEventTimeMs = std::numeric_limits<double>::infinity();
		double NextGassingTimeMs = std::numeric_limits<double>::infinity();
		bool Gassing = false;
		Game::Race::RaceTimelineNode* AccelerationTimelineNode = nullptr;
		Game::Race::RaceTimelineNode* StumbleTimelineNode = nullptr;
		Game::Race::RaceTimelineNode* GassingTimelineNode = nullptr;
		Game::Race::RaceTimelineNode* LostBeliefTimelineNode = nullptr;
		Game::Race::RaceTimelineNode* BeliefTimelineNode = nullptr;
		double Distance = 0.0;
	};
}
using namespace RaceSimulationDetail;

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
	: Race(static_cast<unsigned int>(std::rand()))
{
}

Game::Race::Race::Race(unsigned int RaceSeed)
	: TimelineReady(false), ConditionsApplied(false), Complete(false), PlaybackTimeMs(0.0), TimelineDurationMs(0.0),
	LockedSeed(0), LockedTrackLength(0), Seed(RaceSeed), CurrentTick(0), ThisTrack(std::make_shared<Track>())
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

bool Game::Race::Race::GetTimelinePositionAt(const Racer* Runner, double TimeMs, double& Position) const
{
	const RacerRaceTimeline* Timeline = GetTimelineFor(Runner);
	if (Timeline == nullptr || !std::isfinite(TimeMs))
		return false;

	const double TimelineEndTimeMs = Timeline->Finished ? Timeline->FinishTimeMs :
		Timeline->PulledUp ? Timeline->PullUpTimeMs : TimelineDurationMs;
	Position = GetTimelineDistanceAt(*Timeline, std::clamp(TimeMs, 0.0, TimelineEndTimeMs));
	return true;
}

bool Game::Race::IsValidRaceHistorySnapshot(const RaceHistorySnapshot& Snapshot)
{
	const std::size_t EntrantCount = Snapshot.Entrants.size();
	if (Snapshot.Error != RaceError::None || Snapshot.TrackLength == 0 || EntrantCount < 2 ||
		Snapshot.Timelines.size() != EntrantCount || Snapshot.Results.size() != EntrantCount ||
		!std::isfinite(Snapshot.PlaybackTimeMs) || !std::isfinite(Snapshot.TimelineDurationMs) ||
		Snapshot.PlaybackTimeMs < 0.0 || Snapshot.TimelineDurationMs < Snapshot.PlaybackTimeMs)
		return false;
	const auto IsValidCurve = [](const RaceCurve& Curve)
	{
		return std::isfinite(Curve.X1) && std::isfinite(Curve.Y1) &&
			std::isfinite(Curve.X2) && std::isfinite(Curve.Y2);
	};
	for (const RaceHistoryRacerSnapshot& Entrant : Snapshot.Entrants)
		if (!IsValidCurve(Entrant.AccelerationCurve) || !IsValidCurve(Entrant.EnduranceCurve))
			return false;

	std::vector<bool> TimelineIndices(EntrantCount, false);
	for (const RaceHistoryTimelineSnapshot& Timeline : Snapshot.Timelines)
	{
		if (Timeline.RacerIndex >= EntrantCount || TimelineIndices[Timeline.RacerIndex])
			return false;
		TimelineIndices[Timeline.RacerIndex] = true;
		for (const RaceTimelineNode& Node : Timeline.Nodes)
			if ((Node.Type == RaceTimelineNodeType::RandomEvent || Node.Type == RaceTimelineNodeType::Gassing) &&
				Node.Event.RacerIndex >= EntrantCount)
				return false;
	}

	std::vector<bool> ResultIndices(EntrantCount, false);
	for (const RaceHistoryResultSnapshot& Result : Snapshot.Results)
	{
		if (Result.RacerIndex >= EntrantCount || ResultIndices[Result.RacerIndex])
			return false;
		ResultIndices[Result.RacerIndex] = true;
	}
	return true;
}

bool Game::Race::Race::CaptureHistorySnapshot(RaceHistorySnapshot& Snapshot) const
{
	if (!Complete || !TimelineReady || Result.Error != RaceError::None ||
		LockedEntrants.size() != Timelines.size() || Result.RacerResults.size() != LockedEntrants.size())
		return false;

	RaceHistorySnapshot Captured;
	Captured.GUID = GUID;
	Captured.Seed = LockedSeed;
	Captured.TrackLength = LockedTrackLength;
	Captured.CurrentTick = CurrentTick;
	Captured.Error = Result.Error;
	Captured.Financials = Financials;
	Captured.PlaybackTimeMs = PlaybackTimeMs;
	Captured.TimelineDurationMs = TimelineDurationMs;
	Captured.Entrants.reserve(LockedEntrants.size());
	Captured.Timelines.reserve(Timelines.size());
	Captured.Results.reserve(Result.RacerResults.size());

	const auto GetEntrantIndex = [this](const Racer* Runner, unsigned int& RacerIndex)
	{
		const auto Entrant = std::find(LockedEntrants.begin(), LockedEntrants.end(), Runner);
		if (Entrant == LockedEntrants.end())
			return false;
		RacerIndex = static_cast<unsigned int>(Entrant - LockedEntrants.begin());
		return true;
	};

	for (const Racer* Runner : LockedEntrants)
	{
		if (Runner == nullptr)
			return false;
		const RacerRaceTimeline* Timeline = GetTimelineFor(Runner);
		if (Timeline == nullptr)
			return false;
		RaceHistoryRacerSnapshot RacerSnapshot;
		RacerSnapshot.GUID = Runner->GUID;
		RacerSnapshot.Name = Runner->Name;
		RacerSnapshot.Skills = Runner->Skills;
		RacerSnapshot.Details = Runner->Details;
		RacerSnapshot.Conditions.Fatigue = Timeline->PreRaceFatigue;
		RacerSnapshot.Conditions.Confidence = Timeline->PreRaceConfidence;
		RacerSnapshot.Conditions.Injury = Timeline->PreRaceInjury;
		RacerSnapshot.CurveSeed = Runner->CurveSeed;
		RacerSnapshot.AccelerationCurve = Runner->AccelerationCurve;
		RacerSnapshot.EnduranceCurve = Runner->EnduranceCurve;
		Captured.Entrants.push_back(std::move(RacerSnapshot));
	}

	for (const RacerRaceTimeline& Timeline : Timelines)
	{
		RaceHistoryTimelineSnapshot TimelineSnapshot;
		if (!GetEntrantIndex(Timeline.Entrant, TimelineSnapshot.RacerIndex))
			return false;
		TimelineSnapshot.PreRaceConfidence = Timeline.PreRaceConfidence;
		TimelineSnapshot.PreRaceFatigue = Timeline.PreRaceFatigue;
		TimelineSnapshot.PreRaceInjury = Timeline.PreRaceInjury;
		TimelineSnapshot.MotionSegments = Timeline.MotionSegments;
		TimelineSnapshot.FinalDistance = Timeline.FinalDistance;
		TimelineSnapshot.FinishTimeMs = Timeline.FinishTimeMs;
		TimelineSnapshot.PullUpTimeMs = Timeline.PullUpTimeMs;
		TimelineSnapshot.PullUpDistance = Timeline.PullUpDistance;
		TimelineSnapshot.FinishPosition = Timeline.FinishPosition;
		TimelineSnapshot.Finished = Timeline.Finished;
		TimelineSnapshot.PulledUp = Timeline.PulledUp;
		TimelineSnapshot.FatigueAfter = Timeline.FatigueAfter;
		TimelineSnapshot.ConfidenceAfter = Timeline.ConfidenceAfter;
		TimelineSnapshot.InjuryAfter = Timeline.InjuryAfter;
		TimelineSnapshot.CurrentEndurance = Timeline.CurrentEndurance;
		TimelineSnapshot.ActiveEffects = Timeline.ActiveEffects;
		for (const RaceTimelineNode& Node : Timeline.Nodes)
		{
			RaceTimelineNode NodeSnapshot = Node;
			if ((Node.Type == RaceTimelineNodeType::RandomEvent || Node.Type == RaceTimelineNodeType::Gassing) &&
				!GetEntrantIndex(Node.Event.Entrant, NodeSnapshot.Event.RacerIndex))
				return false;
			NodeSnapshot.Event.Entrant = nullptr;
			TimelineSnapshot.Nodes.push_back(std::move(NodeSnapshot));
		}
		Captured.Timelines.push_back(std::move(TimelineSnapshot));
	}

	for (const RacerRaceResult& RacerResult : Result.RacerResults)
	{
		RaceHistoryResultSnapshot ResultSnapshot;
		if (!GetEntrantIndex(RacerResult.Racer, ResultSnapshot.RacerIndex))
			return false;
		ResultSnapshot.Position = RacerResult.Position;
		ResultSnapshot.Ms = RacerResult.Ms;
		Captured.Results.push_back(ResultSnapshot);
	}

	if (!IsValidRaceHistorySnapshot(Captured))
		return false;
	Snapshot = std::move(Captured);
	return true;
}

bool Game::Race::Race::RestoreHistorySnapshot(const RaceHistorySnapshot& Snapshot,
	const std::vector<Racer*>& SnapshotEntrants)
{
	if (!IsValidRaceHistorySnapshot(Snapshot) || SnapshotEntrants.size() != Snapshot.Entrants.size())
		return false;
	for (std::size_t Index = 0; Index < SnapshotEntrants.size(); ++Index)
		if (SnapshotEntrants[Index] == nullptr ||
			SnapshotEntrants[Index]->GUID.GUID != Snapshot.Entrants[Index].GUID.GUID)
			return false;

	Seed = Snapshot.Seed;
	GUID = Snapshot.GUID;
	CurrentTick = Snapshot.CurrentTick;
	Financials = Snapshot.Financials;
	if (ThisTrack == nullptr)
		ThisTrack = std::make_shared<Track>();
	ThisTrack->Length = static_cast<TrackLength>(Snapshot.TrackLength);
	Racers = SnapshotEntrants;
	LockedEntrants = SnapshotEntrants;
	LockedSeed = Snapshot.Seed;
	LockedTrackLength = Snapshot.TrackLength;
	PlaybackTimeMs = Snapshot.PlaybackTimeMs;
	TimelineDurationMs = Snapshot.TimelineDurationMs;
	TimelineReady = true;
	ConditionsApplied = true;
	Complete = true;
	Timelines.clear();
	Events.clear();
	Timelines.reserve(Snapshot.Timelines.size());
	for (const RaceHistoryTimelineSnapshot& TimelineSnapshot : Snapshot.Timelines)
	{
		RacerRaceTimeline Timeline;
		Timeline.Entrant = SnapshotEntrants[TimelineSnapshot.RacerIndex];
		Timeline.RacerIndex = TimelineSnapshot.RacerIndex;
		Timeline.PreRaceConfidence = TimelineSnapshot.PreRaceConfidence;
		Timeline.PreRaceFatigue = TimelineSnapshot.PreRaceFatigue;
		Timeline.PreRaceInjury = TimelineSnapshot.PreRaceInjury;
		Timeline.MotionSegments = TimelineSnapshot.MotionSegments;
		Timeline.FinalDistance = TimelineSnapshot.FinalDistance;
		Timeline.FinishTimeMs = TimelineSnapshot.FinishTimeMs;
		Timeline.PullUpTimeMs = TimelineSnapshot.PullUpTimeMs;
		Timeline.PullUpDistance = TimelineSnapshot.PullUpDistance;
		Timeline.FinishPosition = TimelineSnapshot.FinishPosition;
		Timeline.Finished = TimelineSnapshot.Finished;
		Timeline.PulledUp = TimelineSnapshot.PulledUp;
		Timeline.FatigueAfter = TimelineSnapshot.FatigueAfter;
		Timeline.ConfidenceAfter = TimelineSnapshot.ConfidenceAfter;
		Timeline.InjuryAfter = TimelineSnapshot.InjuryAfter;
		Timeline.CurrentEndurance = TimelineSnapshot.CurrentEndurance;
		Timeline.ActiveEffects = TimelineSnapshot.ActiveEffects;
		for (const RaceTimelineNode& NodeSnapshot : TimelineSnapshot.Nodes)
		{
			RaceTimelineNode Node = NodeSnapshot;
			if (Node.Type == RaceTimelineNodeType::RandomEvent || Node.Type == RaceTimelineNodeType::Gassing)
			{
				Node.Event.Entrant = SnapshotEntrants[Node.Event.RacerIndex];
				Events.push_back(Node.Event);
			}
			else
				Node.Event.Entrant = nullptr;
			Timeline.Nodes.push_back(std::move(Node));
		}
		Timelines.push_back(std::move(Timeline));
	}

	std::stable_sort(Events.begin(), Events.end(), [](const RaceEvent& Left, const RaceEvent& Right)
		{
			if (Left.TimeMs != Right.TimeMs)
				return Left.TimeMs < Right.TimeMs;
			return Left.RacerIndex < Right.RacerIndex;
		});

	Result = RaceResult{};
	Result.GUID = Snapshot.GUID;
	Result.TrackLength = Snapshot.TrackLength;
	Result.Error = Snapshot.Error;
	Result.RacerResults.reserve(Snapshot.Results.size());
	for (const RaceHistoryResultSnapshot& ResultSnapshot : Snapshot.Results)
	{
		Racer* Runner = SnapshotEntrants[ResultSnapshot.RacerIndex];
		Result.RacerResults.emplace_back(Runner, ResultSnapshot.Position, ResultSnapshot.Ms, Snapshot.GUID);
	}

	UpdatePlaybackStateAt(PlaybackTimeMs);
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
	Result.TrackLength = LockedTrackLength;
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

double Game::Race::Race::GetTimelineDistanceAt(const RacerRaceTimeline& Timeline, double TimeMs) const
{
	if (Timeline.Finished && TimeMs >= Timeline.FinishTimeMs)
		return static_cast<double>(LockedTrackLength);
	if (Timeline.PulledUp && TimeMs >= Timeline.PullUpTimeMs)
		return Timeline.PullUpDistance;
	if (Timeline.MotionSegments.empty())
		return 0.0;

	auto Segment = std::upper_bound(Timeline.MotionSegments.begin(), Timeline.MotionSegments.end(), TimeMs,
		[](double Time, const RaceMotionSegment& Entry)
		{
			return Time < Entry.StartTimeMs;
		});
	if (Segment != Timeline.MotionSegments.begin())
		--Segment;

	const double TargetTimeMs = std::clamp(TimeMs, Segment->StartTimeMs, Segment->EndTimeMs);
	const auto PacePerMillisecond = [&Timeline](double Time)
	{
		return EvaluateTimelinePlaybackState(Timeline, Time).Pace / MillisecondsPerSecondDouble;
	};
	const double Distance = Segment->StartDistance +
		IntegrateMotionSegment(PacePerMillisecond, Segment->StartTimeMs, TargetTimeMs);
	return std::clamp(Distance, 0.0, static_cast<double>(LockedTrackLength));
}

void Game::Race::Race::UpdatePlaybackStateAt(double TimeMs)
{
	for (RacerRaceTimeline& Timeline : Timelines)
	{
		if (Timeline.Entrant == nullptr)
			continue;

		const double TerminalTimeMs = Timeline.Finished ? Timeline.FinishTimeMs :
			Timeline.PulledUp ? Timeline.PullUpTimeMs : TimelineDurationMs;
		const double SampleTimeMs = std::clamp(TimeMs, 0.0, TerminalTimeMs);
		const double Distance = GetTimelineDistanceAt(Timeline, SampleTimeMs);
		const TimelinePlaybackState State = EvaluateTimelinePlaybackState(Timeline, SampleTimeMs);

		Timeline.Entrant->Pos.X = static_cast<int>(std::lround(Distance));
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
	if (Precompute() != RaceError::None)
		return Result;
	if (Complete)
		return Result;

	PlaybackTimeMs = TimelineDurationMs;
	CurrentTick = ToMilliseconds(PlaybackTimeMs);
	UpdatePlaybackStateAt(PlaybackTimeMs);
	RecordResultsThrough(PlaybackTimeMs);
	Complete = true;
	ApplyPersistentConditions();
	return Result;
}

void Game::Race::Race::Reset()
{
	Result.RacerResults.clear();
	Result.GUID = GUID;
	Result.TrackLength = 0;
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
	case RaceError::SimulationFailed:
		return "Career race simulation failed.";
	default:
		return "Unknown race error.";
	}
}
