#ifndef APP_H
#define APP_H
#include "SDL3/SDL.h"
#include "Audio.h"
#include "CommandLine.h"
#include "Util.h"
#include "Screens.h"
#include "Career.h"

namespace Game
{
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

		struct CommandLineSettings
		{
			CommandLineSettings();

			//settings
			std::string Filepath, ConfigPath;
			unsigned int LogLevel;

			//errors
			ErrorData Errors;
		};

		struct AppRenderData
		{
			SDL_Window* MainWindow = nullptr;
			SDL_Renderer* MainRenderer = nullptr;
			SDL_Surface* MainSurface = nullptr;
		};

		struct AppData
		{
			AppData();

			void UpdateFromConfig(Config Cfg);

			AppRenderData RenderData;

			unsigned int ScreenWidth = 1024, ScreenHeight = 768;
			bool ShowFPS = false;
			bool Halted = false;


			Race::RaceResult RaceStateOutput;

			Career::CareerData* Career;
			Career::CareerProfile* Profile;
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
		public:
			CommandLine CmdLine;
			Config Cfg;
			AppIO IO;
			AppData Data;
			Screens::AppScreenStateMachine ScreenStack;

			Application(int argc, char* argv[]);

			bool Ended() const;

			bool Init();
			void Update();
		};
		
		
	}
}
#endif