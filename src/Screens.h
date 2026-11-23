#ifndef STATE_H
#define STATE_H
#include <cstdint>
#include <stack>
#include <string>
namespace Game
{
	namespace Race
	{
		class Racer;
	}

	namespace App
	{
		//fwd decs
		class AppIO;
		struct AppData;
	}

	namespace Screens
	{
		enum class AppScreenType
		{
			Null, MainMenu, RaceScreen, RacerScreen, CareerHub, WorldMapScreen, BookiesScreen, GymScreen,
			LicenseOfficeScreen, BreedingFarmScreen, RaceTrackScreen, StableScreen, RankingScreen,
			NodeViewScreen, PostRaceScreen, RacerDBScreen, OptionsScreen, SlotMachineScreen, RouletteScreen,
			LeagueBrowserScreen
		};
		
		std::string AppStateTypeToString(AppScreenType Type);
		
		class AppScreenStateMachine;

		class AppScreen
		{
		protected:
			AppScreenStateMachine& ScreenStack;
			App::AppIO& IO;
			App::AppData& Data;

		public:
			AppScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data);
			~AppScreen();

			virtual void Entry(), Exit();

			virtual AppScreen* Update();
			bool IsPlayerOwnedRacer(const Race::Racer* Runner) const;
			bool HandleRacerInfoClick();

			AppScreenType Type;
		};

		// Stacking a screen suspends the previous one; popping resumes it.
		class AppScreenStateMachine
		{
			std::stack<AppScreen*> ScreenStack; 
			
		public:
			AppScreenStateMachine();
			AppScreenStateMachine(AppScreen*);

			void Update();

			void Push(AppScreen*), Pop(), SwapTop(AppScreen*);
			AppScreen* Top();
		};

		bool OpenCareerHistoricalRace(AppScreenStateMachine& ScreenStack, App::AppIO& IO, App::AppData& Data,
			std::uint64_t HistoryId, bool OpenNodeView);
	}
}
#endif