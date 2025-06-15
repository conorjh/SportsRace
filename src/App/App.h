#ifndef APP_H
#define APP_H
#include "SDL3/SDL.h"
#include "..\Audio.h"
#include "CommandLine.hpp"
#include "..\Util.h"
#include "..\Screens.h"
#include "..\Career.h"

namespace Game
{
	namespace Render
	{
		struct AppRenderContext;
	}

	namespace App
	{
		enum AppErrorCode
		{
			NoError = 0,
			InvalidCommandLineArguments = 1,
			SDLInitFailed = 2,
			IOInitFailed = 3,
			UnknownError = 123456
		};

		struct AppError
		{
			bool HasErrored() const 
			{ 
				return Errors.size(); 
			}

			AppErrorCode ErrorCode = AppErrorCode::NoError;
			std::vector<std::string> Errors;
		};

		struct ParsedCommandLineArguments
		{
			ParsedCommandLineArguments()
			{
				Filepath = "";
				LogLevel = 6;
			};

			//settings
			std::string Filepath;
			unsigned int LogLevel;

			//errors
			AppError Errors;
		};

		struct AppData
		{
			AppData();

			unsigned int ScreenWidth = 1024, ScreenHeight = 768;
			bool ShowFPS = false, Halted = false;

			Screens::AppScreenStateMachine ScreenStack;
			Race::RaceResult RaceStateOutput;
			Career::CareerData* Career;
			Career::CareerProfile* Profile;
			Game::Render::AppRenderContext* RenderContext;
		};

		class AppIO
		{
			AppData& Data;
		public:
			AppIO(AppData& _Data);

			bool Init();

			void Update();

			bool Esc = false;
			Audio::AudioPlayer Player;
			SDL_FPoint MousePosition;
			Uint32 MouseButtons;
		};

		class Application
		{
			bool SDLInit(Game::Render::AppRenderContext* RenderContext, unsigned int ScreenHeight, unsigned int ScreenWidth);
			bool SDLClose(Game::Render::AppRenderContext* RenderContext);

		public:
			AppIO IO;
			AppData Data;
			AppErrorCode ReturnCode;

			Application(int argc, char* argv[]);

			bool Init();
			void Update();

			bool Ended() const;
			void Halt(AppErrorCode ErrorCode);
		};
		
		
	}
}
#endif