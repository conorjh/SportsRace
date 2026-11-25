#include "CareerHub.h"
#include "..\Audio.h"
#include "RacerDBScreen.h"
#include "LeagueBrowserScreen.h"
#include "WorldMapScreen.h"
#include "StableScreen.h"
#include "BreedingFarmScreen.h"
#include "SDL3\SDL.h"
#include "..\App\App.h"
#include "spdlog/spdlog.h"
#include <algorithm>

namespace CareerHubScreenDetail
{
	constexpr SDL_Color HubBackgroundColor{ 23, 22, 20, 255 };
	constexpr SDL_Color HubPanelColor{ 34, 33, 30, 255 };
	constexpr SDL_Color HubPanelBorderColor{ 75, 70, 60, 255 };
	constexpr SDL_Color HubTextColor{ 235, 229, 215, 255 };
	constexpr SDL_Color HubMutedColor{ 164, 155, 138, 255 };
	constexpr SDL_Color HubAccentColor{ 178, 61, 49, 255 };
	constexpr SDL_Color HubButtonHoverColor{ 134, 54, 45, 255 };
	constexpr SDL_Color HubButtonBorderColor{ 170, 96, 72, 255 };
	constexpr int HubNavigationButtonHeight = 52;

	enum class CareerHubIcon
	{
		Leagues,
		Database,
		WorldMap,
		Stable,
		Breeder,
		Save
	};

	void DrawCareerHubIcon(Game::Render::BaseRenderer& Renderer, Game::Render::AppRenderContext* Context,
		CareerHubIcon Icon, SDL_Rect Bounds)
	{
		if (Icon == CareerHubIcon::Database)
		{
			const SDL_FRect Frame{ static_cast<float>(Bounds.x + 3), static_cast<float>(Bounds.y + 3),
				static_cast<float>(Bounds.w - 6), static_cast<float>(Bounds.h - 6) };
			SDL_SetRenderDrawColor(Context->MainRenderer, HubTextColor.r, HubTextColor.g,
				HubTextColor.b, HubTextColor.a);
			SDL_RenderRect(Context->MainRenderer, &Frame);
			for (int Row = 1; Row <= 2; ++Row)
				SDL_RenderLine(Context->MainRenderer, Frame.x + 2.0f, Frame.y + Row * 7.0f,
					Frame.x + Frame.w - 2.0f, Frame.y + Row * 7.0f);
			return;
		}
		if (Icon == CareerHubIcon::Save)
		{
			const SDL_FRect Frame{ static_cast<float>(Bounds.x + 4), static_cast<float>(Bounds.y + 2),
				static_cast<float>(Bounds.w - 8), static_cast<float>(Bounds.h - 4) };
			SDL_SetRenderDrawColor(Context->MainRenderer, HubTextColor.r, HubTextColor.g,
				HubTextColor.b, HubTextColor.a);
			SDL_RenderRect(Context->MainRenderer, &Frame);
			const SDL_FRect Label{ Frame.x + 5.0f, Frame.y + 15.0f, Frame.w - 10.0f, 8.0f };
			SDL_RenderRect(Context->MainRenderer, &Label);
			return;
		}

		Game::Assets::ImageAssetId Asset = Game::Assets::ImageAssetRacerIcon;
		if (Icon == CareerHubIcon::Leagues)
			Asset = Game::Assets::ImageAssetRaceIcon;
		else if (Icon == CareerHubIcon::WorldMap)
			Asset = Game::Assets::ImageAssetWorldMap;
		else if (Icon == CareerHubIcon::Breeder)
			Asset = Game::Assets::ImageAssetTrainingIcon;

		Game::Render::Image* Image = Context->Store.GetImage(Asset);
		if (Image != nullptr && Image->Texture != nullptr)
		{
			Renderer.RenderImage(Image->Texture, nullptr, &Bounds);
		}
	}

