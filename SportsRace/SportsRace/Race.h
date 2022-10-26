#ifndef RACE_H
#define RACE_H
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

		struct Position
		{
			int X;
			double Velocity;
		};

		class Racer
		{
		public:
			Racer(std::string);

			void Tick(unsigned int Ms);

			std::string Name;
			Position Pos;
		};

		enum class RaceStateType
		{
			PreRace, StartersOrders, Racing, Finishing, Finished
		};

		std::string RaceStateTypeToString(RaceStateType Type);

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

		class Race
		{
			void Finished(Racer*);
		public:
			Race();

			bool HasFinished(), HasFinished(Racer*);
			unsigned int FinishedCount() { return Result.RacerResults.size(); }
			unsigned int CurrentWinnerDistance();
			Racer* Get(std::string RacerName);
			bool Contains(std::string RacerName);

			RaceStateType Tick(unsigned int Ms, RaceStateType Type);
			RaceResult Sim();

			unsigned int CurrentTick;
			Track* ThisTrack;
			std::vector<Racer*> Racers;
			RaceResult Result;
		};

		struct RaceStateData
		{
			RaceStateData(Race);
			unsigned int CurrentTick;
			
			Race ThisRace;
		};

		class RaceState
		{
		protected:
			RaceStateData& Data;
		public:
			RaceState(RaceStateData& _Data);
			virtual RaceState* Tick(unsigned int TickTimeMs);

			virtual void Entry(), Exit();

			RaceStateType Type;
		};

		class RaceState_PreRace : public RaceState
		{
			unsigned int TickAccumulator = 0;
		public:
			RaceState_PreRace(RaceStateData& _Data);

			RaceState* Tick(unsigned int TickTimeMs);
		};

		class RaceState_StartersOrders : public RaceState
		{
			unsigned int TickAccumulator = 0;
		public:
			RaceState_StartersOrders(RaceStateData& _Data);
			RaceState* Tick(unsigned int TickTimeMs);
		};

		class RaceState_Racing : public RaceState
		{
		public:
			RaceState_Racing(RaceStateData& _Data);
			RaceState* Tick(unsigned int TickTimeMs);
		};

		class RaceState_Finishing : public RaceState
		{
		public:
			RaceState_Finishing(RaceStateData& _Data);
			RaceState* Tick(unsigned int TickTimeMs);
		};

		class RaceState_Finished : public RaceState
		{
		public:
			RaceState_Finished(RaceStateData& _Data);
			RaceState* Tick(unsigned int TickTimeMs);
		};

		class RaceStateMachine
		{
		public:
			RaceStateData Data;
			RaceStateMachine(Race);

			void Tick(unsigned int TickTimeMs);

			void Push(RaceState*);

			RaceState* State;
		};
	}

	namespace States
	{
		class InRaceState : public AppState
		{
			unsigned int LastFrameEnd;
		public:
			InRaceState(AppStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Race RaceToRun);
			~InRaceState();

			AppState* Update();
			Race::RaceStateMachine RaceSM;
		};
	}
}
#endif