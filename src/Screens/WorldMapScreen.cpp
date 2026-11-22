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
#include <array>
#include <cmath>
#include <limits>
#include <random>
#include <string>
#include <vector>

namespace
{
	constexpr float CityMapLeft = 36.0f;
	constexpr float CityMapTop = 112.0f;
	constexpr float CityMapWidth = 952.0f;
	constexpr float CityMapHeight = 492.0f;
	constexpr float IsometricTileWidth = 128.0f;
	constexpr float IsometricTileHeight = 64.0f;
	constexpr float MinimumCityZoom = 0.55f;
	constexpr float MaximumCityZoom = 1.65f;
	constexpr SDL_Color CityBackgroundColor{ 22, 28, 29, 255 };
	constexpr SDL_Color CityTextColor{ 239, 231, 214, 255 };
	constexpr SDL_Color CityMutedTextColor{ 174, 166, 147, 255 };
	constexpr std::array<Game::Assets::ImageAssetId, 4> IsometricLandscapeAssets{
		Game::Assets::ImageAssetIsometricLandscape000,
		Game::Assets::ImageAssetIsometricLandscape001,
		Game::Assets::ImageAssetIsometricLandscape002,
		Game::Assets::ImageAssetIsometricLandscape003
	};
	constexpr std::array<Game::Assets::ImageAssetId, 12> IsometricCityAssets{
		Game::Assets::ImageAssetIsometricCity000, Game::Assets::ImageAssetIsometricCity001,
		Game::Assets::ImageAssetIsometricCity002, Game::Assets::ImageAssetIsometricCity003,
		Game::Assets::ImageAssetIsometricCity004, Game::Assets::ImageAssetIsometricCity005,
		Game::Assets::ImageAssetIsometricCity006, Game::Assets::ImageAssetIsometricCity007,
		Game::Assets::ImageAssetIsometricCity008, Game::Assets::ImageAssetIsometricCity009,
		Game::Assets::ImageAssetIsometricCity010, Game::Assets::ImageAssetIsometricCity011
	};
	constexpr std::array<Game::Assets::ImageAssetId, 12> IsometricBuildingAssets{
		Game::Assets::ImageAssetIsometricBuilding000, Game::Assets::ImageAssetIsometricBuilding001,
		Game::Assets::ImageAssetIsometricBuilding002, Game::Assets::ImageAssetIsometricBuilding003,
		Game::Assets::ImageAssetIsometricBuilding004, Game::Assets::ImageAssetIsometricBuilding005,
		Game::Assets::ImageAssetIsometricBuilding006, Game::Assets::ImageAssetIsometricBuilding007,
		Game::Assets::ImageAssetIsometricBuilding008, Game::Assets::ImageAssetIsometricBuilding009,
		Game::Assets::ImageAssetIsometricBuilding010, Game::Assets::ImageAssetIsometricBuilding011
	};
	constexpr std::array<Game::Assets::ImageAssetId, 5> IsometricVehicleAssets{
		Game::Assets::ImageAssetIsometricVehicleBlack, Game::Assets::ImageAssetIsometricVehicleBlue,
		Game::Assets::ImageAssetIsometricVehicleGreen, Game::Assets::ImageAssetIsometricVehicleRed,
		Game::Assets::ImageAssetIsometricVehicleSilver
	};

	struct CityWorldBounds
	{
		float MinX;
		float MaxX;
		float MinY;
		float MaxY;
	};

	SDL_FPoint ProjectCityGrid(float GridX, float GridY)
	{
		return { (GridX - GridY) * (IsometricTileWidth * 0.5f),
			(GridX + GridY) * (IsometricTileHeight * 0.5f) };
	}

	SDL_FPoint ProjectCityToScreen(const SDL_FPoint& Position, float CameraX, float CameraY, float Zoom)
	{
		return { CityMapLeft + CityMapWidth * 0.5f + (Position.x - CameraX) * Zoom,
			CityMapTop + CityMapHeight * 0.5f + (Position.y - CameraY) * Zoom };
	}

	SDL_FPoint UnprojectCityScreenToGrid(float ScreenX, float ScreenY, float CameraX, float CameraY,
		float Zoom)
	{
		const float WorldX = CameraX + (ScreenX - (CityMapLeft + CityMapWidth * 0.5f)) / Zoom;
		const float WorldY = CameraY + (ScreenY - (CityMapTop + CityMapHeight * 0.5f)) / Zoom;
		return { WorldX / IsometricTileWidth + WorldY / IsometricTileHeight,
			WorldY / IsometricTileHeight - WorldX / IsometricTileWidth };
	}