	void DrawCareerHubNavigationButton(Game::Render::BaseRenderer& Renderer,
		Game::Render::AppRenderContext* Context, const Game::GUI::Button& Button, CareerHubIcon Icon)
	{
		const SDL_FRect ButtonBounds{ static_cast<float>(Button.Rect.x), static_cast<float>(Button.Rect.y),
			static_cast<float>(Button.Rect.w), static_cast<float>(Button.Rect.h) };
		const SDL_Color Fill = Button.IsMouseOver() ? HubButtonHoverColor : HubPanelColor;
		SDL_SetRenderDrawColor(Context->MainRenderer, Fill.r, Fill.g, Fill.b, Fill.a);
		SDL_RenderFillRect(Context->MainRenderer, &ButtonBounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, HubButtonBorderColor.r, HubButtonBorderColor.g,
			HubButtonBorderColor.b, HubButtonBorderColor.a);
		SDL_RenderRect(Context->MainRenderer, &ButtonBounds);
		const SDL_Rect IconBounds{ Button.Rect.x + 10, Button.Rect.y + (Button.Rect.h - 30) / 2, 30, 30 };
		DrawCareerHubIcon(Renderer, Context, Icon, IconBounds);
		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), Button.Text,
			Button.Rect.x + 48, Button.Rect.y + (Button.Rect.h - 16) / 2, HubTextColor);
	}

	const Game::Career::CareerWorldCity* FindCareerHubCity(const Game::Career::CareerData& Data,
		std::uint64_t CityId)
	{
		const auto City = std::find_if(Data.Cities.begin(), Data.Cities.end(), [CityId](
			const Game::Career::CareerWorldCity& Candidate) { return Candidate.Id == CityId; });
		return City == Data.Cities.end() ? nullptr : &*City;
	}

	const std::vector<Game::Career::CityPlot>* GetCareerHubPlots(const Game::Career::CareerData& Data,
		std::uint64_t CityId)
	{
		if (CityId == Data.HomeCityId && CityId != 0)
			return &Data.CityPlots;
		const Game::Career::CareerWorldCity* City = FindCareerHubCity(Data, CityId);
		return City == nullptr ? nullptr : &City->Plots;
	}

	std::size_t FindCareerHubPlotIndex(const std::vector<Game::Career::CityPlot>& Plots,
		Game::Career::CityPlotType Type)
	{
		const auto Plot = std::find_if(Plots.begin(), Plots.end(), [Type](const Game::Career::CityPlot& Candidate)
			{ return Candidate.Type == Type; });
		return Plot == Plots.end() ? Plots.size() : static_cast<std::size_t>(Plot - Plots.begin());
	}

	struct CareerHubLayout
	{
		int Width;
		int Height;
		int Margin;
		int TopY;
		int BottomY;
		int MapTitleY;
		int FooterY;
		int MessageY;
	};

	CareerHubLayout GetCareerHubLayout(int Width, int Height)
	{
		const Game::GUI::RelativeLayout Relative(static_cast<float>(Width), static_cast<float>(Height));
		return { Width, Height, Relative.X(0.025f), Relative.Y(0.02f), Height - 66,
			Relative.Y(0.10f), Height - 148, Height - 114 };
	}
}
using namespace CareerHubScreenDetail;


