#ifndef RACE_H
#define RACE_H
#include "Racer.h"
#include <list>
#include <memory>
#include <vector>
#include <string>

namespace Game
{
	namespace Race
	{
		inline constexpr unsigned int FirstPlaceRank = 1;
		inline constexpr unsigned int SecondPlaceRank = FirstPlaceRank + 1;
		inline constexpr unsigned int ThirdPlaceRank = SecondPlaceRank + 1;
		inline constexpr unsigned int PulledUpPosition = 0;
		inline constexpr unsigned int FirstPlaceResultIndex = 0;
		inline constexpr unsigned int SecondPlaceResultIndex = FirstPlaceResultIndex + 1;
		inline constexpr unsigned int RaceTickIntervalMs = 33;

		enum class TrackLength : unsigned int
		{
			Standard100m = 2500,
			Standard200m = Standard100m * 2,
			Standard250m = unsigned int(Standard100m * 2.5),
			Standard500m = Standard100m * 5,
			Standard1000m = Standard100m * 10
		};

		struct Track
		{
			Track()
			{
			}

			TrackLength Length = TrackLength::Standard100m;
		};

		struct RaceGUID
		{
			unsigned int GUID = 0;
		};

		struct RacerRaceResult
		{
			RacerRaceResult(Racer* _Racer, unsigned int _Position, unsigned int _Ms, RaceGUID _GUID)
				: GUID(_GUID), Position(_Position), Ms(_Ms), Racer(_Racer)
			{
			}

			RaceGUID GUID;
			unsigned int Position, Ms;
			Racer* Racer;
		};

		enum class RaceError
		{
			None, SeedLocked, MissingTrack, ZeroLengthTrack, SimulationFailed
		};

		struct RaceResult
		{
			RaceGUID GUID;
			unsigned int TrackLength = 0;
			std::vector<RacerRaceResult> RacerResults;
			RaceError Error = RaceError::None;
		};

		std::string RaceErrorToString(RaceError Error);

		enum class RaceStatus
		{
			Racing, Finishing, Finished, Failed
		};

		enum class RaceEventType
		{
			Gassing, Stumble, SecondWind, LostBelief, BelievesInSelf, Injury, PullingUp
		};

		enum RaceEffectMask : unsigned int
		{
			RaceEffectNone = 0,
			RaceEffectStumble = 1u << 0,
			RaceEffectGassing = 1u << 1,
			RaceEffectLostBelief = 1u << 2,
			RaceEffectBelievesInSelf = 1u << 3,
			RaceEffectSecondWind = 1u << 4,
			RaceEffectPulledUp = 1u << 5
		};

		struct RaceEvent
		{
			Racer* Entrant = nullptr;
			unsigned int RacerIndex = 0;
			RaceEventType Type = RaceEventType::Gassing;
			double TimeMs = 0.0;
			unsigned int DurationMs = 0;
			double Magnitude = 0.0;
		};

		enum class RaceTimelineNodeType
		{
			InitialDelay,
			AccelerationToTopSpeed,
			RandomEvent,
			Gassing,
			Finish
		};

		// A timed segment or event in one entrant's precomputed race.
		struct RaceTimelineNode
		{
			RaceTimelineNodeType Type = RaceTimelineNodeType::InitialDelay;
			double StartTimeMs = 0.0;
			double EndTimeMs = 0.0;
			double Distance = 0.0;
			double StartPace = 0.0;
			double EndPace = 0.0;
			// Cumulative time removed from the curve so acceleration resumes at the stumble-reduced pace.
			double AccelerationCurveTimeOffsetMs = 0.0;
			RaceEvent Event;
		};

		struct RaceMotionSegment
		{
			double StartTimeMs = 0.0;
			double EndTimeMs = 0.0;
			double StartDistance = 0.0;
		};

		// One entrant's precomputed outcome and sparse motion intervals for arbitrary-time queries.
		struct RacerRaceTimeline
		{
			Racer* Entrant = nullptr;
			unsigned int RacerIndex = 0;
			// Snapshot conditions so playback uses the stats locked when this entrant's timeline was built.
			double PreRaceConfidence = RacerConditions::InitialConfidence;
			double PreRaceFatigue = 0.0;
			double PreRaceInjury = 0.0;
			std::list<RaceTimelineNode> Nodes;
			std::vector<RaceMotionSegment> MotionSegments;
			double FinalDistance = 0.0;
			double FinishTimeMs = 0.0;
			double PullUpTimeMs = 0.0;
			double PullUpDistance = 0.0;
			unsigned int FinishPosition = 0;
			bool Finished = false;
			bool PulledUp = false;
			double FatigueAfter = 0.0;
			double ConfidenceAfter = RacerConditions::InitialConfidence;
			double InjuryAfter = 0.0;
			double CurrentEndurance = 1.0;
			unsigned int ActiveEffects = RaceEffectNone;
		};

		// The pace, endurance, and active effects sampled at one playback time.
		struct RaceTimelineState
		{
			double Pace = 0.0;
			double Endurance = 1.0;
			unsigned int ActiveEffects = RaceEffectNone;
		};

		struct RaceFinancials
		{
			RaceFinancials();
			RaceFinancials(unsigned int FirstPlacePrize, unsigned int EntranceFee);

			unsigned int FirstPlacePrize, SecondPlacePrize, ThirdPlacePrize;
			unsigned int EntranceFee;
		};

