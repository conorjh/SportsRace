#include "App.h"
#include "Util.h"
#include "Screens.h"
#include "MainMenu.h"
#include "Race.h"
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

Game::App::CommandLineSettings::CommandLineSettings()
{
	Filepath = "";
	ConfigPath = "config.cfg";
	LogLevel = 0;
}


CommandLineSettings ParseCommandLine(CommandLine CmdLine)
{
	CommandLineSettings Output;

	for (auto& Argument : CmdLine.Arguments)
		if (Argument.Type == ApplicationPath)
		{
			Output.Filepath = Argument.Data;
		}

		else if (Argument.Data == "-cfg")
		{
			if (Argument.Parameters.size() != 1)
			{
				//error - no parameters
				Output.Errors.AddError("-cfg: No Filepath specified");
				continue;
			}

			Output.ConfigPath = Argument.Parameters[0];
		}

		else if (Argument.Data == "-log")
		{
			if (Argument.Parameters.size() != 1)
			{
				//error - no parameters
				Output.Errors.AddError("-log: no log level specified (should be a number, 0 - 6)");
				continue;
			}

			auto IntBuff = stoi(Argument.Parameters[0]);
			if (IntBuff < 0 || IntBuff > 6)
			{
				//invalid log level bro
				Output.Errors.AddError("-log: Invalid log level, should be a number 0 to 6, got: " + Argument.Parameters[0]);
				continue;
			}

			Output.LogLevel = IntBuff;
		}

		else
		{
			//unknown parameter - error
			Output.Errors.AddError("Invalid argument: " + Argument.Data);
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
	: CmdLine(argc, argv), IO(Data)
{
	spdlog::set_level(spdlog::level::level_enum::trace);
	srand(time(0));

	spdlog::info("Application launch");
	spdlog::trace("Running dir " + std::filesystem::current_path().string());

	spdlog::debug("Parsing command line");
	auto Settings = ParseCommandLine(CmdLine);
	if(Settings.Errors.HasErrored())
		spdlog::debug("Parsing command line failed");
	
	/*
	* Config parser - TODO
	spdlog::debug("Opening config " + Settings.ConfigPath);
	ConfigParser CfgParser(Settings.ConfigPath);
	if (CfgParser.Errors.HasErrored())
	{
		//error with config file, use exist defaults
		spdlog::warn("Errors in config @ " + Settings.ConfigPath);
	}
	*/
	Data.UpdateFromConfig(Cfg);
}

bool Game::App::Application::Ended() const
{
	return Data.Halted;
}

bool SDLInit(AppData& Data, Config& Cfg)
{
	spdlog::info("SDL Init");

	spdlog::debug("SDL Init - Video");
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		spdlog::critical("SDL init error: {}", SDL_GetError());
		return false;
	}

	//graphics - main window
	spdlog::debug("SDL Init - Creating main window {}, {}", Cfg.ScreenWidth, Cfg.ScreenHeight);
	if (!SDL_CreateWindowAndRenderer("SportsRace", Cfg.ScreenWidth, Cfg.ScreenHeight, NULL, &Data.RenderData.MainWindow, &Data.RenderData.MainRenderer))
	{
		spdlog::critical("SDL init error, SDL_CreateWindowAndRenderer(): {}", SDL_GetError());
		return false;
	}
	Data.RenderData.MainSurface = SDL_CreateSurface(Cfg.ScreenWidth, Cfg.ScreenHeight, SDL_GetWindowPixelFormat(Data.RenderData.MainWindow));
	
	//Initialize renderer color
	SDL_SetRenderDrawColor(Data.RenderData.MainRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

	//Initialize PNG loading
	int imgFlags = IMG_INIT_PNG;
	spdlog::debug("SDL Init - Starting IMG loader ");
	if (!(IMG_Init(imgFlags) & imgFlags))
	{
		spdlog::critical("SDL init error IMG_Init(): {}", SDL_GetError());
		return false;
	}

	//init TTF
	spdlog::debug("SDL Init - Init TTF");
	if (!TTF_Init())
	{
		spdlog::critical("SDL init error TTF_Init(): {}", SDL_GetError());
		return false;
	}

	//init audio
	spdlog::debug("SDL Init - Starting Audio {}hz, {}ch, ({})chunksize", 44100, 2, 2048);
	if (!SDL_Init(SDL_INIT_AUDIO))	
		spdlog::warn("SDL Audio init error: {}", SDL_GetError());

	if (!Mix_OpenAudio(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL))
		spdlog::warn("SDL Init - Warning, could not start default audio device: {}", SDL_GetError());

	spdlog::debug("SDL Init complete");
	return true;
}

bool SDLClose(AppData& Data)
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
	IMG_Quit();
	SDL_Quit();

	return true;
}

bool Game::App::Application::Init()
{
	spdlog::info("Application init");

	//Init SDL
	if (!SDLInit(Data, Cfg))
		return false;

	srand(time(0));

	spdlog::debug("IO Init");

	if (!IO.Init())
		return false;

	spdlog::debug("Starting AppStateMachine: Pushing MainMenuScreen");
	ScreenStack.Push(new MainMenuScreen(ScreenStack, IO, Data));

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
