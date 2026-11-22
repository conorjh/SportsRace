#include "WorldMapScreen.h"
#include "BuildingScreen.h"
#include "BookiesScreen.h"
#include "BreedingFarmScreen.h"
#include "GymScreen.h"
#include "LicenseOfficeScreen.h"
#include "LeagueBrowserScreen.h"
#include "RaceTrackScreen.h"
#include "RacerDBScreen.h"
#include "StableScreen.h"
#include "..\App\App.h"
#include "..\Audio.h"
#include "SDL3\SDL.h"
#include <algorithm>
#include <string>

namespace
{
	constexpr float CityMapLeft = 36.0f;
	constexpr float CityMapTop = 112.0f;
	constexpr float CityPlotWidth = 112.0f;
	constexpr float CityPlotHeight = 92.0f;
	constexpr float CityPlotHorizontalGap = 8.0f;
	constexpr float CityPlotVerticalGap = 8.0f;
	constexpr float CityMapWidth = 952.0f;
	constexpr float CityMapHeight = 492.0f;
	constexpr std::size_t CityMapVisibleRows = 5;
	constexpr SDL_Color CityBackgroundColor{ 22, 28, 29, 255 };
	constexpr SDL_Color CityTextColor{ 239, 231, 214, 255 };
	constexpr SDL_Color CityMutedTextColor{ 174, 166, 147, 255 };
	constexpr SDL_Color CityAccentColor{ 190, 83, 55, 255 };

	std::string FormatWorldTime(std::uint64_t WorldTimeMs)
	{
		const std::uint64_t WorldMinutes = WorldTimeMs / 60000;
		const unsigned int Hour = static_cast<unsigned int>((WorldMinutes / 60) % 24);
		const unsigned int Minute = static_cast<unsigned int>(WorldMinutes % 60);
		return (Hour < 10 ? "0" : "") + std::to_string(Hour) + ":" +
			(Minute < 10 ? "0" : "") + std::to_string(Minute);
	}

	std::string GetPlotActivityStatus(const Game::Career::CareerData& Data, std::size_t PlotIndex)
	{
		if (PlotIndex >= Data.CityPlots.size())
			return {};
		const Game::Career::CityPlot& Plot = Data.CityPlots[PlotIndex];
		if (Plot.Type == Game::Career::CityPlotType::RaceTrack)
		{
			const std::size_t ActiveRaces = static_cast<std::size_t>(std::count_if(
				Data.WorldRaces.begin(), Data.WorldRaces.end(), [PlotIndex](const auto& WorldRace)
				{
					return WorldRace != nullptr && !WorldRace->IsComplete &&
						WorldRace->TrackPlotIndex == PlotIndex;
				}));
			return "LIVE " + std::to_string(ActiveRaces);
		}

		std::size_t NearbyRacers = 0;
		std::size_t TrainingRacers = 0;
		for (const Game::Career::CareerRacerState& State : Data.RacerStates)
		{
			if (State.Activity == Game::Career::CareerRacerActivity::Retired)
				continue;
			const bool IsAtPlot = State.LocationPlotIndex == PlotIndex;
			const bool IsTravellingHere = State.Activity == Game::Career::CareerRacerActivity::Travelling &&
				State.DestinationPlotIndex == PlotIndex;
			if (IsAtPlot || IsTravellingHere)
				++NearbyRacers;
			if (Plot.Type == Game::Career::CityPlotType::Gym &&
				(State.Activity == Game::Career::CareerRacerActivity::Training || IsTravellingHere))
				++TrainingRacers;
		}
		if (Plot.Type == Game::Career::CityPlotType::Gym)
			return "TRAIN " + std::to_string(TrainingRacers);
		return "NEAR " + std::to_string(NearbyRacers);
	}