	CityWorldBounds GetCityWorldBounds(const Game::Career::CareerData* Data)
	{
		if (Data == nullptr || Data->CityPlots.empty())
			return { -IsometricTileWidth, IsometricTileWidth, -IsometricTileHeight * 2.0f,
				IsometricTileHeight * 2.0f };

		CityWorldBounds Bounds{
			std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()
		};
		for (const Game::Career::CityPlot& Plot : Data->CityPlots)
		{
			const SDL_FPoint Position = ProjectCityGrid(static_cast<float>(Plot.GridX),
				static_cast<float>(Plot.GridY));
			Bounds.MinX = std::min(Bounds.MinX, Position.x - 80.0f);
			Bounds.MaxX = std::max(Bounds.MaxX, Position.x + 80.0f);
			Bounds.MinY = std::min(Bounds.MinY, Position.y - 112.0f);
			Bounds.MaxY = std::max(Bounds.MaxY, Position.y + 80.0f);
		}
		return Bounds;
	}

	SDL_Rect GetCityMapViewportBounds()
	{
		return { static_cast<int>(CityMapLeft), static_cast<int>(CityMapTop),
			static_cast<int>(CityMapWidth), static_cast<int>(CityMapHeight) };
	}

	SDL_Color GetPlotGroundColor(Game::Career::CityPlotType Type)
	{
		switch (Type)
		{
		case Game::Career::CityPlotType::VacantLot:
			return { 112, 91, 61, 255 };
		case Game::Career::CityPlotType::RaceTrack:
			return { 70, 77, 70, 255 };
		case Game::Career::CityPlotType::Scenery:
			return { 66, 94, 65, 255 };
		default:
			return { 79, 91, 73, 255 };
		}
	}

	void DrawFilledDiamond(SDL_Renderer* Renderer, const SDL_FPoint& Center, float HalfWidth,
		float HalfHeight, SDL_Color Color)
	{
		const SDL_FColor VertexColor{ static_cast<float>(Color.r) / 255.0f,
			static_cast<float>(Color.g) / 255.0f, static_cast<float>(Color.b) / 255.0f,
			static_cast<float>(Color.a) / 255.0f };
		const SDL_Vertex Vertices[] = {
			{ { Center.x, Center.y - HalfHeight }, VertexColor, { 0.0f, 0.0f } },
			{ { Center.x + HalfWidth, Center.y }, VertexColor, { 0.0f, 0.0f } },
			{ { Center.x, Center.y + HalfHeight }, VertexColor, { 0.0f, 0.0f } },
			{ { Center.x - HalfWidth, Center.y }, VertexColor, { 0.0f, 0.0f } }
		};
		constexpr int Indices[]{ 0, 1, 2, 0, 2, 3 };
		SDL_RenderGeometry(Renderer, nullptr, Vertices, 4, Indices, 6);
	}

	void DrawDiamondOutline(SDL_Renderer* Renderer, const SDL_FPoint& Center, float HalfWidth,
		float HalfHeight, SDL_Color Color)
	{
		SDL_SetRenderDrawColor(Renderer, Color.r, Color.g, Color.b, Color.a);
		SDL_RenderLine(Renderer, Center.x, Center.y - HalfHeight, Center.x + HalfWidth, Center.y);
		SDL_RenderLine(Renderer, Center.x + HalfWidth, Center.y, Center.x, Center.y + HalfHeight);
		SDL_RenderLine(Renderer, Center.x, Center.y + HalfHeight, Center.x - HalfWidth, Center.y);
		SDL_RenderLine(Renderer, Center.x - HalfWidth, Center.y, Center.x, Center.y - HalfHeight);
	}

