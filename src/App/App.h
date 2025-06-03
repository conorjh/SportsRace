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
		struct Config
		{
			Config() {}

			unsigned int ScreenWidth = 1024, ScreenHeight = 768, ScreenDepth = 32;	
			bool ShowFPS = true;

			unsigned int Ver = 2;
			std::string VerString = "0.0.0.2";
		};

		class ConfigParser
		{
			std::string ConfigString;
		public:
			ConfigParser(std::string _Filepath);

			Config Parse();

			ErrorData Errors;
		};

		struct ParsedCommandLineArguments
		{
			ParsedCommandLineArguments()
			{
				Filepath = "";
				ConfigPath = "config.cfg";
				LogLevel = 6;
			};

			//settings
			std::string Filepath, ConfigPath;
			unsigned int LogLevel;

			//errors
			ErrorData Errors;
		};

		struct AppData
		{
			AppData();

			void UpdateFromConfig(Config Cfg);

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
			bool SDLInit(), SDLClose();
		public:
			Config Configuration;
			AppIO IO;
			AppData Data;
			int ReturnCode;	

			Application(int argc, char* argv[]);

			bool Init();
			void Update();

			bool Ended() const;
			void Halt(int ErrorCode);
		};
		
		
	}
}
#endif