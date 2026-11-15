#ifndef RACE_H
#define RACE_H
#include "Racer.h"
#include "Screens.h"
#include <list>
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
			Standard100m = 5000,
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
			None, SeedLocked, MissingTrack, ZeroLengthTrack
		};

		struct RaceResult
		{
			RaceGUID GUID;
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

		struct RaceTimelineNode
		{
			RaceTimelineNodeType Type = RaceTimelineNodeType::InitialDelay;
			double StartTimeMs = 0.0;
			double EndTimeMs = 0.0;
			double Distance = 0.0;
			double StartPace = 0.0;
			double EndPace = 0.0;
			RaceEvent Event;
		};

		struct RacerRaceTimeline
		{
			Racer* Entrant = nullptr;
			unsigned int RacerIndex = 0;
			double PreRaceConfidence = RacerConditions::InitialConfidence;
			double PreRaceFatigue = 0.0;
			double PreRaceInjury = 0.0;
			std::list<RaceTimelineNode> Nodes;
			double FinalDistance = 0.0;
			double PlaybackDistance = 0.0;
			double PlaybackIntegratedTimeMs = 0.0;
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

		class Race
		{
			void Finished(Racer*, unsigned int FinishTimeMs);
			void AddPulledUpResult(Racer*, unsigned int PullUpTimeMs);
			bool HasResult(Racer*) const;
			void RecordResultsThrough(double TimeMs);
			void UpdatePlaybackStateAt(double TimeMs);
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

			double GetTimelineDurationMs() const
			{
				return Complete ? TimelineDurationMs : 0.0;
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
			Track* ThisTrack;
			std::vector<Racer*> Racers;
			RaceResult Result;
			RaceFinancials Financials;
			RaceGUID GUID;
		};
	}
}
#endif