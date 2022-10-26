#ifndef MAINMENU_H
#define MAINMENU_H
#include "State.h"
#include "App.h"
#include "SDL2/SDL_ttf.h"
#include "League.h"

namespace Game
{
	namespace MainMenu
	{
		class MenuButton
		{
			bool IsButtonHoveredOver = false;
		public:
			App::AppIO& IO;

			std::string Text;
			int x, y, w, h;
			SDL_Color Color;
			SDL_Rect Rect;

			MenuButton(App::AppIO& _IO, std::string _Text, int x, int  y, int w, int h);

			void Update();

			bool IsMouseOver(), HasMouseClicked();
		};
	}

	namespace States
	{
		class MainMenuState : public AppState
		{

		public:
			MainMenu::MenuButton RaceButton, ExitButton;

			MainMenuState(AppStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data);
			~MainMenuState();

			AppState* Update();
		};

	}
}
#endif