	SDL_Color GetPlotAccent(Game::Career::CityPlotType Type)
	{
		switch (Type)
		{
		case Game::Career::CityPlotType::Gym:
			return { 103, 151, 110, 255 };
		case Game::Career::CityPlotType::RaceTrack:
			return { 192, 73, 52, 255 };
		case Game::Career::CityPlotType::Bookies:
			return { 184, 145, 74, 255 };
		case Game::Career::CityPlotType::LicenseOffice:
			return { 89, 128, 163, 255 };
		case Game::Career::CityPlotType::BreedingFarm:
			return { 160, 119, 90, 255 };
		case Game::Career::CityPlotType::Stable:
			return { 154, 103, 143, 255 };
		case Game::Career::CityPlotType::VacantLot:
			return { 119, 111, 88, 255 };
		default:
			return { 80, 83, 79, 255 };
		}
	}

	std::string GetPlotLabel(const Game::Career::CityPlot& Plot)
	{
		switch (Plot.Type)
		{
		case Game::Career::CityPlotType::Gym:
			return "GYM\nMunicipal";
		case Game::Career::CityPlotType::RaceTrack:
		{
			const std::size_t NameSeparator = Plot.Name.find_last_of(' ');
			return "TRACK\n" + (NameSeparator == std::string::npos
				? Plot.Name : Plot.Name.substr(NameSeparator + 1));
		}
		case Game::Career::CityPlotType::Bookies:
			return "BOOKIES\nBetting";
		case Game::Career::CityPlotType::LicenseOffice:
			return "LICENSE\nOffice";
		case Game::Career::CityPlotType::BreedingFarm:
			return "BREEDING\nClinic";
		case Game::Career::CityPlotType::Stable:
			return "YOUR\nSTABLE";
		case Game::Career::CityPlotType::VacantLot:
			return "VACANT\nPlot";
		default:
			return "CITY\nScenery";
		}
	}

	void DrawCityButton(Game::Render::BaseRenderer& Renderer, Game::Render::AppRenderContext* Context,
		Game::GUI::Button& Button, bool Enabled)
	{
		const SDL_Color FillColor = !Enabled ? SDL_Color{ 63, 61, 55, 255 } :
			Button.IsMouseOver() ? SDL_Color{ 134, 54, 45, 255 } : SDL_Color{ 104, 42, 36, 255 };
		SDL_FRect Bounds{ static_cast<float>(Button.Rect.x), static_cast<float>(Button.Rect.y),
			static_cast<float>(Button.Rect.w), static_cast<float>(Button.Rect.h) };
		SDL_SetRenderDrawColor(Context->MainRenderer, FillColor.r, FillColor.g, FillColor.b, FillColor.a);
		SDL_RenderFillRect(Context->MainRenderer, &Bounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, 170, 96, 72, 255);
		SDL_RenderRect(Context->MainRenderer, &Bounds);
		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), Button.Text,
			Button.x + 12, Button.y + (Button.h - 16) / 2, Enabled ? CityTextColor : CityMutedTextColor);
	}
}

Game::Screens::WorldMapScreen::WorldMapScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO,
	App::AppData& _Data)
	: AppScreen(_Machine, _IO, _Data),
	Orchestrator(_Data.Career, _Data.Profile),
	PopulationButton(_IO, "Population database", 36, 690, 220, 48),
	CareerHubButton(_IO, "Career hub", 824, 690, 160, 48),
	LeagueBrowserButton(_IO, "League browser", 280, 690, 220, 48)
{
	Type = AppScreenType::WorldMapScreen;
	Orchestrator.Init();
}

void Game::Screens::WorldMapScreen::Entry()
{
}

