#ifndef STATE_H
#define STATE_H
#include <stack>
#include <string>
namespace Game
{
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
			Null, MainMenu, RaceScreen, RacerScreen, CareerHub, RankingScreen
		};
		
		std::string AppStateTypeToString(AppScreenType Type);
		
		class AppScreenStateMachine;

		class AppScreen
		{
		protected:
			AppScreenStateMachine& ParentMachine;
			App::AppIO& IO;
			App::AppData& Data;

		public:
			AppScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data);
			~AppScreen();

			virtual void Entry(), Exit();

			virtual AppScreen* Update();

			AppScreenType Type;
		};

		class AppScreenStateMachine
		{
			std::stack<AppScreen*> StateStack; 
			
		public:
			AppScreenStateMachine();
			AppScreenStateMachine(AppScreen*);

			void Update();

			void Push(AppScreen*), Pop(), SwapTop(AppScreen*);
			AppScreen* Top();
		};
	}
}
#endif