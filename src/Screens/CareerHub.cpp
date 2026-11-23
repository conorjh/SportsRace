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
#include "spdlog/spdlog.h"
#include <algorithm>
#include <cstdlib>
#include <random>
#include <utility>

namespace CareerHubScreenDetail
{
	constexpr SDL_Color HubBackgroundColor{ 23, 22, 20, 255 };
	constexpr SDL_Color HubPanelColor{ 34, 33, 30, 255 };
	constexpr SDL_Color HubPanelBorderColor{ 75, 70, 60, 255 };
	constexpr SDL_Color HubTextColor{ 235, 229, 215, 255 };
	constexpr SDL_Color HubMutedColor{ 164, 155, 138, 255 };
	constexpr SDL_Color HubAccentColor{ 178, 61, 49, 255 };
	constexpr SDL_Color HubSelectedColor{ 74, 43, 35, 255 };
	constexpr SDL_Color HubButtonFillColor{ 104, 42, 36, 255 };
	constexpr SDL_Color HubButtonHoverColor{ 134, 54, 45, 255 };
	constexpr SDL_Color HubButtonBorderColor{ 170, 96, 72, 255 };

	void DrawCareerHubButton(Game::Render::BaseRenderer& Renderer, Game::Render::AppRenderContext* Context,
		Game::GUI::Button& Button)
	{
		SDL_FRect Bounds;
		SDL_RectToFRect(&Button.Rect, &Bounds);
		const SDL_Color FillColor = Button.IsMouseOver() ? HubButtonHoverColor : HubButtonFillColor;
		SDL_SetRenderDrawColor(Context->MainRenderer, FillColor.r, FillColor.g, FillColor.b, FillColor.a);
		SDL_RenderFillRect(Context->MainRenderer, &Bounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, HubButtonBorderColor.r, HubButtonBorderColor.g,
			HubButtonBorderColor.b, HubButtonBorderColor.a);
		SDL_RenderRect(Context->MainRenderer, &Bounds);
		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), Button.Text,
			Button.x + 14, Button.y + 15, HubTextColor);
	}

	void DrawCareerHubCard(Game::Render::BaseRenderer& Renderer, Game::Render::AppRenderContext* Context,
		Game::GUI::IconButton& Button, Game::Assets::ImageAssetId IconAsset, const std::string& Title,
		const std::string& Description)
	{
		const bool IsHovered = Button.IsMouseOver();
		const SDL_FRect Bounds{ static_cast<float>(Button.Rect.x), static_cast<float>(Button.Rect.y),
			static_cast<float>(Button.Rect.w), static_cast<float>(Button.Rect.h) };
		const SDL_Color FillColor = IsHovered ? HubSelectedColor : HubPanelColor;
		const SDL_Color BorderColor = IsHovered ? HubAccentColor : HubPanelBorderColor;
		SDL_SetRenderDrawColor(Context->MainRenderer, FillColor.r, FillColor.g, FillColor.b, FillColor.a);
		SDL_RenderFillRect(Context->MainRenderer, &Bounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, BorderColor.r, BorderColor.g, BorderColor.b,
			BorderColor.a);
		SDL_RenderRect(Context->MainRenderer, &Bounds);
		const SDL_FRect AccentBar{ Bounds.x, Bounds.y, 4.0f, Bounds.h };
		SDL_SetRenderDrawColor(Context->MainRenderer, HubAccentColor.r, HubAccentColor.g, HubAccentColor.b,
			HubAccentColor.a);
		SDL_RenderFillRect(Context->MainRenderer, &AccentBar);

		Game::Render::Image* Icon = Context->Store.GetImage(IconAsset);
		if (Icon != nullptr && Icon->Texture != nullptr)
		{
			SDL_Rect IconBounds{ Button.Rect.x + 24, Button.Rect.y + (Button.Rect.h - 128) / 2, 128, 128 };
			if (IsHovered)
			{
				IconBounds.x += 7;
				IconBounds.y -= 7;
			}
			Renderer.RenderImage(Icon->Texture, nullptr, &IconBounds);
		}

		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), Title,
			Button.Rect.x + 176, Button.Rect.y + 78, HubTextColor);
		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), Description,
			Button.Rect.x + 176, Button.Rect.y + 116, HubMutedColor);
	}
}
using namespace CareerHubScreenDetail;