Game::Screens::CareerHubScreen::CareerHubScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO,
	App::AppData& _Data)
	: AppScreen(_Machine, _IO, _Data),
	Orchestrator(_Data.Career, _Data.Profile),
	LeagueBrowserButton(_IO, "Leagues", 16, 14, 145, 52),
	DatabaseButton(_IO, "Database", 169, 14, 145, 52),
	WorldMapButton(_IO, "World map", 169, 694, 180, 52),
	StableButton(_IO, "Stable", 361, 694, 160, 52),
	BreedingFarmButton(_IO, "Breeder", 533, 694, 160, 52),
	SaveCareerButton(_IO, "Save", 705, 694, 150, 52)
{
	Type = AppScreenType::CareerHub;
	UpdateLayout();
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
	const bool HasStable = Data.Career != nullptr && Data.Career->PlayerStableCityId != 0;
	StableButton.Text = HasStable ? "Stable" : "Rent stable";
	UpdateLayout();
	LeagueBrowserButton.Update();
	DatabaseButton.Update();
	WorldMapButton.Update();
	StableButton.Update();
	BreedingFarmButton.Update();
	SaveCareerButton.Update();
	if (StableButton.HasMouseClicked())
	{
		if (Data.Career == nullptr)
			StatusMessage = "Career data is unavailable.";
		else
		{
			const std::uint64_t CityId = HasStable ? Data.Career->PlayerStableCityId : Data.Career->HomeCityId;
			const std::vector<Career::CityPlot>* CityPlots = GetCareerHubPlots(*Data.Career, CityId);
			const std::size_t PlotIndex = CityPlots == nullptr ? 0 :
				HasStable ? Data.Career->PlayerStablePlotIndex :
				FindCareerHubPlotIndex(*CityPlots, Career::CityPlotType::Stable);
			if (CityPlots == nullptr || PlotIndex >= CityPlots->size() ||
				(*CityPlots)[PlotIndex].Type != Career::CityPlotType::Stable)
				StatusMessage = "Could not locate a stable in this city.";
			else
			{
				ScreenStack.Push(new StableScreen(ScreenStack, IO, Data, (*CityPlots)[PlotIndex], CityId,
					PlotIndex));
				return ScreenStack.Top();
			}
		}
	}
	if (LeagueBrowserButton.HasMouseClicked())
	{
		LeagueBrowserLocationContext LocationContext;
		if (HasStable)
		{
			LocationContext.CityId = Data.Career->PlayerStableCityId;
			LocationContext.Filter = LeagueBrowserLocationFilter::City;
			if (const Career::CareerWorldCity* City = FindCareerHubCity(*Data.Career,
				Data.Career->PlayerStableCityId))
			{
				LocationContext.RegionId = City->RegionId;
				const auto Region = std::find_if(Data.Career->Regions.begin(), Data.Career->Regions.end(),
					[City](const Career::CareerWorldRegion& Candidate) { return Candidate.Id == City->RegionId; });
				if (Region != Data.Career->Regions.end())
					LocationContext.CountryId = Region->CountryId;
			}
		}
		ScreenStack.Push(new LeagueBrowserScreen(ScreenStack, IO, Data, LocationContext));
		return ScreenStack.Top();
	}
	if (DatabaseButton.HasMouseClicked())
	{
		if (Data.Career == nullptr)
			StatusMessage = "Career data is unavailable.";
		else
		{
			ScreenStack.Push(new RacerDBScreen(ScreenStack, IO, Data, Data.Career->Racers));
			return ScreenStack.Top();
		}
	}
	if (WorldMapButton.HasMouseClicked())
	{
		ScreenStack.Push(new WorldMapScreen(ScreenStack, IO, Data));
		return ScreenStack.Top();
	}
	if (BreedingFarmButton.HasMouseClicked())
	{
		if (Data.Career == nullptr)
			StatusMessage = "Career data is unavailable.";
		else
		{
			const std::uint64_t CityId = HasStable ? Data.Career->PlayerStableCityId : Data.Career->HomeCityId;
			const std::vector<Career::CityPlot>* CityPlots = GetCareerHubPlots(*Data.Career, CityId);
			const std::size_t PlotIndex = CityPlots == nullptr ? 0 :
				FindCareerHubPlotIndex(*CityPlots, Career::CityPlotType::BreedingFarm);
			if (CityPlots == nullptr || PlotIndex >= CityPlots->size())
				StatusMessage = "Could not locate a breeder in this city.";
			else
			{
				ScreenStack.Push(new BreedingFarmScreen(ScreenStack, IO, Data, (*CityPlots)[PlotIndex]));
				return ScreenStack.Top();
			}
		}
	}
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

	if (IO.Esc)
	{
		IO.Player.Play(Audio::BuiltInSounds::Click);

		ScreenStack.Pop();
		return ScreenStack.Top();
	}

	return this;
}

