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

	namespace States
	{
		enum class AppStateType
		{
			Null, MainMenu, InRace
		};
		
		std::string AppStateTypeToString(AppStateType Type);
		
		class AppStateMachine;

		class AppState
		{
		protected:
			AppStateMachine& Machine;
			App::AppIO& IO;
			App::AppData& Data;

		public:
			AppState(AppStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data);
			~AppState();

			virtual void Entry(), Exit();

			virtual AppState* Update();

			AppStateType Type;
		};

		class AppStateMachine
		{
			std::stack<AppState*> StateStack;
		public:
			AppStateMachine();
			AppStateMachine(AppState*);

			void Update();

			void Push(AppState*), Pop(), SwapTop(AppState*);
			AppState* Top();
		};
	}
}
#endif