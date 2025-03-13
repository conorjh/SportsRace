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
		else if (Argument.Data == "-cfg")
		{
			if (Argument.Parameters.size() != 1)
			{
				//error - no parameters
				Output.Errors.Errors.push_back("-cfg: No Filepath specified");
				continue;
			}

			Output.ConfigPath = Argument.Parameters[0];
		}
		else if (Argument.Data == "-log")
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

Game::App::ConfigParser::ConfigParser(std::string _ConfigString)
{
	ConfigString = _ConfigString;
	
}

Config Game::App::ConfigParser::Parse()
{
	if(Errors.HasErrored())
		return Config();

	Config Output;
	
	return Output;
}

Game::App::Application::Application(int argc, char* argv[]) 
	: IO(Data)
{
	spdlog::set_level(spdlog::level::level_enum::trace);
	spdlog::trace("Application launch");

	//command line
	spdlog::trace("Parsing command line");
	spdlog::trace("CMD = " + CommandLineToString(argc,argv));
	ParsedCommandLineArguments Arguments = ParseCommandLine(argc, argv);
	if (Arguments.Errors.HasErrored())
	{
		for (auto& Error : Arguments.Errors.Errors)
			spdlog::critical(Error);

		//TODO: error code = command line error
		Halt(Arguments.Errors.GetErrorCode());
		return;
	}

	//use command line value or default
	spdlog::set_level((spdlog::level::level_enum)Arguments.LogLevel);

	//set the rng seed
	srand(time(0));

	//splash info
	spdlog::debug(" - - - Settings - - - ");
	spdlog::debug("Filepath: " + Arguments.Filepath);
	spdlog::debug("Log level: " + Arguments.LogLevel);
	spdlog::debug("Config file: " + Arguments.ConfigPath);
	spdlog::debug(" - - - - - - - - - - -");
	spdlog::debug("Queried running dir " + std::filesystem::current_path().string());
}

bool Game::App::Application::Init()
{
	spdlog::trace("Application init");

	//Init SDL
	if (!SDLInit())
		return false;

	spdlog::debug("IO Init");
	if (!IO.Init())
	{
		spdlog::critical("IO Init error");
		return false;
	}

	spdlog::trace("Starting AppStateMachine: Pushing MainMenuScreen");
	ScreenStack.Push(new MainMenuScreen(ScreenStack, IO, Data));

	return true;
}
bool Game::App::Application::Ended() const
{
	return Data.Halted;
}

void Game::App::Application::Halt(int _ReturnCode)
{
	ReturnCode = _ReturnCode;
	Data.Halted = true;
}

bool Game::App::Application::SDLInit()
{
	spdlog::debug("SDL Init - Video");
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		spdlog::critical("SDL init error: {}", SDL_GetError());
		return false;
	}

	spdlog::debug("SDL Init - Creating main window {}, {}", Configuration.ScreenWidth, Configuration.ScreenHeight);
	if (!SDL_CreateWindowAndRenderer("SportsRace", Configuration.ScreenWidth, Configuration.ScreenHeight, NULL, &Data.RenderData.MainWindow, &Data.RenderData.MainRenderer))
	{
		spdlog::critical("SDL init error, SDL_CreateWindowAndRenderer(): {}", SDL_GetError());
		return false;
	}
	Data.RenderData.MainSurface = SDL_CreateSurface(Configuration.ScreenWidth, Configuration.ScreenHeight, SDL_GetWindowPixelFormat(Data.RenderData.MainWindow));
	SDL_SetRenderDrawColor(Data.RenderData.MainRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

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

bool Game::App::Application::SDLClose()
{
	spdlog::debug("SDL Close");

	//Destroy window and renderer
	SDL_DestroyRenderer(Data.RenderData.MainRenderer);
	SDL_DestroyWindow(Data.RenderData.MainWindow);
	Data.RenderData.MainRenderer = nullptr;
	Data.RenderData.MainWindow = nullptr;

	//Quit SDL subsystems
	Mix_Quit();
	TTF_Quit();
	SDL_Quit();

	return true;
}


void Game::App::Application::Update()
{
	IO.Update();

	ScreenStack.Update();
}

Game::App::AppIO::AppIO(AppData& _Data) : Data(_Data)
{

}

bool Game::App::AppIO::Init()
{
	return Player.Init();
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
{
}

void Game::App::AppData::UpdateFromConfig(Config Cfg)
{
	ScreenWidth = Cfg.ScreenWidth; ScreenHeight = Cfg.ScreenHeight;
	ShowFPS = Cfg.ShowFPS;
}
