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
		enum class AppStateType
		{
			Null, MainMenu, RaceScreen, RacerScreen, CareerHub, RankingScreen
		};
		
		std::string AppStateTypeToString(AppStateType Type);
		
		class AppStateMachine;

		class AppScreen
		{
		protected:
			AppStateMachine& Machine;
			App::AppIO& IO;
			App::AppData& Data;

		public:
			AppScreen(AppStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data);
			~AppScreen();

			virtual void Entry(), Exit();

			virtual AppScreen* Update();

			AppStateType Type;
		};

		class AppStateMachine
		{
			std::stack<AppScreen*> StateStack; 
			
		public:
			AppStateMachine();
			AppStateMachine(AppScreen*);

			void Update();

			void Push(AppScreen*), Pop(), SwapTop(AppScreen*);
			AppScreen* Top();
		};
	}
}
#endif