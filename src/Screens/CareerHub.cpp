#include "CareerHub.h"
#include "..\Audio.h"
#include "..\RaceState.h"
#include "RaceScreen.h"
#include "RacerScreen.h"
#include "RankingScreen.h"
#include "RacerDBScreen.h"
#include "LeagueBrowserScreen.h"
#include "WorldMapScreen.h"
#include "SDL3\SDL.h"
#include "..\App\App.h"
#include <algorithm>
#include <cstdlib>
#include <random>
#include <utility>

namespace
{
	void DrawCareerHubButton(Game::Render::BaseRenderer& Renderer, Game::Render::AppRenderContext* Context,
		Game::GUI::Button& Button)
	{
		SDL_FRect Bounds;
		SDL_RectToFRect(&Button.Rect, &Bounds);
		const SDL_Color FillColor = Button.IsMouseOver() ? SDL_Color{ 134, 54, 45, 255 } :
			SDL_Color{ 104, 42, 36, 255 };
		SDL_SetRenderDrawColor(Context->MainRenderer, FillColor.r, FillColor.g, FillColor.b, FillColor.a);
		SDL_RenderFillRect(Context->MainRenderer, &Bounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, 170, 96, 72, 255);
		SDL_RenderRect(Context->MainRenderer, &Bounds);
		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), Button.Text,
			Button.x + 14, Button.y + 15, SDL_Color{ 239, 231, 214, 255 });
	}
}


Game::Screens::CareerHubScreen::CareerHubScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO,
	App::AppData& _Data, bool _PlayerCreationPending)
	: AppScreen(_Machine, _IO, _Data),
	Orchestrator(_Data.Career, _Data.Profile),
	PlayerCreationPending(_PlayerCreationPending),
	RacerIcon(_IO, GUI::IconButtonType::Racer, 100, 50, 200, 200),
	RaceIcon(_IO, GUI::IconButtonType::Race, 700, 450, 200, 200),
	TrainingIcon(_IO, GUI::IconButtonType::Training, 700, 50, 200, 200),
	RankingIcon(_IO, GUI::IconButtonType::Ranking, 100, 450, 200, 200),
	RacerDBButton(_IO, "Racer database", 520, 694, 200, 48),
	WorldMapButton(_IO, "World map", 304, 694, 200, 48),
	LeagueBrowserButton(_IO, "League browser", 736, 694, 200, 48)
{
	Type = AppScreenType::CareerHub;
	Orchestrator.Init();
	Orchestrator.BeginInitialWorldMonthSimulation();
}

Game::Screens::CareerHubScreen::~CareerHubScreen()
{

}

void Game::Screens::CareerHubScreen::Entry()
{
	IO.Player.Play(Audio::Soundtrack::Catering);
}

void Game::Screens::CareerHubScreen::Exit()
{
	IO.Player.StopMusic();
}


