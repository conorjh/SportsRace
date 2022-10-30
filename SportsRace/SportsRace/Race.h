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
		struct Track
		{
			Track() {};
			unsigned int Length = 5000;
		};

		struct RacerRaceResult
		{
			RacerRaceResult(Racer* _Racer, unsigned int _Position, unsigned int _Ms)
			{
				Racer = _Racer;
				Position = _Position;
				Ms = _Ms;
			}
			unsigned int Position, Ms;
			Racer* Racer;
		};

		struct RaceResult
		{
			std::vector<RacerRaceResult> RacerResults;
		};

		enum class RaceStatus
		{
			Racing, Finishing, Finished
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

			Racer* Get(std::string RacerName);
			bool Contains(std::string RacerName);

			RaceStatus Tick(unsigned int Ms, RaceStatus Type);
			RaceResult Sim();

			void Reset();

			unsigned int CurrentTick;
			Track* ThisTrack;
			std::vector<Racer*> Racers;
			RaceResult Result;
		};
	}
}
#endif