	void DrawThickLine(SDL_Renderer* Renderer, const SDL_FPoint& Start, const SDL_FPoint& End,
		float Width, SDL_Color Color)
	{
		const float DeltaX = End.x - Start.x;
		const float DeltaY = End.y - Start.y;
		const float Length = std::hypot(DeltaX, DeltaY);
		if (Length <= 0.01f)
			return;

		const float NormalX = -DeltaY / Length * Width * 0.5f;
		const float NormalY = DeltaX / Length * Width * 0.5f;
		const SDL_FColor VertexColor{ static_cast<float>(Color.r) / 255.0f,
			static_cast<float>(Color.g) / 255.0f, static_cast<float>(Color.b) / 255.0f,
			static_cast<float>(Color.a) / 255.0f };
		const SDL_Vertex Vertices[] = {
			{ { Start.x + NormalX, Start.y + NormalY }, VertexColor, { 0.0f, 0.0f } },
			{ { End.x + NormalX, End.y + NormalY }, VertexColor, { 0.0f, 0.0f } },
			{ { End.x - NormalX, End.y - NormalY }, VertexColor, { 0.0f, 0.0f } },
			{ { Start.x - NormalX, Start.y - NormalY }, VertexColor, { 0.0f, 0.0f } }
		};
		constexpr int Indices[]{ 0, 1, 2, 0, 2, 3 };
		SDL_RenderGeometry(Renderer, nullptr, Vertices, 4, Indices, 6);
	}

	void DrawLandscapeTile(Game::Render::AppRenderContext* Context, Game::Render::Image* Image,
		const SDL_FPoint& Center, float Zoom)
	{
		if (Image == nullptr || Image->Texture == nullptr)
			return;
		const SDL_FRect Target{ Center.x - IsometricTileWidth * 0.5f * Zoom,
			Center.y - IsometricTileHeight * 0.5f * Zoom, IsometricTileWidth * Zoom,
			IsometricTileHeight * 1.5f * Zoom };
		SDL_RenderTexture(Context->MainRenderer, Image->Texture, nullptr, &Target);
	}

	void DrawMapSprite(Game::Render::AppRenderContext* Context, Game::Render::Image* Image,
		const SDL_FPoint& Center, float Zoom, float Scale)
	{
		if (Image == nullptr || Image->Texture == nullptr)
			return;
		float TextureWidth = 0.0f;
		float TextureHeight = 0.0f;
		SDL_GetTextureSize(Image->Texture, &TextureWidth, &TextureHeight);
		const float SpriteScale = Zoom * Scale;
		const float Width = TextureWidth * SpriteScale;
		const float Height = TextureHeight * SpriteScale;
		const SDL_FRect Target{ Center.x - Width * 0.5f,
			Center.y + IsometricTileHeight * 0.5f * Zoom - Height, Width, Height };
		SDL_RenderTexture(Context->MainRenderer, Image->Texture, nullptr, &Target);
	}

	Game::Assets::ImageAssetId GetPlotSpriteAsset(const Game::Career::CityPlot& Plot)
	{
		if (Plot.Type == Game::Career::CityPlotType::Scenery)
			return IsometricCityAssets[Plot.SpriteIndex % IsometricCityAssets.size()];

		std::size_t TypeOffset = 0;
		switch (Plot.Type)
		{
		case Game::Career::CityPlotType::Gym:
			TypeOffset = 0;
			break;
		case Game::Career::CityPlotType::RaceTrack:
			TypeOffset = 2;
			break;
		case Game::Career::CityPlotType::Bookies:
			TypeOffset = 4;
			break;
		case Game::Career::CityPlotType::LicenseOffice:
			TypeOffset = 6;
			break;
		case Game::Career::CityPlotType::BreedingFarm:
			TypeOffset = 8;
			break;
		case Game::Career::CityPlotType::Stable:
			TypeOffset = 10;
			break;
		default:
			break;
		}
		return IsometricBuildingAssets[(TypeOffset + Plot.SpriteIndex) % IsometricBuildingAssets.size()];
	}

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

	std::string GetPlotTypeLabel(Game::Career::CityPlotType Type)
	{
		switch (Type)
		{
		case Game::Career::CityPlotType::Gym:
			return "TRAINING GYM";
		case Game::Career::CityPlotType::RaceTrack:
			return "RACE TRACK";
		case Game::Career::CityPlotType::Bookies:
			return "BOOKMAKER";
		case Game::Career::CityPlotType::LicenseOffice:
			return "RACING LICENSE OFFICE";
		case Game::Career::CityPlotType::BreedingFarm:
			return "BREEDING CLINIC";
		case Game::Career::CityPlotType::Stable:
			return "STABLE";
		case Game::Career::CityPlotType::VacantLot:
			return "VACANT LOT";
		default:
			return "CITY SCENERY";
		}
	}