Game::Screens::AppScreen* Game::Screens::CareerHubScreen::Update()
{
	if (Data.Career != nullptr && Data.Career->InitialWorldMonthSimulationInProgress)
	{
		if (!Orchestrator.AdvanceInitialWorldMonthSimulation(256))
			return this;
	}
	if (Data.Career != nullptr && Data.Career->InitialWorldMonthSimulationFailed)
	{
		if (IO.Esc)
		{
			ScreenStack.Pop();
			return ScreenStack.Top();
		}
		return this;
	}
	if (PlayerCreationPending && Data.Career != nullptr && Data.Career->InitialWorldMonthSimulated)
	{
		PlayerCreationPending = false;
		ScreenStack.Push(new RacerScreen(ScreenStack, IO, Data, &Orchestrator.Profile->MainFella,
			RacerScreenStateInitType::RacerCreation));
		return ScreenStack.Top();
	}

	using namespace Game::Race;
	WorldMapButton.Update();
	if (WorldMapButton.HasMouseClicked())
	{
		ScreenStack.Push(new WorldMapScreen(ScreenStack, IO, Data));
		return ScreenStack.Top();
	}
	LeagueBrowserButton.Update();
	if (LeagueBrowserButton.HasMouseClicked())
	{
		ScreenStack.Push(new LeagueBrowserScreen(ScreenStack, IO, Data));
		return ScreenStack.Top();
	}

	RaceIcon.Update();
	if (RaceIcon.HasMouseClicked())
	{
		// Draw the player's opponents from the same roster used by the career-world simulations.
		Game::Race::Race RaceBuffer;
		Orchestrator.Profile->MainFella.Reset();
		RaceBuffer.Racers.push_back(&Orchestrator.Profile->MainFella);
		std::vector<Game::Race::Racer*> Opponents = Orchestrator.SelectMatchedOpponents(5, RaceBuffer.GetSeed());
		for (Game::Race::Racer* Opponent : Opponents)
		{
			Opponent->Reset();
			RaceBuffer.Racers.push_back(Opponent);
		}
		RaceBuffer.Financials = RaceFinancials(unsigned int(500 * (rand() % 10)), 50 * (rand() % 4));
		
		//actually run our race
		ScreenStack.Push(new RaceScreen(ScreenStack, IO, Data, std::move(RaceBuffer), Orchestrator.Profile->MainFella.GUID));
		return ScreenStack.Top();
	}

	RacerDBButton.Update();
	if (RacerDBButton.HasMouseClicked())
	{
		ScreenStack.Push(new RacerDBScreen(ScreenStack, IO, Data, Orchestrator.Data->Racers));
		return ScreenStack.Top();
	}

	TrainingIcon.Update();
	if (TrainingIcon.HasMouseClicked())
	{
		//actually run our race
		ScreenStack.Push(new RaceScreen(ScreenStack, IO, Data, &Orchestrator.Profile->MainFella));
		return ScreenStack.Top();
	}

	RacerIcon.Update();
	if (RacerIcon.HasMouseClicked())
	{
		//actually run our race
		ScreenStack.Push(new RacerScreen(ScreenStack, IO, Data, &Orchestrator.Profile->MainFella, RacerScreenStateInitType::ViewOnly));
		return ScreenStack.Top();
	}

	RankingIcon.Update();
	if (RankingIcon.HasMouseClicked())
	{
		//actually run our race
		ScreenStack.Push(new RankingScreen(ScreenStack, IO, Data, &Orchestrator.Profile->MainFella));
		return ScreenStack.Top();
	}

	if (IO.Esc)
	{
		IO.Player.Play(Audio::BuiltInSounds::Click);

		ScreenStack.Pop();
		return ScreenStack.Top();
	}	

	return this;
}

Game::Career::CareerData* Game::Screens::CareerHubScreen::GetCareerData() const
{
	return Orchestrator.Data;
}



Game::Render::CareerHubRenderer::CareerHubRenderer(Game::Render::AppRenderContext* Context, Screens::CareerHubScreen* _State) 
	: Screen(_State), 
	BaseRenderer(Context)
{
}

Game::Render::CareerHubRenderer::CareerHubRenderer(Game::Render::AppRenderContext* Context) 
	: Screen(), 
	BaseRenderer(Context)
{
}