void Game::Screens::CareerHubScreen::UpdateLayout()
{
	const unsigned int ScreenWidth = Data.RenderContext == nullptr ? 1024 : Data.RenderContext->ScreenWidth;
	const unsigned int ScreenHeight = Data.RenderContext == nullptr ? 768 : Data.RenderContext->ScreenHeight;
	const GUI::RelativeLayout Layout(static_cast<float>(ScreenWidth), static_cast<float>(ScreenHeight));
	const int TopY = Layout.Y(0.02f);
	const int TopButtonWidth = 145;
	LeagueBrowserButton.SetBounds(Layout.X(0.02f), TopY, TopButtonWidth, HubNavigationButtonHeight);
	DatabaseButton.SetBounds(Layout.X(0.17f), TopY, TopButtonWidth, HubNavigationButtonHeight);

	const int BottomGap = Layout.X(0.015f);
	const int BottomWidth = 160;
	const int BottomRowWidth = BottomWidth * 3 + 150 + BottomGap * 3;
	const int BottomLeft = std::max(Layout.X(0.02f),
		(static_cast<int>(ScreenWidth) - BottomRowWidth) / 2);
	const int BottomY = static_cast<int>(ScreenHeight) - HubNavigationButtonHeight - Layout.Y(0.02f);
	WorldMapButton.SetBounds(BottomLeft, BottomY, BottomWidth, HubNavigationButtonHeight);
	StableButton.SetBounds(BottomLeft + BottomWidth + BottomGap, BottomY,
		BottomWidth, HubNavigationButtonHeight);
	BreedingFarmButton.SetBounds(BottomLeft + (BottomWidth + BottomGap) * 2, BottomY,
		BottomWidth, HubNavigationButtonHeight);
	SaveCareerButton.SetBounds(BottomLeft + (BottomWidth + BottomGap) * 3, BottomY,
		150, HubNavigationButtonHeight);
}

Game::Career::CareerData* Game::Screens::CareerHubScreen::GetCareerData() const
{
	return Orchestrator.Data;
}