	std::string GetPlotTooltipDetails(const Game::Career::CareerData& Data,
		const Game::Career::CityPlot& Plot, std::size_t PlotIndex)
	{
		std::string Details;
		switch (Plot.Type)
		{
		case Game::Career::CityPlotType::Gym:
			Details = "Training and recovery for racers";
			break;
		case Game::Career::CityPlotType::RaceTrack:
			Details = "Track length " + std::to_string(Plot.TrackLength / 50) + "m";
			break;
		case Game::Career::CityPlotType::Bookies:
			Details = "Place bets on upcoming races";
			break;
		case Game::Career::CityPlotType::LicenseOffice:
			Details = "Apply for a racing license";
			break;
		case Game::Career::CityPlotType::BreedingFarm:
			Details = "Manage breeding";
			break;
		case Game::Career::CityPlotType::Stable:
			Details = "View and manage your racer";
			break;
		case Game::Career::CityPlotType::VacantLot:
			return "Available for future development";
		default:
			return "Decorative city scenery";
		}
		Details += "  |  " + GetPlotActivityStatus(Data, PlotIndex) + "  |  Click to enter";
		return Details;
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
	InitializeTraffic();
}

void Game::Screens::WorldMapScreen::Entry()
{
	if (CityCameraInitialized)
		return;

	const CityWorldBounds Bounds = GetCityWorldBounds(Data.Career);
	CityCameraX = (Bounds.MinX + Bounds.MaxX) * 0.5f;
	CityCameraY = (Bounds.MinY + Bounds.MaxY) * 0.5f;
	CityCameraInitialized = true;
	LastCityInputUpdateMs = SDL_GetTicks();
	ClampCityCamera();
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
	UpdateCityCamera();
	UpdateCityTraffic();

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

void Game::Screens::WorldMapScreen::InitializeTraffic()
{
	unsigned int MaximumGridRow = 0;
	if (Data.Career != nullptr)
		for (const Career::CityPlot& Plot : Data.Career->CityPlots)
			MaximumGridRow = std::max(MaximumGridRow, Plot.GridY);
	const unsigned int RouteRowCount = std::max(1u, MaximumGridRow);
	std::mt19937 Random(static_cast<std::mt19937::result_type>(SDL_GetTicks()));
	std::uniform_int_distribution<unsigned int> RouteRow(0, RouteRowCount - 1);
	std::uniform_int_distribution<unsigned int> SpriteIndex(0,
		static_cast<unsigned int>(IsometricVehicleAssets.size() - 1));
	std::uniform_int_distribution<unsigned int> HonkInterval(45000, 105000);
	std::uniform_real_distribution<float> RouteProgress(0.0f, 1.0f);
	std::uniform_real_distribution<float> Speed(20.0f, 34.0f);
	const std::uint64_t NowMs = SDL_GetTicks();
	for (CityTrafficCar& Car : CityTraffic)
	{
		Car.RouteRow = RouteRow(Random);
		Car.SpriteIndex = SpriteIndex(Random);
		Car.RouteProgress = RouteProgress(Random);
		Car.Speed = Speed(Random);
		Car.HonkIntervalMs = HonkInterval(Random);
		Car.NextHonkAtMs = NowMs + Car.HonkIntervalMs;
	}
}

void Game::Screens::WorldMapScreen::UpdateCityCamera()
{
	if (!CityCameraInitialized)
		Entry();

	const std::uint64_t NowMs = SDL_GetTicks();
	const float DeltaSeconds = LastCityInputUpdateMs == 0 ? 0.0f :
		std::min(static_cast<float>(NowMs - LastCityInputUpdateMs) / 1000.0f, 0.1f);
	LastCityInputUpdateMs = NowMs;
	const SDL_Rect MapBounds = GetCityMapViewportBounds();
	const SDL_Point MousePosition{ static_cast<int>(IO.MousePosition.x),
		static_cast<int>(IO.MousePosition.y) };
	const bool MouseOverMap = SDL_PointInRect(&MousePosition, &MapBounds);
	if (MouseOverMap && IO.MouseWheelY != 0.0f)
	{
		const float CenterX = CityMapLeft + CityMapWidth * 0.5f;
		const float CenterY = CityMapTop + CityMapHeight * 0.5f;
		const float AnchorX = CityCameraX + (IO.MousePosition.x - CenterX) / CityZoom;
		const float AnchorY = CityCameraY + (IO.MousePosition.y - CenterY) / CityZoom;
		CityZoom = std::clamp(CityZoom * std::pow(1.12f, IO.MouseWheelY),
			MinimumCityZoom, MaximumCityZoom);
		CityCameraX = AnchorX - (IO.MousePosition.x - CenterX) / CityZoom;
		CityCameraY = AnchorY - (IO.MousePosition.y - CenterY) / CityZoom;
	}

	if (MouseOverMap && DeltaSeconds > 0.0f)
	{
		constexpr float EdgeSize = 24.0f;
		constexpr float CameraSpeed = 360.0f;
		float ScrollX = 0.0f;
		float ScrollY = 0.0f;
		if (IO.MousePosition.x < CityMapLeft + EdgeSize)
			ScrollX = -1.0f;
		else if (IO.MousePosition.x > CityMapLeft + CityMapWidth - EdgeSize)
			ScrollX = 1.0f;
		if (IO.MousePosition.y < CityMapTop + EdgeSize)
			ScrollY = -1.0f;
		else if (IO.MousePosition.y > CityMapTop + CityMapHeight - EdgeSize)
			ScrollY = 1.0f;
		CityCameraX += ScrollX * CameraSpeed * DeltaSeconds / CityZoom;
		CityCameraY += ScrollY * CameraSpeed * DeltaSeconds / CityZoom;
	}
	ClampCityCamera();
}

void Game::Screens::WorldMapScreen::ClampCityCamera()
{
	const CityWorldBounds Bounds = GetCityWorldBounds(Data.Career);
	const float HalfViewWidth = CityMapWidth / (2.0f * CityZoom);
	const float HalfViewHeight = CityMapHeight / (2.0f * CityZoom);
	if (Bounds.MaxX - Bounds.MinX <= HalfViewWidth * 2.0f)
		CityCameraX = (Bounds.MinX + Bounds.MaxX) * 0.5f;
	else
		CityCameraX = std::clamp(CityCameraX, Bounds.MinX + HalfViewWidth, Bounds.MaxX - HalfViewWidth);
	if (Bounds.MaxY - Bounds.MinY <= HalfViewHeight * 2.0f)
		CityCameraY = (Bounds.MinY + Bounds.MaxY) * 0.5f;
	else
		CityCameraY = std::clamp(CityCameraY, Bounds.MinY + HalfViewHeight, Bounds.MaxY - HalfViewHeight);
}

void Game::Screens::WorldMapScreen::UpdateCityTraffic()
{
	if (Data.Career == nullptr)
		return;

	const std::uint64_t NowMs = SDL_GetTicks();
	for (CityTrafficCar& Car : CityTraffic)
		if (NowMs >= Car.NextHonkAtMs)
		{
			IO.Player.Play(Game::Audio::BuiltInSounds::CarHorn);
			Car.NextHonkAtMs = NowMs + Car.HonkIntervalMs;
		}
}

int Game::Screens::WorldMapScreen::GetPlotAt(float X, float Y) const
{
	if (Data.Career == nullptr)
		return -1;
	const SDL_Rect MapBounds = GetCityMapViewportBounds();
	const SDL_Point MousePosition{ static_cast<int>(X), static_cast<int>(Y) };
	if (!SDL_PointInRect(&MousePosition, &MapBounds))
		return -1;

	const SDL_FPoint GridPosition = UnprojectCityScreenToGrid(X, Y, CityCameraX, CityCameraY,
		CityZoom);
	int ClosestPlotIndex = -1;
	float ClosestDistance = std::numeric_limits<float>::max();
	for (std::size_t Index = 0; Index < Data.Career->CityPlots.size(); ++Index)
	{
		const Career::CityPlot& Plot = Data.Career->CityPlots[Index];
		const SDL_FPoint Center = ProjectCityToScreen(ProjectCityGrid(static_cast<float>(Plot.GridX),
			static_cast<float>(Plot.GridY)), CityCameraX, CityCameraY, CityZoom);
		const SDL_FRect SpriteBounds{ Center.x - 66.0f * CityZoom,
			Center.y - 96.0f * CityZoom, 132.0f * CityZoom, 128.0f * CityZoom };
		const bool IsOverGroundTile = std::abs(GridPosition.x - static_cast<float>(Plot.GridX)) +
			std::abs(GridPosition.y - static_cast<float>(Plot.GridY)) <= 1.0f;
		const bool IsOverSprite = X >= SpriteBounds.x && X <= SpriteBounds.x + SpriteBounds.w &&
			Y >= SpriteBounds.y && Y <= SpriteBounds.y + SpriteBounds.h;
		if (!IsOverGroundTile && !IsOverSprite)
			continue;

		const float DeltaX = Center.x - X;
		const float DeltaY = Center.y - Y;
		const float Distance = DeltaX * DeltaX + DeltaY * DeltaY;
		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			ClosestPlotIndex = static_cast<int>(Index);
		}
	}
	return ClosestPlotIndex;
}

const Game::Career::CityPlot* Game::Screens::WorldMapScreen::GetInfoPlot() const
{
	if (Data.Career == nullptr || HoveredPlotIndex < 0 ||
		static_cast<std::size_t>(HoveredPlotIndex) >= Data.Career->CityPlots.size())
		return nullptr;
	return &Data.Career->CityPlots[static_cast<std::size_t>(HoveredPlotIndex)];
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
	RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
		"MOUSE WHEEL: ZOOM   |   MOVE TO A MAP EDGE: PAN   |   HOVER TO INSPECT, CLICK TO ENTER",
		36, 96, CityMutedTextColor);
	const SDL_FRect MapBounds{ CityMapLeft, CityMapTop, CityMapWidth, CityMapHeight };
	SDL_SetRenderDrawColor(Context->MainRenderer, 35, 44, 39, 255);
	SDL_RenderFillRect(Context->MainRenderer, &MapBounds);
	const SDL_Rect MapClip = GetCityMapViewportBounds();
	SDL_SetRenderClipRect(Context->MainRenderer, &MapClip);