unsigned int Game::Render::CareerHubRenderer::Render()
{
	//start timerv=
	auto StartTime = SDL_GetTicks();

	SDL_SetRenderDrawColor(Context->MainRenderer, 255, 255, 255, 255);
	SDL_RenderClear(Context->MainRenderer);

	if (Screen->RacerIcon.IsMouseOver())
	{
		SDL_Rect RenderQuad1 = Screen->RacerIcon.Rect;
		RenderQuad1.x += 7;
		RenderQuad1.y -= 7;
		RenderImage(Context->Store.GetImage(Assets::ImageAssetRacerIcon)->Texture, NULL, &RenderQuad1);
	}
	else
		RenderImage(Context->Store.GetImage(Assets::ImageAssetRacerIcon)->Texture, NULL, &Screen->RacerIcon.Rect);

	if (Screen->TrainingIcon.IsMouseOver())
	{
		SDL_Rect RenderQuad2 = Screen->TrainingIcon.Rect;
		RenderQuad2.x += 7;
		RenderQuad2.y -= 7;
		RenderImage(Context->Store.GetImage(Assets::ImageAssetTrainingIcon)->Texture, NULL, &RenderQuad2);
	}
	else
		RenderImage(Context->Store.GetImage(Assets::ImageAssetTrainingIcon)->Texture, NULL, &Screen->TrainingIcon.Rect);

	if (Screen->RaceIcon.IsMouseOver())
	{
		SDL_Rect RenderQuad3 = Screen->RaceIcon.Rect;
		RenderQuad3.x += 7;
		RenderQuad3.y -= 7;
		RenderImage(Context->Store.GetImage(Assets::ImageAssetRaceIcon)->Texture, NULL, &RenderQuad3);
	}
	else
		RenderImage(Context->Store.GetImage(Assets::ImageAssetRaceIcon)->Texture, NULL, &Screen->RaceIcon.Rect);

	if (Screen->RankingIcon.IsMouseOver())
	{
		SDL_Rect RenderQuad3 = Screen->RankingIcon.Rect;
		RenderQuad3.x += 7;
		RenderQuad3.y -= 7;
		RenderImage(Context->Store.GetImage(Assets::ImageAssetRankingIcon)->Texture, NULL, &RenderQuad3);
	}
	else
		RenderImage(Context->Store.GetImage(Assets::ImageAssetRankingIcon)->Texture, NULL, &Screen->RankingIcon.Rect);

	DrawCareerHubButton(*this, Context, Screen->WorldMapButton);
	DrawCareerHubButton(*this, Context, Screen->RacerDBButton);
	DrawCareerHubButton(*this, Context, Screen->LeagueBrowserButton);

	const Career::CareerData* CareerData = Screen->GetCareerData();
	if (CareerData != nullptr)
	{
		const std::uint64_t WorldMinutes = CareerData->WorldClock.NowMs() / 60000;
		const std::uint64_t Day = WorldMinutes / 1440 + 1;
		const unsigned int Hour = static_cast<unsigned int>((WorldMinutes / 60) % 24);
		const unsigned int Minute = static_cast<unsigned int>(WorldMinutes % 60);
		const std::string TimeText = (Hour < 10 ? "0" : "") + std::to_string(Hour) + ":" +
			(Minute < 10 ? "0" : "") + std::to_string(Minute);
		const std::size_t ActiveRacerCount = static_cast<std::size_t>(std::count_if(
			CareerData->RacerStates.begin(), CareerData->RacerStates.end(),
			[](const Career::CareerRacerState& State)
			{
				return State.Activity != Career::CareerRacerActivity::Retired;
			}));
		const std::size_t ActiveRaceCount = static_cast<std::size_t>(std::count_if(
			CareerData->WorldRaces.begin(), CareerData->WorldRaces.end(),
			[](const std::shared_ptr<Career::CareerWorldRace>& WorldRace)
			{
				return WorldRace != nullptr && !WorldRace->IsComplete;
			}));
		RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont),
			"World day " + std::to_string(Day) + "  " + TimeText + "    Active AI: " +
			std::to_string(ActiveRacerCount) + "    Live races: " + std::to_string(ActiveRaceCount),
			310, 630, SDL_Color{ 194, 179, 152, 255 });

		if (CareerData->InitialWorldMonthSimulationInProgress || CareerData->InitialWorldMonthSimulationFailed)
		{
			const SDL_FRect ProgressPanel{ 250.0f, 276.0f, 524.0f, 188.0f };
			SDL_SetRenderDrawColor(Context->MainRenderer, 27, 26, 24, 255);
			SDL_RenderFillRect(Context->MainRenderer, &ProgressPanel);
			SDL_SetRenderDrawColor(Context->MainRenderer, 170, 96, 72, 255);
			SDL_RenderRect(Context->MainRenderer, &ProgressPanel);
			if (CareerData->InitialWorldMonthSimulationFailed)
			{
				RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont),
					"Career warm-up failed. Check the log, then press Escape to return.", 286, 326,
					SDL_Color{ 239, 190, 133, 255 });
			}
			else
			{
				const double Progress = CareerData->InitialWorldMonthTotalRealMs == 0 ? 0.0 :
					std::clamp(static_cast<double>(CareerData->InitialWorldMonthElapsedRealMs) /
						static_cast<double>(CareerData->InitialWorldMonthTotalRealMs), 0.0, 1.0);
				const SDL_FRect ProgressTrack{ 286.0f, 352.0f, 452.0f, 20.0f };
				SDL_SetRenderDrawColor(Context->MainRenderer, 57, 55, 49, 255);
				SDL_RenderFillRect(Context->MainRenderer, &ProgressTrack);
				const SDL_FRect ProgressFill{ ProgressTrack.x, ProgressTrack.y,
					static_cast<float>(ProgressTrack.w * Progress), ProgressTrack.h };
				SDL_SetRenderDrawColor(Context->MainRenderer, 192, 73, 52, 255);
				SDL_RenderFillRect(Context->MainRenderer, &ProgressFill);
				RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont),
					"Simulating the first month of career life", 286, 310,
					SDL_Color{ 239, 231, 214, 255 });
				RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont),
					std::to_string(static_cast<unsigned int>(Progress * 100.0)) + "%  |  " +
					"AI races, league fixtures, and racer history", 286, 390,
					SDL_Color{ 194, 179, 152, 255 });
			}
		}
	}

	//TODO make financials accessible from here
	//RenderText(Context->Store.GetFont(Assets::FontAssetMainFont), "Cash: $" + to_string(Financials.Cash), 450,720, {0,0,0});

	Display();
	auto EndTime = SDL_GetTicks();
	return EndTime - StartTime;
}

