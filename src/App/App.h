#ifndef APP_H
#define APP_H
#include "SDL3/SDL.h"
#include "..\Audio.h"
#include "CommandLine.hpp"
#include "..\Util.h"
#include "..\Screens.h"
#include "..\Career.h"
#include <cstddef>

namespace Game
{
	namespace Render
	{
		struct AppRenderContext;
	}

	namespace App
	{
		inline constexpr std::size_t DebugMenuSpawnRacerCount = 5;
		inline constexpr SDL_Rect DebugMenuSpawnRacersButtonBounds{ 312, 352, 400, 72 };

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

		// Shared runtime state used by the active screen, race, and renderer.
		struct AppData
		{
			AppData();

			unsigned int ScreenWidth = 1024, ScreenHeight = 768;
			unsigned int WindowedWidth = 1024, WindowedHeight = 768;
			bool Fullscreen = false;
			bool ShowFPS = false, Halted = false;
			bool DebugMenuOpen = false;
			std::string DebugMenuStatus;

			Screens::AppScreenStateMachine ScreenStack;
			Race::RaceResult RaceStateOutput;
			Career::CareerData* Career;
			Career::CareerProfile* Profile;
			Game::Render::AppRenderContext* RenderContext;

			bool SetWindowedResolution(unsigned int Width, unsigned int Height);
			bool SetFullscreen(bool Enabled);
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
			Uint32 MouseButtonsPressed = 0;
			float MouseWheelY = 0.0f;
			bool DebugMenuTogglePressed = false;
			bool DebugSpawnRacersPressed = false;
		};

		class Application
		{
			bool SDLInit(Game::Render::AppRenderContext* RenderContext, unsigned int ScreenWidth, unsigned int ScreenHeight);
			bool SDLClose(Game::Render::AppRenderContext* RenderContext);
			Uint64 LastCareerSimulationUpdateMs = 0;

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