	std::vector<std::size_t> PlotOrder(CareerData.CityPlots.size());
	unsigned int MaximumGridX = 0;
	unsigned int MaximumGridY = 0;
	for (std::size_t Index = 0; Index < CareerData.CityPlots.size(); ++Index)
	{
		PlotOrder[Index] = Index;
		MaximumGridX = std::max(MaximumGridX, CareerData.CityPlots[Index].GridX);
		MaximumGridY = std::max(MaximumGridY, CareerData.CityPlots[Index].GridY);
	}
	std::sort(PlotOrder.begin(), PlotOrder.end(), [&CareerData](std::size_t Left, std::size_t Right)
		{
			const Career::CityPlot& LeftPlot = CareerData.CityPlots[Left];
			const Career::CityPlot& RightPlot = CareerData.CityPlots[Right];
			return LeftPlot.GridX + LeftPlot.GridY < RightPlot.GridX + RightPlot.GridY;
		});

	for (const std::size_t Index : PlotOrder)
	{
		const Career::CityPlot& Plot = CareerData.CityPlots[Index];
		const SDL_FPoint Center = ProjectCityToScreen(ProjectCityGrid(static_cast<float>(Plot.GridX),
			static_cast<float>(Plot.GridY)), Screen->CityCameraX, Screen->CityCameraY, Screen->CityZoom);
		DrawFilledDiamond(Context->MainRenderer, Center, IsometricTileWidth * 0.45f * Screen->CityZoom,
			IsometricTileHeight * 0.45f * Screen->CityZoom, GetPlotGroundColor(Plot.Type));
		const std::size_t LandscapeIndex = (Plot.GridX + Plot.GridY + Plot.SpriteIndex) %
			IsometricLandscapeAssets.size();
		DrawLandscapeTile(Context, Context->Store.GetImage(IsometricLandscapeAssets[LandscapeIndex]),
			Center, Screen->CityZoom);
	}