int Game::Screens::CareerHubScreen::GetPlayerCash() const
{
	return Data.Profile == nullptr ? 0 : Data.Profile->Financials.Cash;
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
	if (Screen == nullptr)
	{
		Display();
		return SDL_GetTicks() - StartTime;
	}

	const int ScreenWidth = static_cast<int>(Context->ScreenWidth);
	const int ScreenHeight = static_cast<int>(Context->ScreenHeight);
	const CareerHubLayout Layout = GetCareerHubLayout(ScreenWidth, ScreenHeight);
	const float HubMapLeft = static_cast<float>(Layout.Margin);
	const float HubMapTop = static_cast<float>(Layout.MapTitleY + 44);
	const float HubMapWidth = static_cast<float>(Layout.Width - Layout.Margin * 2);
	const float HubMapHeight = static_cast<float>(Layout.FooterY - Layout.MapTitleY - 52);
	const Career::CareerData* CareerData = Screen->GetCareerData();
	const SDL_FRect MapBounds{ HubMapLeft, HubMapTop, HubMapWidth, HubMapHeight };
	if (CareerData != nullptr)
		DrawCareerHubMapPreview(Context, *CareerData, CareerData->PlayerStableCityId, MapBounds);
	else
	{
		SDL_SetRenderDrawColor(Context->MainRenderer, HubPanelColor.r, HubPanelColor.g,
			HubPanelColor.b, HubPanelColor.a);
		SDL_RenderFillRect(Context->MainRenderer, &MapBounds);
	}

	const std::uint64_t StableCityId = CareerData == nullptr ? 0 : CareerData->PlayerStableCityId;
	const Career::CareerWorldCity* StableCity = StableCityId == 0 ? nullptr :
		FindCareerHubCity(*CareerData, StableCityId);
	const SDL_FRect MapTitleBounds{ static_cast<float>(Layout.Margin + 12),
		static_cast<float>(Layout.MapTitleY), static_cast<float>(std::min(480, Layout.Width - Layout.Margin * 2)),
		34.0f };
	SDL_SetRenderDrawColor(Context->MainRenderer, 27, 26, 23, 228);
	SDL_RenderFillRect(Context->MainRenderer, &MapTitleBounds);
	RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), StableCity == nullptr
		? "WORLD MAP  |  NO STABLE RENTED" : StableCity->Name + "  |  YOUR STABLE",
		Layout.Margin + 24, Layout.MapTitleY + 9, HubTextColor);

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
		const std::size_t OwnedCount = CareerData->OwnedRacers.size();
		const SDL_FRect StatusBounds{ static_cast<float>(Layout.Margin), static_cast<float>(Layout.FooterY),
			static_cast<float>(Layout.Width - Layout.Margin * 2), 28.0f };
		SDL_SetRenderDrawColor(Context->MainRenderer, 27, 26, 23, 228);
		SDL_RenderFillRect(Context->MainRenderer, &StatusBounds);
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
			"DAY " + std::to_string(Day) + "  " + TimeText + "   |   AI " +
			std::to_string(ActiveRacerCount) + "   |   LIVE RACES " + std::to_string(ActiveRaceCount) +
			"   |   ROSTER " + std::to_string(OwnedCount) + "/" +
			std::to_string(Career::CareerStableRacerCapacity) + "   |   CASH $" +
			std::to_string(Screen->GetPlayerCash()),
			Layout.Margin + 12, Layout.FooterY + 8, HubTextColor);
		const std::string HubMessage = Screen->StatusMessage.empty()
			? StableCity == nullptr ? "Rent stable space to begin building your racing team."
				: "Use the city map to visit your stable, breeder, tracks, and training facilities."
			: Screen->StatusMessage;
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), HubMessage,
			Layout.Margin + 12, Layout.MessageY, Screen->StatusMessage.empty() ? HubMutedColor :
				SDL_Color{ 239, 190, 133, 255 });

		if (CareerData->InitialWorldMonthSimulationInProgress || CareerData->InitialWorldMonthSimulationFailed)
		{
			const float ProgressPanelWidth = static_cast<float>(std::min(524, Layout.Width - 80));
			const float ProgressPanelHeight = 188.0f;
			const SDL_FRect ProgressPanel{ (Layout.Width - ProgressPanelWidth) * 0.5f,
				(Layout.Height - ProgressPanelHeight) * 0.5f, ProgressPanelWidth, ProgressPanelHeight };
			SDL_SetRenderDrawColor(Context->MainRenderer, HubBackgroundColor.r, HubBackgroundColor.g,
				HubBackgroundColor.b, HubBackgroundColor.a);
			SDL_RenderFillRect(Context->MainRenderer, &ProgressPanel);
			SDL_SetRenderDrawColor(Context->MainRenderer, HubButtonBorderColor.r, HubButtonBorderColor.g,
				HubButtonBorderColor.b, HubButtonBorderColor.a);
			SDL_RenderRect(Context->MainRenderer, &ProgressPanel);
			if (CareerData->InitialWorldMonthSimulationFailed)
			{
				RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont),
				"Career warm-up failed. Check the log, then press Escape to return.",
				static_cast<int>(ProgressPanel.x + 28.0f), static_cast<int>(ProgressPanel.y + 84.0f),
					SDL_Color{ 239, 190, 133, 255 });
			}
			else
			{
				const bool FinishingFirstRaces = CareerData->InitialWorldMonthElapsedRealMs >=
					CareerData->InitialWorldMonthTotalRealMs;
				const double Progress = CareerData->InitialWorldMonthTotalRealMs == 0 ? 0.0 :
					std::clamp(static_cast<double>(CareerData->InitialWorldMonthElapsedRealMs) /
						static_cast<double>(CareerData->InitialWorldMonthTotalRealMs), 0.0, 1.0);
				const SDL_FRect ProgressTrack{ ProgressPanel.x + 36.0f, ProgressPanel.y + 76.0f,
					ProgressPanel.w - 72.0f, 20.0f };
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
						"Simulating the first week of career life",
						static_cast<int>(ProgressPanel.x + 36.0f), static_cast<int>(ProgressPanel.y + 34.0f),
					HubTextColor);
				RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont),
					std::to_string(static_cast<unsigned int>(Progress * 100.0)) + "%  |  " +
					"AI races, league fixtures, and racer history",
					static_cast<int>(ProgressPanel.x + 36.0f), static_cast<int>(ProgressPanel.y + 116.0f),
					HubMutedColor);
			}
		}
	}
	DrawCareerHubNavigationButton(*this, Context, Screen->LeagueBrowserButton, CareerHubIcon::Leagues);
	DrawCareerHubNavigationButton(*this, Context, Screen->DatabaseButton, CareerHubIcon::Database);
	DrawCareerHubNavigationButton(*this, Context, Screen->WorldMapButton, CareerHubIcon::WorldMap);
	DrawCareerHubNavigationButton(*this, Context, Screen->StableButton, CareerHubIcon::Stable);
	DrawCareerHubNavigationButton(*this, Context, Screen->BreedingFarmButton, CareerHubIcon::Breeder);
	DrawCareerHubNavigationButton(*this, Context, Screen->SaveCareerButton, CareerHubIcon::Save);

	Display();
	auto EndTime = SDL_GetTicks();
	return EndTime - StartTime;
}

