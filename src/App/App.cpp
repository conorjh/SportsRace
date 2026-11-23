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
#include <algorithm>
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

	// Translate parser tokens into application settings and collect invalid options before startup.
	for (auto& Argument : CommandLineParser().Parse(argc, argv))
		if (Argument.Type == ApplicationPath)
			Output.Filepath = Argument.Data;
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
			//unknown parameter - error
			Output.Errors.Errors.push_back("Invalid argument: " + Argument.Data);

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

		// Stop before initializing SDL so invalid startup options cannot leave a partial application running.
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

	if (!SDLInit(Data.RenderContext, Data.WindowedWidth, Data.WindowedHeight))
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
	LastCareerSimulationUpdateMs = SDL_GetTicks();

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
	SDL_WindowFlags WindowFlags = SDL_WINDOW_RESIZABLE;
	if (Data.Fullscreen)
		WindowFlags |= SDL_WINDOW_FULLSCREEN;
	if (!SDL_CreateWindowAndRenderer("SportsRace", static_cast<int>(ScreenWidth), static_cast<int>(ScreenHeight),
		WindowFlags, &RenderContext->MainWindow, &RenderContext->MainRenderer))
	{
		spdlog::critical("SDL init error, SDL_CreateWindowAndRenderer(): {}", SDL_GetError());
		return false;
	}
	RenderContext->ScreenWidth = 1024;
	RenderContext->ScreenHeight = 768;
	if (!SDL_SetRenderLogicalPresentation(RenderContext->MainRenderer, RenderContext->ScreenWidth,
		RenderContext->ScreenHeight, SDL_LOGICAL_PRESENTATION_LETTERBOX))
	{
		spdlog::critical("SDL init error, SDL_SetRenderLogicalPresentation(): {}", SDL_GetError());
		return false;
	}
	RenderContext->MainSurface = SDL_CreateSurface(RenderContext->ScreenWidth, RenderContext->ScreenHeight,
		SDL_GetWindowPixelFormat(RenderContext->MainWindow));
	SDL_SetRenderDrawColor(RenderContext->MainRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

	spdlog::debug("SDL Init - Init TTF");
	if (!TTF_Init())
	{
		spdlog::critical("SDL init error TTF_Init(): {}", SDL_GetError());
		return false;
	}

	spdlog::debug("SDL Init - Starting Audio {}hz, {}ch, ({})chunksize", 44100, 2, 2048);
	// Audio is optional; keep the game usable when the device or mixer cannot be opened.
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
	if (IO.DebugMenuTogglePressed)
	{
		Data.DebugMenuOpen = !Data.DebugMenuOpen;
		if (Data.DebugMenuOpen)
		{
			if (Data.Career != nullptr && Data.Career->WorldSimulationInitialized)
			{
				Career::CareerOrchestrator Orchestrator(Data.Career, Data.Profile);
				Data.DebugMenuStatus = "Active AI racers: " + std::to_string(Orchestrator.GetActiveRacerCount()) +
					". Press 1 or click to spawn racers.";
			}
			else
				Data.DebugMenuStatus = "Start a career before spawning world racers.";
		}
		else
			Data.DebugMenuStatus.clear();
	}
	if (Data.DebugMenuOpen && IO.DebugSpawnRacersPressed)
	{
		Career::CareerOrchestrator Orchestrator(Data.Career, Data.Profile);
		const std::size_t SpawnedCount = Orchestrator.SpawnRacers(DebugMenuSpawnRacerCount);
		Data.DebugMenuStatus = SpawnedCount == 0
			? "No racers spawned. Start a career first."
			: "Spawned " + std::to_string(SpawnedCount) + " AI racers; active: " +
			std::to_string(Orchestrator.GetActiveRacerCount()) + ".";
	}
	if (Data.DebugMenuOpen && IO.DebugNodeViewerTogglePressed)
	{
		Data.NodeViewerEnabled = !Data.NodeViewerEnabled;
		Data.DebugMenuStatus = std::string("Node viewer ") +
			(Data.NodeViewerEnabled ? "enabled." : "disabled.");
	}

	const Uint64 Now = SDL_GetTicks();
	if (Data.Career != nullptr && (Data.Career->InitialWorldMonthSimulationInProgress ||
		Data.Career->InitialWorldMonthSimulationFailed))
		LastCareerSimulationUpdateMs = Now;
	else if (Data.Career != nullptr && Data.Career->InitialWorldMonthClockResetPending)
	{
		Data.Career->InitialWorldMonthClockResetPending = false;
		LastCareerSimulationUpdateMs = Now;
	}
	else if (Data.Career != nullptr && Data.Career->WorldSimulationInitialized &&
		LastCareerSimulationUpdateMs != 0 && Now > LastCareerSimulationUpdateMs)
	{
		Career::CareerOrchestrator Orchestrator(Data.Career, Data.Profile);
		Orchestrator.Update(Now - LastCareerSimulationUpdateMs);
	}
	LastCareerSimulationUpdateMs = Now;

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
	MouseButtonsPressed = 0;
	MouseWheelY = 0.0f;
	DebugMenuTogglePressed = false;
	DebugSpawnRacersPressed = false;
	DebugNodeViewerTogglePressed = false;
	ShiftClickPressed = false;

	// Drain queued input events for this frame, then refresh the pointer position for screen updates.
	SDL_Event Event;
	while (SDL_PollEvent(&Event))
		switch (Event.type)
		{
		case SDL_EVENT_KEY_DOWN:
			if (Event.key.scancode == SDL_SCANCODE_ESCAPE)
				Esc = true;
			if (!Event.key.repeat && Event.key.scancode == SDL_SCANCODE_F3)
				DebugMenuTogglePressed = true;
			if (Data.DebugMenuOpen && !Event.key.repeat && Event.key.scancode == SDL_SCANCODE_1)
				DebugSpawnRacersPressed = true;
			break;

		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			MouseButtonsPressed |= SDL_BUTTON_MASK(Event.button.button);
			break;

		case SDL_EVENT_MOUSE_WHEEL:
			MouseWheelY += Event.wheel.y *
				(Event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0f : 1.0f);
			break;

		case SDL_EVENT_WINDOW_RESIZED:
			if (!Data.Fullscreen && Data.RenderContext != nullptr && Data.RenderContext->MainWindow != nullptr &&
				Event.window.windowID == SDL_GetWindowID(Data.RenderContext->MainWindow))
			{
				Data.WindowedWidth = static_cast<unsigned int>(std::max(Event.window.data1, 1));
				Data.WindowedHeight = static_cast<unsigned int>(std::max(Event.window.data2, 1));
			}
			break;

		case SDL_EVENT_QUIT:
			Data.Halted = true;
			break;
		}

	float WindowMouseX = 0.0f;
	float WindowMouseY = 0.0f;
	MouseButtons = SDL_GetMouseState(&WindowMouseX, &WindowMouseY);
	if (Data.RenderContext != nullptr && Data.RenderContext->MainRenderer != nullptr)
	{
		if (!SDL_RenderCoordinatesFromWindow(Data.RenderContext->MainRenderer, WindowMouseX, WindowMouseY,
			&MousePosition.x, &MousePosition.y))
		{
			spdlog::warn("Could not convert mouse position to logical render coordinates: {}", SDL_GetError());
			MousePosition = { -1.0f, -1.0f };
		}
	}
	else
		MousePosition = { WindowMouseX, WindowMouseY };

	if (Data.DebugMenuOpen && (MouseButtonsPressed & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) != 0)
	{
		const SDL_Point MousePoint{ static_cast<int>(MousePosition.x), static_cast<int>(MousePosition.y) };
		if (SDL_PointInRect(&MousePoint, &DebugMenuSpawnRacersButtonBounds))
			DebugSpawnRacersPressed = true;
		else if (SDL_PointInRect(&MousePoint, &DebugMenuNodeViewerButtonBounds))
			DebugNodeViewerTogglePressed = true;
	}
	if (Data.DebugMenuOpen || DebugMenuTogglePressed)
	{
		Esc = false;
		MouseButtons = 0;
		MouseButtonsPressed = 0;
		MouseWheelY = 0.0f;
	}
	else
		ShiftClickPressed = (MouseButtonsPressed & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) != 0 &&
			(SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
}

Game::App::AppData::AppData()
	: ScreenStack(),
	RaceStateOutput(),
	Career(new Game::Career::CareerData()),
	Profile(new Game::Career::CareerProfile()),
	RenderContext(new Game::Render::AppRenderContext())
{

}

bool Game::App::AppData::IsPlayerOwnedRacer(const Race::Racer* Runner) const
{
	if (Runner == nullptr)
		return false;
	if (Profile != nullptr && Runner == &Profile->MainFella)
		return true;
	return Career != nullptr && (Runner == Career->PlayerLeagueRacer ||
		std::find(Career->OwnedRacers.begin(), Career->OwnedRacers.end(), Runner) != Career->OwnedRacers.end());
}

bool Game::App::AppData::SetWindowedResolution(unsigned int Width, unsigned int Height)
{
	if (Width == 0 || Height == 0)
	{
		spdlog::error("Cannot set an invalid window resolution: {}x{}", Width, Height);
		return false;
	}
	if (Fullscreen)
	{
		WindowedWidth = Width;
		WindowedHeight = Height;
		return true;
	}
	if (RenderContext == nullptr || RenderContext->MainWindow == nullptr)
	{
		spdlog::error("Cannot set the window resolution before SDL has created the main window.");
		return false;
	}
	if (!SDL_SetWindowSize(RenderContext->MainWindow, static_cast<int>(Width), static_cast<int>(Height)))
	{
		spdlog::error("Could not set the window resolution to {}x{}: {}", Width, Height, SDL_GetError());
		return false;
	}
	WindowedWidth = Width;
	WindowedHeight = Height;
	return true;
}

bool Game::App::AppData::SetFullscreen(bool Enabled)
{
	if (Fullscreen == Enabled)
		return true;
	if (RenderContext == nullptr || RenderContext->MainWindow == nullptr)
	{
		spdlog::error("Cannot change fullscreen mode before SDL has created the main window.");
		return false;
	}
	if (Enabled)
	{
		int CurrentWidth = 0;
		int CurrentHeight = 0;
		if (SDL_GetWindowSize(RenderContext->MainWindow, &CurrentWidth, &CurrentHeight))
		{
			WindowedWidth = static_cast<unsigned int>(std::max(CurrentWidth, 1));
			WindowedHeight = static_cast<unsigned int>(std::max(CurrentHeight, 1));
		}
		if (!SDL_SetWindowFullscreen(RenderContext->MainWindow, true))
		{
			spdlog::error("Could not enable fullscreen mode: {}", SDL_GetError());
			return false;
		}
		Fullscreen = true;
		return true;
	}

	if (!SDL_SetWindowFullscreen(RenderContext->MainWindow, false))
	{
		spdlog::error("Could not leave fullscreen mode: {}", SDL_GetError());
		return false;
	}
	Fullscreen = false;
	if (!SDL_SetWindowSize(RenderContext->MainWindow, static_cast<int>(WindowedWidth),
		static_cast<int>(WindowedHeight)))
	{
		spdlog::error("Could not restore the windowed resolution to {}x{}: {}",
			WindowedWidth, WindowedHeight, SDL_GetError());
		return false;
	}
	return true;
}
