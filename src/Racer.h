#ifndef RACER_H
#define RACER_H
#include <string>
#include <vector>
#include <unordered_map>
#include <cstddef>
#include <cstdint>

namespace Game
{
	namespace Race
	{
		class RacerNameMaker
		{
			std::vector<std::string> FirstNames, SecondNames;
		public:
			RacerNameMaker();

			std::string Make();
		};
		class Racer;
		class RacerMaker
		{
			Racer* Make();
		};

		struct Position
		{
			int X;
			double Velocity;
		};

		struct RacerGUID
		{
			RacerGUID()
			{
				GUID = 0;
			}

			RacerGUID(unsigned int _GUID)
			{
				GUID = _GUID;
			}

			bool operator==(const RacerGUID& Other);
			bool operator==(RacerGUID& Other);
			RacerGUID& operator=(const RacerGUID& Other);
			RacerGUID& operator=(const unsigned int& Other);
			RacerGUID& operator=(const int& Other);

			unsigned int GUID;
		};

		struct RacerSkills
		{
			RacerSkills();

			std::uint64_t Xp;

			double Reflex, Composure, Acceleration, Endurance, Speed;
			double Potential, Trainability, Luck;
		};

		struct RacerDetails
		{
			RacerDetails();
			double HeightCm, WeightKg;
			unsigned int Age;
		};

		struct RacerConditions
		{
			static constexpr double InitialFatigue = 0.0;
			static constexpr double InitialConfidence = 0.5;
			static constexpr double InitialInjury = 0.0;

			double Fatigue = InitialFatigue;
			double Confidence = InitialConfidence;
			double Injury = InitialInjury;
		};

		struct RacerCareerHistoryEntry
		{
			std::uint64_t WorldTimeMs = 0;
			std::string EventName;
			unsigned int TrackLength = 0;
			unsigned int Position = 0;
			unsigned int FinishTimeMs = 0;
		};

		struct RacerCareerRecord
		{
			std::uint64_t Wins = 0;
			std::uint64_t Losses = 0;
			std::uint64_t RacesEntered = 0;
			std::uint64_t PositionPoints = 0;
			unsigned int BestTimeMs = 0;
			std::vector<RacerCareerHistoryEntry> History;
		};

		struct RaceCurve
		{
			double X1, Y1, X2, Y2;
			bool Descending;

			static RaceCurve Generate(unsigned int Seed, bool Descending);
			double Evaluate(double Time) const;
			double TimeAtValue(double Value) const;
			double Integrate(double StartTime, double EndTime) const;
		};

		// Profile attributes and deterministic curves accompany the racer state advanced during playback.
		class Racer
		{
		public:
			Racer();
			Racer(std::string);

			void Tick(unsigned int Ms);
			void Reset();
			void RecordCareerResult(unsigned int Position, unsigned int FinishTimeMs);
			void RecordCareerResult(unsigned int Position, unsigned int FinishTimeMs,
				const RacerCareerHistoryEntry& HistoryEntry);

			RacerGUID GUID;
			unsigned int CurrentTick;
			std::string Name;
			unsigned int RunFrame, LastRunFrameEnd;
			RacerSkills Skills;
			RacerDetails Details;
			RacerConditions Conditions;
			RacerCareerRecord CareerRecord;
			unsigned int CurveSeed;
			RaceCurve AccelerationCurve, EnduranceCurve;
			Position Pos;
		};

		class RacerDB
		{
			std::unordered_map<unsigned int, Racer*> Container;
		public:
			RacerDB() = default;
			~RacerDB();
			RacerDB(const RacerDB&) = delete;
			RacerDB& operator=(const RacerDB&) = delete;

			Racer* Make(std::string Name = "");
			Racer* Get(unsigned int GUID);
			const Racer* Get(unsigned int GUID) const;
			std::vector<Racer*> GetAll() const;
			std::size_t Size() const;
			void Add(Racer*);

			bool Contains(unsigned int GUID)
			{
				return Get(GUID) != nullptr;
			}
		};
	}
}

#endif