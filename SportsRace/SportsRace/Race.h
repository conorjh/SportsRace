#ifndef RACE_H
#define RACE_H
#include "Racer.h"
#include "State.h"
#include <vector>
#include <string>
namespace Game
{
	namespace Race
	{
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
			Track() {};
			TrackLength Length = TrackLength::Standard100m;
		};

		struct RaceGUID
		{
			unsigned int GUID;
		};

		struct RacerRaceResult
		{
			RacerRaceResult(Racer* _Racer, unsigned int _Position, unsigned int _Ms, RaceGUID _GUID)
			{
				GUID = _GUID;
				Racer = _Racer;
				Position = _Position;
				Ms = _Ms;
			}

			RaceGUID GUID;
			unsigned int Position, Ms;
			Racer* Racer;
		};

		struct RaceResult
		{

			RaceGUID GUID;
			std::vector<RacerRaceResult> RacerResults;
		};

		enum class RaceStatus
		{
			Racing, Finishing, Finished
		};

		struct RaceFinancials
		{
			RaceFinancials();
			RaceFinancials(unsigned int irstPlacePrize, unsigned int EntranceFee);
			unsigned int FirstPlacePrize, SecondPlacePrize, ThirdPlacePrize;
			unsigned int EntranceFee;
		};

		class Race
		{
			void Finished(Racer*);

		public:
			Race();

			bool HasFinished(), HasFinished(Racer*);
			unsigned int FinishedCount() { return Result.RacerResults.size(); }
			Racer* CurrentWinner();
			unsigned int CurrentWinnerDistance();

			Racer* Get(RacerGUID Guid);
			bool Contains(RacerGUID Guid);

			RaceStatus Tick(unsigned int Ms, RaceStatus Type);
			RaceResult Sim();

			void Reset();

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