	const SDL_Color RoadShoulder{ 130, 116, 88, 255 };
	const SDL_Color RoadColor{ 54, 59, 57, 255 };
	const SDL_Color RoadMarking{ 168, 155, 119, 255 };
	for (float RoadRow = -0.5f; RoadRow <= static_cast<float>(MaximumGridY) + 0.5f; RoadRow += 1.0f)
	{
		const SDL_FPoint Start = ProjectCityToScreen(ProjectCityGrid(-0.5f, RoadRow),
			Screen->CityCameraX, Screen->CityCameraY, Screen->CityZoom);
		const SDL_FPoint End = ProjectCityToScreen(ProjectCityGrid(static_cast<float>(MaximumGridX) + 0.5f,
			RoadRow), Screen->CityCameraX, Screen->CityCameraY, Screen->CityZoom);
		DrawThickLine(Context->MainRenderer, Start, End, 14.0f * Screen->CityZoom, RoadShoulder);
		DrawThickLine(Context->MainRenderer, Start, End, 10.0f * Screen->CityZoom, RoadColor);
		DrawThickLine(Context->MainRenderer, Start, End, 1.0f * Screen->CityZoom, RoadMarking);
	}
	for (float RoadColumn = -0.5f; RoadColumn <= static_cast<float>(MaximumGridX) + 0.5f;
		RoadColumn += 1.0f)
	{
		const SDL_FPoint Start = ProjectCityToScreen(ProjectCityGrid(RoadColumn, -0.5f),
			Screen->CityCameraX, Screen->CityCameraY, Screen->CityZoom);
		const SDL_FPoint End = ProjectCityToScreen(ProjectCityGrid(RoadColumn,
			static_cast<float>(MaximumGridY) + 0.5f), Screen->CityCameraX, Screen->CityCameraY,
			Screen->CityZoom);
		DrawThickLine(Context->MainRenderer, Start, End, 14.0f * Screen->CityZoom, RoadShoulder);
		DrawThickLine(Context->MainRenderer, Start, End, 10.0f * Screen->CityZoom, RoadColor);
		DrawThickLine(Context->MainRenderer, Start, End, 1.0f * Screen->CityZoom, RoadMarking);
	}