Game::Screens::AppScreen* Game::Screens::WorldMapScreen::Update()
{
	using namespace Game::Audio;
	PopulationButton.Update();
	CareerHubButton.Update();
	LeagueBrowserButton.Update();

	if (CareerHubButton.HasMouseClicked() || IO.Esc)
	{
		IO.Player.Play(BuiltInSounds::Click);
		ScreenStack.Pop();
		return ScreenStack.Top();
	}
	if (LeagueBrowserButton.HasMouseClicked())
	{
		IO.Player.Play(BuiltInSounds::Click);
		ScreenStack.Push(new LeagueBrowserScreen(ScreenStack, IO, Data));
		return ScreenStack.Top();
	}

	if (Data.Career != nullptr && Data.Career->UpcomingRaceEntrants.empty())
		Orchestrator.PrepareUpcomingRace(SDL_GetTicks());
	const SDL_Point MousePosition{ static_cast<int>(IO.MousePosition.x), static_cast<int>(IO.MousePosition.y) };
	const SDL_Rect CityMapBounds{ static_cast<int>(CityMapLeft), static_cast<int>(CityMapTop),
		static_cast<int>(CityMapWidth), static_cast<int>(CityMapHeight) };
	if (IO.MouseWheelY != 0.0f && SDL_PointInRect(&MousePosition, &CityMapBounds))
	{
		CityScrollRemainder -= IO.MouseWheelY * 2.0f;
		const int RowDelta = static_cast<int>(CityScrollRemainder);
		if (RowDelta != 0)
		{
			const long long NewOffset = static_cast<long long>(CityScrollRow) + RowDelta;
			CityScrollRow = static_cast<std::size_t>(std::clamp<long long>(NewOffset, 0,
				static_cast<long long>(GetMaximumCityScrollRow())));
			CityScrollRemainder -= static_cast<float>(RowDelta);
		}
	}

	if (PopulationButton.HasMouseClicked())
	{
		IO.Player.Play(BuiltInSounds::Click);
		if (Data.Career != nullptr)
		{
			ScreenStack.Push(new RacerDBScreen(ScreenStack, IO, Data, Data.Career->Racers));
			return ScreenStack.Top();
		}
	}

	HoveredPlotIndex = GetPlotAt(IO.MousePosition.x, IO.MousePosition.y);
	const Uint32 RightButtonMask = SDL_BUTTON_MASK(SDL_BUTTON_RIGHT);
	if ((IO.MouseButtonsPressed & RightButtonMask) != 0)
	{
		RightClickPlotIndex = HoveredPlotIndex;
		RightClickStartedAt = SDL_GetTicks();
		PlotInfoVisible = false;
	}
	if ((IO.MouseButtons & RightButtonMask) == 0)
	{
		RightClickPlotIndex = -1;
		PlotInfoVisible = false;
	}
	else if (HoveredPlotIndex != RightClickPlotIndex)
	{
		RightClickPlotIndex = HoveredPlotIndex;
		RightClickStartedAt = SDL_GetTicks();
		PlotInfoVisible = false;
	}
	else if (RightClickPlotIndex >= 0 && SDL_GetTicks() - RightClickStartedAt >= 300)
	{
		InfoPlotIndex = RightClickPlotIndex;
		PlotInfoVisible = true;
	}

	if ((IO.MouseButtonsPressed & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) != 0 && HoveredPlotIndex >= 0 &&
		Data.Career != nullptr)
	{
		const Career::CityPlot& Plot = Data.Career->CityPlots[static_cast<std::size_t>(HoveredPlotIndex)];
		if (Plot.Type != Career::CityPlotType::Scenery && Plot.Type != Career::CityPlotType::VacantLot)
		{
			const unsigned int TrackLength = Plot.Type == Career::CityPlotType::RaceTrack ? Plot.TrackLength : 0;
			if (Plot.Type != Career::CityPlotType::RaceTrack)
				Orchestrator.PrepareUpcomingRace(SDL_GetTicks(), TrackLength);
			BuildingScreen* Building = nullptr;
			switch (Plot.Type)
			{
			case Career::CityPlotType::Bookies:
				Building = new BookiesScreen(ScreenStack, IO, Data, Plot);
				break;
			case Career::CityPlotType::Gym:
				Building = new GymScreen(ScreenStack, IO, Data, Plot);
				break;
			case Career::CityPlotType::LicenseOffice:
				Building = new LicenseOfficeScreen(ScreenStack, IO, Data, Plot);
				break;
			case Career::CityPlotType::BreedingFarm:
				Building = new BreedingFarmScreen(ScreenStack, IO, Data, Plot);
				break;
			case Career::CityPlotType::RaceTrack:
				Building = new RaceTrackScreen(ScreenStack, IO, Data, Plot);
				break;
			case Career::CityPlotType::Stable:
				Building = new StableScreen(ScreenStack, IO, Data, Plot);
				break;
			default:
				break;
			}
			if (Building != nullptr)
			{
				IO.Player.Play(BuiltInSounds::Click);
				ScreenStack.Push(Building);
				return ScreenStack.Top();
			}
		}
	}

	return this;
}

