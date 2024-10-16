#ifndef APP_H
#define APP_H
#include "SDL2/SDL.h"
#include "Audio.h"
#include "CommandLine.h"
#include "Util.h"
#include "State.h"
#include "League.h"

namespace Game
{
	namespace App
	{
		struct Config
		{
			Config() {}

			unsigned int ScreenWidth = 1024, ScreenHeight = 768, ScreenDepth = 32;	
			bool ShowFPS = true;
		};

		class ConfigParser
		{
			std::string Filepath;
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
			void UpdateFromConfig(Config Cfg);

			AppRenderData RenderData;
			unsigned int ScreenWidth = 1024, ScreenHeight = 768;
			bool ShowFPS = false;
			bool Halted = false;

			League::League League;
		};

		class AppIO
		{
			AppData& Data;

		public:
			AppIO(AppData& _Data);

			bool Init();

			void Update();

			Audio::AudioPlayer Player;
			SDL_Point MousePosition;
			Uint32 MouseButtons;
		};

		class Application
		{
		public:
			CommandLine CmdLine;
			Config Cfg;
			AppIO IO;
			AppData Data;
			States::AppStateMachine StateMachine;

			Application(int argc, char* argv[]);

			bool Ended() const;

			bool Init();
			void Update();
		};
		
		
	}
}
#endif