		struct RaceHistoryRacerSnapshot
		{
			RacerGUID GUID;
			std::string Name;
			RacerSkills Skills;
			RacerDetails Details;
			RacerConditions Conditions;
			unsigned int CurveSeed = 0;
			RaceCurve AccelerationCurve{ 0.0, 0.0, 1.0, 1.0, false };
			RaceCurve EnduranceCurve{ 0.0, 1.0, 1.0, 0.0, true };
		};

		struct RaceHistoryTimelineSnapshot
		{
			unsigned int RacerIndex = 0;
			double PreRaceConfidence = RacerConditions::InitialConfidence;
			double PreRaceFatigue = 0.0;
			double PreRaceInjury = 0.0;
			std::list<RaceTimelineNode> Nodes;
			std::vector<RaceMotionSegment> MotionSegments;
			double FinalDistance = 0.0;
			double FinishTimeMs = 0.0;
			double PullUpTimeMs = 0.0;
			double PullUpDistance = 0.0;
			unsigned int FinishPosition = 0;
			bool Finished = false;
			bool PulledUp = false;
			double FatigueAfter = 0.0;
			double ConfidenceAfter = RacerConditions::InitialConfidence;
			double InjuryAfter = 0.0;
			double CurrentEndurance = 1.0;
			unsigned int ActiveEffects = RaceEffectNone;
		};

		struct RaceHistoryResultSnapshot
		{
			unsigned int RacerIndex = 0;
			unsigned int Position = 0;
			unsigned int Ms = 0;
		};

		struct RaceHistorySnapshot
		{
			RaceGUID GUID;
			unsigned int Seed = 0;
			unsigned int TrackLength = 0;
			unsigned int CurrentTick = 0;
			RaceError Error = RaceError::None;
			RaceFinancials Financials;
			double PlaybackTimeMs = 0.0;
			double TimelineDurationMs = 0.0;
			std::vector<RaceHistoryRacerSnapshot> Entrants;
			std::vector<RaceHistoryTimelineSnapshot> Timelines;
			std::vector<RaceHistoryResultSnapshot> Results;
		};

		bool IsValidRaceHistorySnapshot(const RaceHistorySnapshot& Snapshot);

		// Precompute one outcome; player races tick playback while background viewers sample the timeline.
		class Race
		{
			void Finished(Racer*, unsigned int FinishTimeMs);
			void AddPulledUpResult(Racer*, unsigned int PullUpTimeMs);
			bool HasResult(Racer*) const;
			void RecordResultsThrough(double TimeMs);
			void UpdatePlaybackStateAt(double TimeMs);
			double GetTimelineDistanceAt(const RacerRaceTimeline& Timeline, double TimeMs) const;
			void ApplyPersistentConditions();
			void PrecomputeRacerTimelines();
			void AssignTimelineFinishPositions();
			void CalculateTimelineConditions();
			void SortTimelineNodesAndCollectEvents();

			std::vector<Racer*> LockedEntrants;
			std::vector<RacerRaceTimeline> Timelines;
			std::vector<RaceEvent> Events;
			bool TimelineReady;
			bool ConditionsApplied;
			bool Complete;
			double PlaybackTimeMs;
			double TimelineDurationMs;
			unsigned int LockedSeed;
			unsigned int LockedTrackLength;

		public:
			Race();
			explicit Race(unsigned int RaceSeed);

			bool HasFinished() const;
			bool HasFinished(Racer* Racer) const;
			bool IsComplete() const
			{
				return Complete;
			}

			bool IsPrecomputed() const
			{
				return TimelineReady;
			}

			unsigned int FinishedCount() const;
			unsigned int PulledUpCount() const;

			unsigned int GetSeed() const
			{
				return TimelineReady ? LockedSeed : Seed;
			}

			unsigned int GetTrackLength() const;
			double GetCurrentTimeMs() const
			{
				return PlaybackTimeMs;
			}
			const RacerRaceTimeline* GetTimelineFor(const Racer* Runner) const;
			bool GetTimelineStateAt(const Racer* Runner, double TimeMs, RaceTimelineState& State) const;
			bool GetTimelinePositionAt(const Racer* Runner, double TimeMs, double& Position) const;
			bool SamplePlaybackAt(double TimeMs);
			bool CaptureHistorySnapshot(RaceHistorySnapshot& Snapshot) const;
			bool RestoreHistorySnapshot(const RaceHistorySnapshot& Snapshot,
				const std::vector<Racer*>& SnapshotEntrants);

			double GetTimelineDurationMs() const
			{
				return Complete ? TimelineDurationMs : 0.0;
			}
			double GetPrecomputedDurationMs() const
			{
				return TimelineReady ? TimelineDurationMs : 0.0;
			}

			const std::vector<Racer*>& GetEntrants() const
			{
				return TimelineReady ? LockedEntrants : Racers;
			}

			std::vector<RaceEvent> GetEventsThroughPlaybackTime() const;
			unsigned int GetActiveEffects(const Racer* Racer) const;
			double GetCurrentEndurance(const Racer* Racer) const;

			RaceError SetSeed(unsigned int RaceSeed);
			RaceError Precompute();
			Racer* CurrentWinner();
			unsigned int CurrentWinnerDistance();

			Racer* Get(RacerGUID Guid);
			Racer* GetByRank(unsigned int Rank);
			bool Contains(RacerGUID Guid);

			RaceStatus Tick(unsigned int Ms, RaceStatus Type);
			RaceResult Sim();

			void Reset();

			unsigned int Seed;
			unsigned int CurrentTick;
			std::shared_ptr<Track> ThisTrack;
			std::vector<Racer*> Racers;
			RaceResult Result;
			RaceFinancials Financials;
			RaceGUID GUID;
		};
	}
}
#endif