SDL_FRect Game::Screens::WorldMapScreen::GetPlotBounds(std::size_t Index) const
{
	if (Data.Career == nullptr || Index >= Data.Career->CityPlots.size())
		return {};
	const Career::CityPlot& Plot = Data.Career->CityPlots[Index];
	return {
		CityMapLeft + Plot.GridX * (CityPlotWidth + CityPlotHorizontalGap),
		CityMapTop + (static_cast<float>(Plot.GridY) - static_cast<float>(CityScrollRow)) *
			(CityPlotHeight + CityPlotVerticalGap),
		CityPlotWidth,
		CityPlotHeight
	};
}

std::size_t Game::Screens::WorldMapScreen::GetMaximumCityScrollRow() const
{
	if (Data.Career == nullptr || Data.Career->CityPlots.empty())
		return 0;
	unsigned int MaximumGridRow = 0;
	for (const Career::CityPlot& Plot : Data.Career->CityPlots)
		MaximumGridRow = std::max(MaximumGridRow, Plot.GridY);
	const std::size_t RowCount = static_cast<std::size_t>(MaximumGridRow) + 1;
	return RowCount > CityMapVisibleRows ? RowCount - CityMapVisibleRows : 0;
}

int Game::Screens::WorldMapScreen::GetPlotAt(float X, float Y) const
{
	if (Data.Career == nullptr)
		return -1;
	for (std::size_t Index = 0; Index < Data.Career->CityPlots.size(); ++Index)
	{
		const SDL_FRect Bounds = GetPlotBounds(Index);
		if (X >= Bounds.x && X <= Bounds.x + Bounds.w && Y >= Bounds.y && Y <= Bounds.y + Bounds.h)
			return static_cast<int>(Index);
	}
	return -1;
}

const Game::Career::CityPlot* Game::Screens::WorldMapScreen::GetInfoPlot() const
{
	if (!PlotInfoVisible || Data.Career == nullptr || InfoPlotIndex < 0 ||
		static_cast<std::size_t>(InfoPlotIndex) >= Data.Career->CityPlots.size())
		return nullptr;
	return &Data.Career->CityPlots[static_cast<std::size_t>(InfoPlotIndex)];
}

bool Game::Screens::WorldMapScreen::IsPlotInfoVisible() const
{
	return GetInfoPlot() != nullptr;
}

int Game::Screens::WorldMapScreen::GetHoveredPlotIndex() const
{
	return HoveredPlotIndex;
}

Game::Render::WorldMapRenderer::WorldMapRenderer(AppRenderContext* Context)
	: BaseRenderer(Context)
{
}

