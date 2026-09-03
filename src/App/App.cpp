#include "App.h"
#include "..\Util.h"
#include "..\Screens.h"
#include "..\Screens\MainMenu.h"
#include "..\Race.h"
#include "SDL3/SDL.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "SDL3_image/SDL_image.h"
#include "SDL3_mixer/SDL_mixer.h"
#include "spdlog/spdlog.h"
#include <filesystem>

using namespace std;
using namespace Game;
using namespace Game::App;
using namespace Game::Screens;
using namespace Game::Util;
using enum CommandLineArgumentType;

std::string CommandLineToString(int argc, char* argv[])
{
	std::string FullCommandLine;
	for (int i = 0; i < argc; i++)
		FullCommandLine += argv[i];
	return FullCommandLine;
}

ParsedCommandLineArguments ParseCommandLine(int argc, char* argv[])
{
	ParsedCommandLineArguments Output;

	for (auto& Argument : CommandLineParser().Parse(argc, argv))
		if (Argument.Type == ApplicationPath)
		{
			Output.Filepath = Argument.Data;
		}
		else if (Argument.Data == "cfg")
		{
			if (Argument.Parameters.size() != 1)
			{
				//error - no parameters
				Output.Errors.Errors.push_back("-cfg: No Filepath specified");
				continue;
			}

		}
		else if (Argument.Data == "log")
		{
			if (Argument.Parameters.size() != 1)
			{
				//error - no parameters
				Output.Errors.Errors.push_back("-log: no log level specified (should be a number, 0 - 6)");
				continue;
			}

			auto IntBuff = stoi(Argument.Parameters[0]);
			if (IntBuff < 0 || IntBuff > 6)
			{
				//invalid log level bro
				Output.Errors.Errors.push_back("-log: Invalid log level, should be a number 0 to 6, got: " + Argument.Parameters[0]);
				continue;
			}

			Output.LogLevel = IntBuff;
		}
		else
		{
			//unknown parameter - error
			Output.Errors.Errors.push_back("Invalid argument: " + Argument.Data);
		}

	return Output;
}

Game::App::Application::Application(int argc, char* argv[]) 
	: IO(Data)
{
	spdlog::set_level(spdlog::level::level_enum::trace);
	spdlog::trace("Application launch");

	//command line
	spdlog::trace("Parsing command line: " + CommandLineToString(argc,argv));
	ParsedCommandLineArguments Arguments = ParseCommandLine(argc, argv);
	if (Arguments.Errors.HasErrored())
	{
		for (auto& Error : Arguments.Errors.Errors)
			spdlog::critical(Error);

		//TODO: error code = command line error
		Halt(Arguments.Errors.ErrorCode);
		return;
	}

	//use command line value or default
	spdlog::set_level((spdlog::level::level_enum)Arguments.LogLevel);
	spdlog::set_level(spdlog::level::level_enum::trace);

	//set the rng seed
	srand(time(0));

	//splash info
	spdlog::debug(" - - - Settings - - - ");
	spdlog::debug("Filepath: " + Arguments.Filepath);
	spdlog::debug("Log level: " + Arguments.LogLevel);
	spdlog::debug(" - - - - - - - - - - -");
	spdlog::debug("Queried running dir " + std::filesystem::current_path().string());
}

bool Game::App::Application::Init()
{
	spdlog::trace("Application init");

	if (!SDLInit(Data.RenderContext, 1024, 768))
	{
		Halt(AppErrorCode::SDLInitFailed);
		return false;	//critical error, cannot continue
	}

	if (!IO.Init())
	{
		Halt(AppErrorCode::SDLInitFailed);
		return false;	//critical error, cannot continue
	}

	Data.ScreenStack.Push(new MainMenuScreen(Data.ScreenStack, IO, Data));

	spdlog::trace("Application init complete");
	return true;
}
bool Game::App::Application::Ended() const
{
	return Data.Halted;
}

void Game::App::Application::Halt(AppErrorCode _ReturnCode)
{
	ReturnCode = _ReturnCode;
	Data.Halted = true;
}

bool Game::App::Application::SDLInit(Game::Render::AppRenderContext* RenderContext, unsigned int ScreenWidth, unsigned int ScreenHeight)
{
	spdlog::debug("SDL Init - Video");
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		spdlog::critical("SDL init error: {}", SDL_GetError());
		return false;
	}

	spdlog::debug("SDL Init - Creating main window {}, {}", ScreenWidth, ScreenHeight);
	if (!SDL_CreateWindowAndRenderer("SportsRace", ScreenWidth, ScreenHeight, NULL, &RenderContext->MainWindow, &RenderContext->MainRenderer))
	{
		spdlog::critical("SDL init error, SDL_CreateWindowAndRenderer(): {}", SDL_GetError());
		return false;
	}
	RenderContext->MainSurface = SDL_CreateSurface(ScreenWidth, ScreenHeight, SDL_GetWindowPixelFormat(RenderContext->MainWindow));
	SDL_SetRenderDrawColor(RenderContext->MainRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

	spdlog::debug("SDL Init - Init TTF");
	if (!TTF_Init())
	{
		spdlog::critical("SDL init error TTF_Init(): {}", SDL_GetError());
		return false;
	}

	spdlog::debug("SDL Init - Starting Audio {}hz, {}ch, ({})chunksize", 44100, 2, 2048);
	if (!SDL_Init(SDL_INIT_AUDIO))	
		spdlog::warn("SDL Audio init error: {}", SDL_GetError());										//we can live without sound
	if (!Mix_OpenAudio(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL))
		spdlog::warn("SDL Init - Warning, could not start default audio device: {}", SDL_GetError());	//we can live without sound

	spdlog::debug("SDL Init complete");
	return true;
}

bool Game::App::Application::SDLClose(Game::Render::AppRenderContext* RenderContext)
{
	spdlog::debug("SDL Close");

	//Destroy window and renderer
	SDL_DestroyRenderer(RenderContext->MainRenderer);
	SDL_DestroyWindow(RenderContext->MainWindow);
	RenderContext->MainRenderer = nullptr;
	RenderContext->MainWindow = nullptr;

	//Quit SDL subsystems
	Mix_Quit();
	TTF_Quit();
	SDL_Quit();

	return true;
}


void Game::App::Application::Update()
{
	IO.Update();

	Data.ScreenStack.Update();
}

Game::App::AppIO::AppIO(AppData& _Data) 
	: Data(_Data)
{

}

bool Game::App::AppIO::Init()
{
	spdlog::debug("IO Init");

	if(!Player.Init())
	{
		spdlog::critical("AudioPlayer init error");
		return false;
	}

	spdlog::debug("IO Init complete");
	return true;
}

void Game::App::AppIO::Update()
{
	Esc = false;
	MouseButtons = 0;

	//handle input
	SDL_Event Event;
	while (SDL_PollEvent(&Event))
		switch (Event.type)
		{
		case SDL_EVENT_KEY_DOWN:
			// Handle any key presses here.
			if (Event.key.scancode == SDL_SCANCODE_ESCAPE)
				Esc = true;
			break;

		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			// Handle mouse clicks here.
			MouseButtons = Event.button.button;
			break;

		case SDL_EVENT_QUIT:
			Data.Halted = true;
			break;
		}

	//MouseButtons =
	SDL_GetMouseState(&MousePosition.x, &MousePosition.y);
}

Game::App::AppData::AppData()
	:	ScreenStack(),
	RaceStateOutput(),
	RenderContext(new Game::Render::AppRenderContext()),
	Profile(new Game::Career::CareerProfile()),
	Career(new Game::Career::CareerData())
{

}