Game::Screens::CareerHubScreen::CareerHubScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO,
	App::AppData& _Data, bool _PlayerCreationPending)
	: AppScreen(_Machine, _IO, _Data),
	Orchestrator(_Data.Career, _Data.Profile),
	PlayerCreationPending(_PlayerCreationPending),
	RacerIcon(_IO, GUI::IconButtonType::Racer, 24, 84, 472, 232),
	RaceIcon(_IO, GUI::IconButtonType::Race, 528, 84, 472, 232),
	TrainingIcon(_IO, GUI::IconButtonType::Training, 24, 340, 472, 232),
	RankingIcon(_IO, GUI::IconButtonType::Ranking, 528, 340, 472, 232),
	RacerDBButton(_IO, "Racer database", 520, 694, 200, 48),
	WorldMapButton(_IO, "World map", 24, 694, 200, 48),
	LeagueBrowserButton(_IO, "League browser", 272, 694, 200, 48),
	SaveCareerButton(_IO, "Save career", 768, 694, 232, 48)
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
	SaveCareerButton.Update();
	if (SaveCareerButton.HasMouseClicked())
	{
		std::string Error;
		if (Data.Career == nullptr || Data.Profile == nullptr)
		{
			StatusMessage = "Career data is unavailable; the career could not be saved.";
			spdlog::error("Career save failed: {}", StatusMessage);
			return this;
		}
		if (!Career::SaveCareer(*Data.Career, *Data.Profile, Career::CareerSaveFilePath, Error))
		{
			StatusMessage = Error;
			spdlog::error("Career save failed: {}", Error);
			return this;
		}
		StatusMessage = "Career saved (format version " +
			std::to_string(Career::CurrentCareerSaveFormatVersion) + "): " + Career::CareerSaveFilePath;
		spdlog::info("{}", StatusMessage);
		return this;
	}

	RaceIcon.Update();
	if (RaceIcon.HasMouseClicked())
	{
		if (!Orchestrator.PrepareUpcomingRace(SDL_GetTicks()))
		{
			StatusMessage = "Could not prepare a race from the available runners.";
			return this;
		}
		Game::Race::Race RaceBuffer = Orchestrator.CreateUpcomingRace();
		if (RaceBuffer.Racers.size() < 2)
		{
			StatusMessage = "The scheduled race needs at least two runners.";
			return this;
		}
		for (Game::Race::Racer* Runner : RaceBuffer.Racers)
			if (Runner != nullptr)
				Runner->Reset();
		RaceBuffer.Financials = RaceFinancials(unsigned int(500 * (rand() % 10)), 50 * (rand() % 4));
		StatusMessage.clear();
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
	const unsigned int StartTime = SDL_GetTicks();

	SDL_SetRenderDrawColor(Context->MainRenderer, HubBackgroundColor.r, HubBackgroundColor.g,
		HubBackgroundColor.b, HubBackgroundColor.a);
	SDL_RenderClear(Context->MainRenderer);

	RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "CAREER HUB", 32, 24, HubTextColor);
	RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), "YOUR RACING WORLD", 32, 52, HubMutedColor);
	DrawCareerHubCard(*this, Context, Screen->RacerIcon, Assets::ImageAssetRacerIcon,
		"YOUR RACER", "Review form, results, and history.");
	DrawCareerHubCard(*this, Context, Screen->RaceIcon, Assets::ImageAssetRaceIcon,
		"RACE", "Your owned racers enter automatically.");
	DrawCareerHubCard(*this, Context, Screen->TrainingIcon, Assets::ImageAssetTrainingIcon,
		"TRAINING", "Build skills and improve race readiness.");
	DrawCareerHubCard(*this, Context, Screen->RankingIcon, Assets::ImageAssetRankingIcon,
		"RANKINGS", "Compare racers across the world.");

	DrawCareerHubButton(*this, Context, Screen->WorldMapButton);
	DrawCareerHubButton(*this, Context, Screen->RacerDBButton);
	DrawCareerHubButton(*this, Context, Screen->LeagueBrowserButton);
	DrawCareerHubButton(*this, Context, Screen->SaveCareerButton);

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
		const SDL_FRect StatusBounds{ 24.0f, 592.0f, 976.0f, 78.0f };
		SDL_SetRenderDrawColor(Context->MainRenderer, HubPanelColor.r, HubPanelColor.g,
			HubPanelColor.b, HubPanelColor.a);
		SDL_RenderFillRect(Context->MainRenderer, &StatusBounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, HubPanelBorderColor.r, HubPanelBorderColor.g,
			HubPanelBorderColor.b, HubPanelBorderColor.a);
		SDL_RenderRect(Context->MainRenderer, &StatusBounds);
		RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont),
			"World day " + std::to_string(Day) + "  " + TimeText + "    Active AI: " +
			std::to_string(ActiveRacerCount) + "    Live races: " + std::to_string(ActiveRaceCount),
			44, 603, HubTextColor);
		RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont),
			Screen->StatusMessage.empty() ? "Your owned racers are added automatically to prepared races." :
			Screen->StatusMessage, 44, 633,
			Screen->StatusMessage.empty() ? HubMutedColor : SDL_Color{ 239, 190, 133, 255 });

		if (CareerData->InitialWorldMonthSimulationInProgress || CareerData->InitialWorldMonthSimulationFailed)
		{
			const SDL_FRect ProgressPanel{ 250.0f, 276.0f, 524.0f, 188.0f };
			SDL_SetRenderDrawColor(Context->MainRenderer, HubBackgroundColor.r, HubBackgroundColor.g,
				HubBackgroundColor.b, HubBackgroundColor.a);
			SDL_RenderFillRect(Context->MainRenderer, &ProgressPanel);
			SDL_SetRenderDrawColor(Context->MainRenderer, HubButtonBorderColor.r, HubButtonBorderColor.g,
				HubButtonBorderColor.b, HubButtonBorderColor.a);
			SDL_RenderRect(Context->MainRenderer, &ProgressPanel);
			if (CareerData->InitialWorldMonthSimulationFailed)
			{
				RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont),
					"Career warm-up failed. Check the log, then press Escape to return.", 286, 326,
					SDL_Color{ 239, 190, 133, 255 });
			}
			else
			{
				const bool FinishingFirstRaces = CareerData->InitialWorldMonthElapsedRealMs >=
					CareerData->InitialWorldMonthTotalRealMs;
				const double Progress = CareerData->InitialWorldMonthTotalRealMs == 0 ? 0.0 :
					std::clamp(static_cast<double>(CareerData->InitialWorldMonthElapsedRealMs) /
						static_cast<double>(CareerData->InitialWorldMonthTotalRealMs), 0.0, 1.0);
				const SDL_FRect ProgressTrack{ 286.0f, 352.0f, 452.0f, 20.0f };
				SDL_SetRenderDrawColor(Context->MainRenderer, HubPanelBorderColor.r, HubPanelBorderColor.g,
					HubPanelBorderColor.b, HubPanelBorderColor.a);
				SDL_RenderFillRect(Context->MainRenderer, &ProgressTrack);
				const SDL_FRect ProgressFill{ ProgressTrack.x, ProgressTrack.y,
					static_cast<float>(ProgressTrack.w * Progress), ProgressTrack.h };
				SDL_SetRenderDrawColor(Context->MainRenderer, HubAccentColor.r, HubAccentColor.g,
					HubAccentColor.b, HubAccentColor.a);
				SDL_RenderFillRect(Context->MainRenderer, &ProgressFill);
				RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont),
					FinishingFirstRaces ? "Completing first races for active AI racers" :
						"Simulating the first week of career life", 286, 310,
					HubTextColor);
				RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont),
					std::to_string(static_cast<unsigned int>(Progress * 100.0)) + "%  |  " +
					"AI races, league fixtures, and racer history", 286, 390,
					HubMutedColor);
			}
		}
	}

	Display();
	auto EndTime = SDL_GetTicks();
	return EndTime - StartTime;
}