unsigned int Game::Render::WorldMapRenderer::Render()
{
	const unsigned int StartTime = SDL_GetTicks();
	SDL_SetRenderDrawColor(Context->MainRenderer, CityBackgroundColor.r, CityBackgroundColor.g,
		CityBackgroundColor.b, CityBackgroundColor.a);
	SDL_RenderClear(Context->MainRenderer);
	RenderText(Context->Store.GetFont(Assets::FontAssetMainFont), "NORTHBRIDGE / CAREER CITY", 36, 22,
		CityTextColor);

	if (Screen == nullptr || Screen->Data.Career == nullptr)
	{
		Display();
		return SDL_GetTicks() - StartTime;
	}

	const Career::CareerData& CareerData = *Screen->Data.Career;
	std::size_t InBuildingCount = 0;
	std::size_t TravellingCount = 0;
	std::size_t TrainingCount = 0;
	std::size_t InRaceCount = 0;
	std::size_t WaitingCount = 0;
	for (const Career::CareerRacerState& State : CareerData.RacerStates)
		switch (State.Activity)
		{
		case Career::CareerRacerActivity::InBuilding:
			++InBuildingCount;
			break;
		case Career::CareerRacerActivity::Travelling:
			++TravellingCount;
			break;
		case Career::CareerRacerActivity::Training:
			++TrainingCount;
			break;
		case Career::CareerRacerActivity::InRace:
			++InRaceCount;
			break;
		case Career::CareerRacerActivity::WaitingForLeague:
			++WaitingCount;
			break;
		default:
			break;
		}
	const std::uint64_t WorldMinutes = CareerData.WorldClock.NowMs() / 60000;
	const std::uint64_t WorldDay = WorldMinutes / 1440 + 1;
	const std::size_t ActiveRaceCount = Screen->Orchestrator.GetActiveRaceCount();
	const std::string CityStatus = "DAY " + std::to_string(WorldDay) + "  " +
		FormatWorldTime(CareerData.WorldClock.NowMs()) + "   |   AI " +
		std::to_string(Screen->Orchestrator.GetActiveRacerCount()) + "   |   RACES " +
		std::to_string(ActiveRaceCount) + "   |   CASH $" +
		std::to_string(Screen->Data.Profile == nullptr ? 0 : Screen->Data.Profile->Financials.Cash);
	RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), CityStatus, 36, 52, CityTextColor);
	RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
		"In building " + std::to_string(InBuildingCount) + "   Travelling " + std::to_string(TravellingCount) +
		"   Training " + std::to_string(TrainingCount) + "   In race " + std::to_string(InRaceCount) +
		"   Waiting for league " + std::to_string(WaitingCount), 36, 78, CityMutedTextColor);
	SDL_Rect MapTarget{ static_cast<int>(CityMapLeft), static_cast<int>(CityMapTop),
		static_cast<int>(CityMapWidth), static_cast<int>(CityMapHeight) };
	Game::Render::Image* Map = Context->Store.GetImage(Assets::ImageAssetWorldMap);
	if (Map != nullptr && Map->Texture != nullptr)
		RenderImage(Map->Texture, nullptr, &MapTarget);
	else
	{
		const SDL_FRect MapBounds{ CityMapLeft, CityMapTop, CityMapWidth, CityMapHeight };
		SDL_SetRenderDrawColor(Context->MainRenderer, 114, 125, 99, 255);
		SDL_RenderFillRect(Context->MainRenderer, &MapBounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, 173, 165, 137, 255);
		SDL_RenderRect(Context->MainRenderer, &MapBounds);
	}

	for (std::size_t Index = 0; Index < CareerData.CityPlots.size(); ++Index)
	{
		const Career::CityPlot& Plot = CareerData.CityPlots[Index];
		const SDL_FRect Bounds = Screen->GetPlotBounds(Index);
		if (Bounds.y < CityMapTop || Bounds.y + Bounds.h > CityMapTop + CityMapHeight)
			continue;
		const bool IsHovered = Screen->GetHoveredPlotIndex() == static_cast<int>(Index);
		if (Plot.Type == Career::CityPlotType::Scenery)
		{
			const Assets::ImageAssetId TileAsset = static_cast<Assets::ImageAssetId>(
				Assets::ImageAssetCityTile000 + Plot.SpriteIndex);
			Game::Render::Image* Tile = Context->Store.GetImage(TileAsset);
			if (Tile != nullptr && Tile->Texture != nullptr)
			{
				SDL_Rect TileTarget{ static_cast<int>(Bounds.x + 8), static_cast<int>(Bounds.y + 7),
					static_cast<int>(Bounds.w - 16), static_cast<int>(Bounds.h - 14) };
				RenderImage(Tile->Texture, nullptr, &TileTarget);
			}
			if (IsHovered)
			{
				SDL_SetRenderDrawColor(Context->MainRenderer, 239, 224, 188, 255);
				SDL_RenderRect(Context->MainRenderer, &Bounds);
			}
			continue;
		}

		const SDL_Color Accent = GetPlotAccent(Plot.Type);
		const SDL_FRect MarkerBounds{ Bounds.x + 4, Bounds.y + 6, Bounds.w - 8, Bounds.h - 12 };
		const SDL_Color MarkerFill = Plot.Type == Career::CityPlotType::VacantLot
			? SDL_Color{ 44, 57, 46, 255 } : SDL_Color{ 34, 33, 30, 255 };
		SDL_SetRenderDrawColor(Context->MainRenderer, MarkerFill.r, MarkerFill.g, MarkerFill.b,
			MarkerFill.a);
		SDL_RenderFillRect(Context->MainRenderer, &MarkerBounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, Accent.r, Accent.g, Accent.b, Accent.a);
		const SDL_FRect AccentBar{ MarkerBounds.x, MarkerBounds.y, 4.0f, MarkerBounds.h };
		SDL_RenderFillRect(Context->MainRenderer, &AccentBar);
		SDL_RenderRect(Context->MainRenderer, &MarkerBounds);
		if (Plot.Type == Career::CityPlotType::VacantLot)
			RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), GetPlotLabel(Plot),
				static_cast<int>(MarkerBounds.x + 12), static_cast<int>(MarkerBounds.y + 20), CityTextColor);
		else
		{
			const Assets::ImageAssetId TileAsset = static_cast<Assets::ImageAssetId>(
				Assets::ImageAssetCityTile000 + Plot.SpriteIndex);
			Game::Render::Image* Tile = Context->Store.GetImage(TileAsset);
			if (Tile != nullptr && Tile->Texture != nullptr)
			{
				SDL_Rect IconTarget{ static_cast<int>(MarkerBounds.x + 8),
					static_cast<int>(MarkerBounds.y + 14), 36, 36 };
				RenderImage(Tile->Texture, nullptr, &IconTarget);
			}
			RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), GetPlotLabel(Plot),
				static_cast<int>(MarkerBounds.x + 48), static_cast<int>(MarkerBounds.y + 17), CityTextColor);
		}
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
			GetPlotActivityStatus(CareerData, Index), static_cast<int>(MarkerBounds.x + 8),
			static_cast<int>(MarkerBounds.y + 62), CityMutedTextColor);
		if (IsHovered)
		{
			SDL_SetRenderDrawColor(Context->MainRenderer, 239, 224, 188, 255);
			SDL_RenderRect(Context->MainRenderer, &MarkerBounds);
		}
	}
	if (Screen->GetMaximumCityScrollRow() > 0)
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
			"Scroll over the map to explore outer districts (row " +
			std::to_string(Screen->CityScrollRow + 1) + ")", 36, 96, CityMutedTextColor);

	if (const Career::CityPlot* InfoPlot = Screen->GetInfoPlot())
	{
		const SDL_FRect InfoBounds{ 288.0f, 624.0f, 440.0f, 52.0f };
		SDL_SetRenderDrawColor(Context->MainRenderer, 34, 33, 30, 255);
		SDL_RenderFillRect(Context->MainRenderer, &InfoBounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, 125, 111, 88, 255);
		SDL_RenderRect(Context->MainRenderer, &InfoBounds);
		std::string Description = InfoPlot->Name;
		if (InfoPlot->Type == Career::CityPlotType::RaceTrack)
			Description += " - " + std::to_string(InfoPlot->TrackLength / 50) + "m track";
		else if (InfoPlot->Type == Career::CityPlotType::VacantLot)
			Description += " - available for future development";
		else if (InfoPlot->Type == Career::CityPlotType::Scenery)
			Description += " - unavailable city scenery";
		else
			Description += " - hold right-click to inspect; left-click to enter";
		RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), Description, 302, 642, CityTextColor);
	}

	DrawCityButton(*this, Context, Screen->PopulationButton, true);
	DrawCityButton(*this, Context, Screen->LeagueBrowserButton, true);
	DrawCityButton(*this, Context, Screen->CareerHubButton, true);
	Display();
	return SDL_GetTicks() - StartTime;
}
