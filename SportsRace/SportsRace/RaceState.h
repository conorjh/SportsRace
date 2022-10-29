#ifndef RACESTATE_H
#define RACESTATE_H
#include "Race.h"

namespace Game
{
	namespace Race
	{

		enum class RaceStateType
		{
			PreRace, StartersOrders, Racing, Finishing, Finished
		};

		std::string RaceStateTypeToString(RaceStateType Type);

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

			RaceStateType Tick(unsigned int TickTimeMs);

			void Push(RaceState*);

			RaceState* State;
		};
	}

	namespace States
	{
		enum class InRaceStateInitType
		{
			Race, Training
		};

		class InRaceState : public AppState
		{
			unsigned int LastFrameEnd;
		public:
			InRaceState(AppStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* TrainingRacer);	//InRaceStateInitType::Training
			InRaceState(AppStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Race RaceToRun);			//InRaceStateInitType::Race
			~InRaceState();

			void Entry(), Exit();

			AppState* Update();
			Race::RaceStateMachine RaceSM;
		};
	}
}
#endif