	for (const std::size_t Index : PlotOrder)
	{
		const Career::CityPlot& Plot = CareerData.CityPlots[Index];
		const SDL_FPoint Center = ProjectCityToScreen(ProjectCityGrid(static_cast<float>(Plot.GridX),
			static_cast<float>(Plot.GridY)), Screen->CityCameraX, Screen->CityCameraY, Screen->CityZoom);
		const bool IsHovered = Screen->GetHoveredPlotIndex() == static_cast<int>(Index);
		if (Plot.Type == Career::CityPlotType::VacantLot)
		{
			const SDL_FRect SignPost{ Center.x - 1.5f * Screen->CityZoom,
				Center.y - 7.0f * Screen->CityZoom, 3.0f * Screen->CityZoom, 24.0f * Screen->CityZoom };
			const SDL_FRect Sign{ Center.x - 17.0f * Screen->CityZoom,
				Center.y - 42.0f * Screen->CityZoom, 34.0f * Screen->CityZoom, 14.0f * Screen->CityZoom };
			SDL_SetRenderDrawColor(Context->MainRenderer, 78, 56, 38, 255);
			SDL_RenderFillRect(Context->MainRenderer, &SignPost);
			SDL_SetRenderDrawColor(Context->MainRenderer, 197, 149, 70, 255);
			SDL_RenderFillRect(Context->MainRenderer, &Sign);
			if (Screen->CityZoom >= 0.7f)
				RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), "LOT",
					static_cast<int>(Sign.x + 7.0f * Screen->CityZoom),
					static_cast<int>(Sign.y + 1.0f * Screen->CityZoom), SDL_Color{ 35, 33, 29, 255 });
		}
		else
		{
			const Assets::ImageAssetId SpriteAsset = GetPlotSpriteAsset(Plot);
			const float SpriteScale = Plot.Type == Career::CityPlotType::Scenery ? 0.78f : 0.96f;
			DrawMapSprite(Context, Context->Store.GetImage(SpriteAsset), Center, Screen->CityZoom,
				SpriteScale);
			if (Plot.Type != Career::CityPlotType::Scenery)
			{
				const SDL_Color Accent = GetPlotAccent(Plot.Type);
				const SDL_FRect Marker{ Center.x - 4.0f * Screen->CityZoom,
					Center.y + 16.0f * Screen->CityZoom, 8.0f * Screen->CityZoom,
					8.0f * Screen->CityZoom };
				SDL_SetRenderDrawColor(Context->MainRenderer, Accent.r, Accent.g, Accent.b, Accent.a);
				SDL_RenderFillRect(Context->MainRenderer, &Marker);
			}
		}
		if (IsHovered)
			DrawDiamondOutline(Context->MainRenderer, Center, IsometricTileWidth * 0.49f * Screen->CityZoom,
				IsometricTileHeight * 0.49f * Screen->CityZoom, SDL_Color{ 246, 220, 126, 255 });
	}

	const float MaximumX = static_cast<float>(MaximumGridX);
	const float ElapsedSeconds = static_cast<float>(SDL_GetTicks()) / 1000.0f;
	for (const Screens::WorldMapScreen::CityTrafficCar& Car : Screen->CityTraffic)
	{
		const float Top = static_cast<float>(Car.RouteRow) + 0.5f;
		const std::array<SDL_FPoint, 4> RouteGrid{
			SDL_FPoint{ -0.5f, Top },
			SDL_FPoint{ MaximumX + 0.5f, Top },
			SDL_FPoint{ MaximumX + 0.5f, Top + 1.0f },
			SDL_FPoint{ -0.5f, Top + 1.0f }
		};
		std::array<SDL_FPoint, 4> RouteWorld{};
		std::array<float, 4> SegmentLengths{};
		float RouteLength = 0.0f;
		for (std::size_t Index = 0; Index < RouteGrid.size(); ++Index)
		{
			RouteWorld[Index] = ProjectCityGrid(RouteGrid[Index].x, RouteGrid[Index].y);
			const SDL_FPoint& Start = RouteWorld[Index];
			const SDL_FPoint& End = ProjectCityGrid(RouteGrid[(Index + 1) % RouteGrid.size()].x,
				RouteGrid[(Index + 1) % RouteGrid.size()].y);
			SegmentLengths[Index] = std::hypot(End.x - Start.x, End.y - Start.y);
			RouteLength += SegmentLengths[Index];
		}
		if (RouteLength <= 0.0f)
			continue;

		float RouteDistance = std::fmod(Car.RouteProgress * RouteLength + ElapsedSeconds * Car.Speed,
			RouteLength);
		SDL_FPoint Position{};
		float DirectionX = 0.0f;
		float DirectionY = 0.0f;
		for (std::size_t Index = 0; Index < RouteWorld.size(); ++Index)
		{
			const SDL_FPoint& Start = RouteWorld[Index];
			const SDL_FPoint& End = RouteWorld[(Index + 1) % RouteWorld.size()];
			if (RouteDistance <= SegmentLengths[Index] || Index + 1 == RouteWorld.size())
			{
				const float SegmentProgress = SegmentLengths[Index] > 0.0f ?
					std::clamp(RouteDistance / SegmentLengths[Index], 0.0f, 1.0f) : 0.0f;
				Position = { Start.x + (End.x - Start.x) * SegmentProgress,
					Start.y + (End.y - Start.y) * SegmentProgress };
				DirectionX = End.x - Start.x;
				DirectionY = End.y - Start.y;
				break;
			}
			RouteDistance -= SegmentLengths[Index];
		}
		Game::Render::Image* Vehicle = Context->Store.GetImage(
			IsometricVehicleAssets[Car.SpriteIndex % IsometricVehicleAssets.size()]);
		if (Vehicle == nullptr || Vehicle->Texture == nullptr)
			continue;
		const SDL_FPoint CarCenter = ProjectCityToScreen(Position, Screen->CityCameraX,
			Screen->CityCameraY, Screen->CityZoom);
		const float Angle = std::atan2(DirectionY, DirectionX) * 180.0f / 3.14159265358979323846f;
		float VehicleWidth = 0.0f;
		float VehicleHeight = 0.0f;
		SDL_GetTextureSize(Vehicle->Texture, &VehicleWidth, &VehicleHeight);
		const float CarScale = Screen->CityZoom * 1.1f;
		const SDL_FRect CarBounds{ CarCenter.x - VehicleWidth * CarScale * 0.5f,
			CarCenter.y - VehicleHeight * CarScale * 0.5f,
			VehicleWidth * CarScale, VehicleHeight * CarScale };
		SDL_RenderTextureRotated(Context->MainRenderer, Vehicle->Texture, nullptr, &CarBounds,
			Angle, nullptr, SDL_FLIP_NONE);
	}

	SDL_SetRenderClipRect(Context->MainRenderer, nullptr);
	SDL_SetRenderDrawColor(Context->MainRenderer, 173, 165, 137, 255);
	SDL_RenderRect(Context->MainRenderer, &MapBounds);

	if (const Career::CityPlot* InfoPlot = Screen->GetInfoPlot())
	{
		const std::size_t PlotIndex = static_cast<std::size_t>(Screen->GetHoveredPlotIndex());
		const SDL_FRect InfoBounds{ CityMapLeft, 624.0f, CityMapWidth, 52.0f };
		SDL_SetRenderDrawColor(Context->MainRenderer, 34, 33, 30, 255);
		SDL_RenderFillRect(Context->MainRenderer, &InfoBounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, 125, 111, 88, 255);
		SDL_RenderRect(Context->MainRenderer, &InfoBounds);
		RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont),
			InfoPlot->Name + "  |  " + GetPlotTypeLabel(InfoPlot->Type), 50, 630, CityTextColor);
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
			GetPlotTooltipDetails(CareerData, *InfoPlot, PlotIndex), 50, 651, CityMutedTextColor);
	}

	DrawCityButton(*this, Context, Screen->PopulationButton, true);
	DrawCityButton(*this, Context, Screen->LeagueBrowserButton, true);
	DrawCityButton(*this, Context, Screen->CareerHubButton, true);
	Display();
	return SDL_GetTicks() - StartTime;
}
