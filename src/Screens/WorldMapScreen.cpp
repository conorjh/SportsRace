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
#include <utility>
#include <vector>

namespace WorldMapScreenDetail
{
	constexpr float CityMapLeft = 36.0f;
	constexpr float CityMapTop = 112.0f;
	constexpr float CityMapWidth = 952.0f;
	constexpr float CityMapHeight = 492.0f;
	constexpr float IsometricTileWidth = 128.0f;
	constexpr float IsometricTileHeight = 64.0f;
	constexpr float MinimumCityZoom = 0.55f;
	constexpr float MaximumCityZoom = 1.65f;
	const std::vector<Game::Career::CityPlot> EmptyCityPlots;
	constexpr SDL_Color CityBackgroundColor{ 22, 28, 29, 255 };
	constexpr SDL_Color CityTextColor{ 239, 231, 214, 255 };
	constexpr SDL_Color CityMutedTextColor{ 174, 166, 147, 255 };
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

	struct CityTrafficSprite
	{
		Game::Render::Image* Image;
		SDL_FPoint Position;
		float Angle;
		SDL_FlipMode Flip;
		float Scale;
		float Width;
		float Height;
		float Depth;
	};

	struct CitySceneItem
	{
		float Depth;
		bool IsTraffic;
		std::size_t Index;
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

	CityWorldBounds GetCityWorldBounds(const std::vector<Game::Career::CityPlot>& CityPlots)
	{
		if (CityPlots.empty())
			return { -IsometricTileWidth, IsometricTileWidth, -IsometricTileHeight * 2.0f,
				IsometricTileHeight * 2.0f };

		CityWorldBounds Bounds{
			std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()
		};
		for (const Game::Career::CityPlot& Plot : CityPlots)
		{
			const SDL_FPoint Position = ProjectCityGrid(static_cast<float>(Plot.GridX),
				static_cast<float>(Plot.GridY));
			Bounds.MinX = std::min(Bounds.MinX, Position.x - 72.0f);
			Bounds.MaxX = std::max(Bounds.MaxX, Position.x + 72.0f);
			Bounds.MinY = std::min(Bounds.MinY, Position.y - 96.0f);
			Bounds.MaxY = std::max(Bounds.MaxY, Position.y + 48.0f);
		}
		return Bounds;
	}

	std::vector<std::size_t> GetCityPlotDrawOrder(const std::vector<Game::Career::CityPlot>& CityPlots)
	{
		std::vector<std::size_t> PlotOrder(CityPlots.size());
		for (std::size_t Index = 0; Index < PlotOrder.size(); ++Index)
			PlotOrder[Index] = Index;
		std::sort(PlotOrder.begin(), PlotOrder.end(), [&CityPlots](std::size_t Left, std::size_t Right)
			{
				const Game::Career::CityPlot& LeftPlot = CityPlots[Left];
				const Game::Career::CityPlot& RightPlot = CityPlots[Right];
				const unsigned int LeftDepth = LeftPlot.GridX + LeftPlot.GridY;
				const unsigned int RightDepth = RightPlot.GridX + RightPlot.GridY;
				if (LeftDepth != RightDepth)
					return LeftDepth < RightDepth;
				if (LeftPlot.GridX != RightPlot.GridX)
					return LeftPlot.GridX < RightPlot.GridX;
				return Left < Right;
			});
		return PlotOrder;
	}

	const Game::Career::CareerWorldCity* FindCity(const Game::Career::CareerData& Data,
		std::uint64_t CityId)
	{
		const auto City = std::find_if(Data.Cities.begin(), Data.Cities.end(),
			[CityId](const Game::Career::CareerWorldCity& Candidate) { return Candidate.Id == CityId; });
		return City == Data.Cities.end() ? nullptr : &*City;
	}

	const Game::Career::CareerWorldRegion* FindRegion(const Game::Career::CareerData& Data,
		std::uint64_t RegionId)
	{
		const auto Region = std::find_if(Data.Regions.begin(), Data.Regions.end(),
			[RegionId](const Game::Career::CareerWorldRegion& Candidate) { return Candidate.Id == RegionId; });
		return Region == Data.Regions.end() ? nullptr : &*Region;
	}

	const Game::Career::CareerWorldCountry* FindCountry(const Game::Career::CareerData& Data,
		std::uint64_t CountryId)
	{
		const auto Country = std::find_if(Data.Countries.begin(), Data.Countries.end(),
			[CountryId](const Game::Career::CareerWorldCountry& Candidate) { return Candidate.Id == CountryId; });
		return Country == Data.Countries.end() ? nullptr : &*Country;
	}

	SDL_FPoint GetOverviewMapPosition(float MapX, float MapY)
	{
		constexpr float OverviewMapInsetX = 84.0f;
		constexpr float OverviewMapInsetY = 58.0f;
		return { CityMapLeft + OverviewMapInsetX + MapX * (CityMapWidth - OverviewMapInsetX * 2.0f),
			CityMapTop + OverviewMapInsetY + MapY * (CityMapHeight - OverviewMapInsetY * 2.0f) };
	}

	std::string FormatPopulation(std::uint64_t Population)
	{
		std::string Text = std::to_string(Population);
		for (std::size_t Position = Text.size(); Position > 3; Position -= 3)
			Text.insert(Position - 3, ",");
		return Text;
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
			return { 191, 179, 147, 255 };
		case Game::Career::CityPlotType::Scenery:
			return { 92, 112, 78, 255 };
		default:
			return { 191, 179, 147, 255 };
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

	void DrawFilledEllipse(SDL_Renderer* Renderer, const SDL_FPoint& Center, float RadiusX,
		float RadiusY, SDL_Color Color)
	{
		constexpr std::size_t SegmentCount = 24;
		const SDL_FColor VertexColor{ static_cast<float>(Color.r) / 255.0f,
			static_cast<float>(Color.g) / 255.0f, static_cast<float>(Color.b) / 255.0f,
			static_cast<float>(Color.a) / 255.0f };
		std::array<SDL_Vertex, SegmentCount + 1> Vertices{};
		std::array<int, SegmentCount * 3> Indices{};
		Vertices[0] = { { Center.x, Center.y }, VertexColor, { 0.0f, 0.0f } };
		for (std::size_t Index = 0; Index < SegmentCount; ++Index)
		{
			const float Angle = 2.0f * 3.14159265358979323846f * static_cast<float>(Index) /
				static_cast<float>(SegmentCount);
			Vertices[Index + 1] = { { Center.x + std::cos(Angle) * RadiusX,
				Center.y + std::sin(Angle) * RadiusY }, VertexColor, { 0.0f, 0.0f } };
			Indices[Index * 3] = 0;
			Indices[Index * 3 + 1] = static_cast<int>(Index + 1);
			Indices[Index * 3 + 2] = static_cast<int>((Index + 1) % SegmentCount + 1);
		}
		SDL_RenderGeometry(Renderer, nullptr, Vertices.data(), static_cast<int>(Vertices.size()),
			Indices.data(), static_cast<int>(Indices.size()));
	}

	void DrawEllipseRing(SDL_Renderer* Renderer, const SDL_FPoint& Center, float OuterRadiusX,
		float OuterRadiusY, float InnerRadiusX, float InnerRadiusY, SDL_Color Color)
	{
		constexpr std::size_t SegmentCount = 24;
		const SDL_FColor VertexColor{ static_cast<float>(Color.r) / 255.0f,
			static_cast<float>(Color.g) / 255.0f, static_cast<float>(Color.b) / 255.0f,
			static_cast<float>(Color.a) / 255.0f };
		std::array<SDL_Vertex, SegmentCount * 2> Vertices{};
		std::array<int, SegmentCount * 6> Indices{};
		for (std::size_t Index = 0; Index < SegmentCount; ++Index)
		{
			const float Angle = 2.0f * 3.14159265358979323846f * static_cast<float>(Index) /
				static_cast<float>(SegmentCount);
			const std::size_t OuterIndex = Index * 2;
			Vertices[OuterIndex] = { { Center.x + std::cos(Angle) * OuterRadiusX,
				Center.y + std::sin(Angle) * OuterRadiusY }, VertexColor, { 0.0f, 0.0f } };
			Vertices[OuterIndex + 1] = { { Center.x + std::cos(Angle) * InnerRadiusX,
				Center.y + std::sin(Angle) * InnerRadiusY }, VertexColor, { 0.0f, 0.0f } };

			const std::size_t NextOuterIndex = ((Index + 1) % SegmentCount) * 2;
			const std::size_t IndexOffset = Index * 6;
			Indices[IndexOffset] = static_cast<int>(OuterIndex);
			Indices[IndexOffset + 1] = static_cast<int>(NextOuterIndex);
			Indices[IndexOffset + 2] = static_cast<int>(OuterIndex + 1);
			Indices[IndexOffset + 3] = static_cast<int>(NextOuterIndex);
			Indices[IndexOffset + 4] = static_cast<int>(NextOuterIndex + 1);
			Indices[IndexOffset + 5] = static_cast<int>(OuterIndex + 1);
		}
		SDL_RenderGeometry(Renderer, nullptr, Vertices.data(), static_cast<int>(Vertices.size()),
			Indices.data(), static_cast<int>(Indices.size()));
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
using namespace WorldMapScreenDetail;

Game::Screens::WorldMapScreen::WorldMapScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO,
	App::AppData& _Data)
	: AppScreen(_Machine, _IO, _Data),
	Orchestrator(_Data.Career, _Data.Profile),
	PopulationButton(_IO, "Population database", 36, 690, 220, 48),
	CareerHubButton(_IO, "Career hub", 824, 690, 160, 48),
	LeagueBrowserButton(_IO, "League browser", 280, 690, 220, 48),
	ZoomOutButton(_IO, "Zoom out", 520, 690, 130, 48),
	ZoomInButton(_IO, "Zoom in", 666, 690, 130, 48)
{
	Type = AppScreenType::WorldMapScreen;
	Orchestrator.Init();
	if (Data.Career != nullptr)
	{
		SelectedCityId = Data.Career->HomeCityId;
		if (const Career::CareerWorldCity* City = FindCity(*Data.Career, SelectedCityId))
		{
			SelectedRegionId = City->RegionId;
			if (const Career::CareerWorldRegion* Region = FindRegion(*Data.Career, SelectedRegionId))
				SelectedCountryId = Region->CountryId;
		}
	}
	InitializeTraffic();
}

void Game::Screens::WorldMapScreen::Entry()
{
	if (CityCameraInitialized)
		return;

	const CityWorldBounds Bounds = GetCityWorldBounds(GetCurrentCityPlots());
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
	ZoomOutButton.Update();
	ZoomInButton.Update();

	if (CareerHubButton.HasMouseClicked() || IO.Esc)
	{
		IO.Player.Play(BuiltInSounds::Click);
		ScreenStack.Pop();
		return ScreenStack.Top();
	}
	if (LeagueBrowserButton.HasMouseClicked())
	{
		IO.Player.Play(BuiltInSounds::Click);
		LeagueBrowserLocationContext LocationContext;
		LocationContext.CityId = SelectedCityId;
		LocationContext.RegionId = SelectedRegionId;
		LocationContext.CountryId = SelectedCountryId;
		switch (MapView)
		{
		case WorldMapViewLevel::City:
			LocationContext.Filter = LeagueBrowserLocationFilter::City;
			break;
		case WorldMapViewLevel::Region:
			LocationContext.Filter = LeagueBrowserLocationFilter::Region;
			break;
		case WorldMapViewLevel::Country:
			LocationContext.Filter = LeagueBrowserLocationFilter::Country;
			break;
		}
		ScreenStack.Push(new LeagueBrowserScreen(ScreenStack, IO, Data, LocationContext));
		return ScreenStack.Top();
	}
	if (ZoomOutButton.HasMouseClicked() && MapView != WorldMapViewLevel::Country)
	{
		IO.Player.Play(BuiltInSounds::Click);
		ZoomOut();
		return this;
	}
	if (ZoomInButton.HasMouseClicked() && MapView != WorldMapViewLevel::City)
	{
		IO.Player.Play(BuiltInSounds::Click);
		ZoomIn();
		return this;
	}

	if (Data.Career != nullptr && Data.Career->UpcomingRaceEntrants.empty())
		Orchestrator.PrepareUpcomingRace(SDL_GetTicks());
	UpdateCityCamera();
	if (MapView == WorldMapViewLevel::City)
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

	HoveredPlotIndex = MapView == WorldMapViewLevel::City
		? GetPlotAt(IO.MousePosition.x, IO.MousePosition.y) : -1;
	HoveredCityId = MapView == WorldMapViewLevel::Region
		? GetCityMarkerAt(IO.MousePosition.x, IO.MousePosition.y) : 0;
	HoveredRegionId = MapView == WorldMapViewLevel::Country
		? GetRegionMarkerAt(IO.MousePosition.x, IO.MousePosition.y) : 0;
	if ((IO.MouseButtonsPressed & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) != 0 && MapView == WorldMapViewLevel::Country &&
		HoveredRegionId != 0)
	{
		SelectedRegionId = HoveredRegionId;
		if (const Career::CareerWorldRegion* Region = FindRegion(*Data.Career, SelectedRegionId))
			SelectedCityId = Region->CapitalCityId;
		MapView = WorldMapViewLevel::Region;
		return this;
	}
	if ((IO.MouseButtonsPressed & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) != 0 && MapView == WorldMapViewLevel::Region &&
		HoveredCityId != 0)
	{
		SelectedCityId = HoveredCityId;
		MapView = WorldMapViewLevel::City;
		CityCameraInitialized = false;
		CityRoadNetworkInitialized = false;
		Entry();
		return this;
	}

	if (Data.Career != nullptr && MapView == WorldMapViewLevel::City &&
		SelectedCityId == Data.Career->HomeCityId &&
		(IO.MouseButtonsPressed & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) != 0 && HoveredPlotIndex >= 0)
	{
		const std::vector<Career::CityPlot>& CityPlots = GetCurrentCityPlots();
		if (static_cast<std::size_t>(HoveredPlotIndex) >= CityPlots.size())
			return this;
		const Career::CityPlot& Plot = CityPlots[static_cast<std::size_t>(HoveredPlotIndex)];
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

const std::vector<Game::Career::CityPlot>& Game::Screens::WorldMapScreen::GetCurrentCityPlots() const
{
	if (Data.Career == nullptr)
		return EmptyCityPlots;
	if (SelectedCityId == 0 || SelectedCityId == Data.Career->HomeCityId)
		return Data.Career->CityPlots;
	const Career::CareerWorldCity* City = FindCity(*Data.Career, SelectedCityId);
	return City == nullptr ? Data.Career->CityPlots : City->Plots;
}

void Game::Screens::WorldMapScreen::ZoomOut()
{
	if (Data.Career == nullptr)
		return;
	if (MapView == WorldMapViewLevel::City)
	{
		if (const Career::CareerWorldCity* City = FindCity(*Data.Career, SelectedCityId))
		{
			SelectedRegionId = City->RegionId;
			if (const Career::CareerWorldRegion* Region = FindRegion(*Data.Career, SelectedRegionId))
				SelectedCountryId = Region->CountryId;
		}
		MapView = WorldMapViewLevel::Region;
		HoveredPlotIndex = -1;
		return;
	}
	if (MapView == WorldMapViewLevel::Region)
		MapView = WorldMapViewLevel::Country;
}

void Game::Screens::WorldMapScreen::ZoomIn()
{
	if (Data.Career == nullptr)
		return;
	if (MapView == WorldMapViewLevel::Country)
	{
		const Career::CareerWorldCountry* Country = FindCountry(*Data.Career, SelectedCountryId);
		if (Country == nullptr && !Data.Career->Countries.empty())
			Country = &Data.Career->Countries.front();
		const Career::CareerWorldRegion* Region = FindRegion(*Data.Career, SelectedRegionId);
		if (Region == nullptr || (Country != nullptr && Region->CountryId != Country->Id))
			Region = Country == nullptr ? nullptr : FindRegion(*Data.Career, Country->CapitalRegionId);
		if (Region == nullptr)
			return;
		SelectedCountryId = Region->CountryId;
		SelectedRegionId = Region->Id;
		SelectedCityId = Region->CapitalCityId;
		MapView = WorldMapViewLevel::Region;
		return;
	}
	if (MapView == WorldMapViewLevel::Region)
	{
		const Career::CareerWorldRegion* Region = FindRegion(*Data.Career, SelectedRegionId);
		const Career::CareerWorldCity* City = FindCity(*Data.Career, SelectedCityId);
		if (Region == nullptr)
			return;
		if (City == nullptr || City->RegionId != Region->Id)
			SelectedCityId = Region->CapitalCityId;
		MapView = WorldMapViewLevel::City;
		CityCameraInitialized = false;
		CityRoadNetworkInitialized = false;
		Entry();
	}
}

std::uint64_t Game::Screens::WorldMapScreen::GetCityMarkerAt(float X, float Y) const
{
	if (Data.Career == nullptr)
		return 0;
	const Career::CareerWorldRegion* Region = FindRegion(*Data.Career, SelectedRegionId);
	if (Region == nullptr)
		return 0;
	for (const std::uint64_t CityId : Region->CityIds)
	{
		const Career::CareerWorldCity* City = FindCity(*Data.Career, CityId);
		if (City == nullptr)
			continue;
		const SDL_FPoint Position = GetOverviewMapPosition(City->MapX, City->MapY);
		if (std::hypot(Position.x - X, Position.y - Y) <= 24.0f)
			return City->Id;
	}
	return 0;
}

std::uint64_t Game::Screens::WorldMapScreen::GetRegionMarkerAt(float X, float Y) const
{
	if (Data.Career == nullptr)
		return 0;
	const Career::CareerWorldCountry* Country = FindCountry(*Data.Career, SelectedCountryId);
	if (Country == nullptr)
		return 0;
	for (const std::uint64_t RegionId : Country->RegionIds)
	{
		const Career::CareerWorldRegion* Region = FindRegion(*Data.Career, RegionId);
		if (Region == nullptr)
			continue;
		const SDL_FPoint Position = GetOverviewMapPosition(Region->MapX, Region->MapY);
		if (std::hypot(Position.x - X, Position.y - Y) <= 28.0f)
			return Region->Id;
	}
	return 0;
}

void Game::Screens::WorldMapScreen::InitializeTraffic()
{
	if (Data.Career == nullptr)
		return;

	const std::vector<Career::CityPlot>& CityPlots = GetCurrentCityPlots();
	unsigned int MaximumGridX = 0;
	unsigned int MaximumGridY = 0;
	std::uint32_t CitySeed = 2166136261u;
	for (const Career::CityPlot& Plot : CityPlots)
	{
		MaximumGridX = std::max(MaximumGridX, Plot.GridX);
		MaximumGridY = std::max(MaximumGridY, Plot.GridY);
		CitySeed = (CitySeed ^ Plot.GridX) * 16777619u;
		CitySeed = (CitySeed ^ Plot.GridY) * 16777619u;
		CitySeed = (CitySeed ^ Plot.SpriteIndex) * 16777619u;
	}
	std::mt19937 Random(CitySeed);
	const std::size_t NodeColumns = static_cast<std::size_t>(MaximumGridX) + 2;
	const std::size_t NodeRows = static_cast<std::size_t>(MaximumGridY) + 2;
	struct RoadEdge
	{
		std::size_t StartNode;
		std::size_t EndNode;
	};
	std::vector<RoadEdge> PotentialRoadEdges;
	const auto GetNodeIndex = [NodeColumns](std::size_t GridX, std::size_t GridY)
	{
		return GridY * NodeColumns + GridX;
	};
	for (std::size_t GridY = 0; GridY < NodeRows; ++GridY)
		for (std::size_t GridX = 0; GridX < NodeColumns; ++GridX)
		{
			const std::size_t Node = GetNodeIndex(GridX, GridY);
			if (GridX + 1 < NodeColumns)
			{
				const std::size_t Neighbor = GetNodeIndex(GridX + 1, GridY);
				PotentialRoadEdges.push_back({ Node, Neighbor });
			}
			if (GridY + 1 < NodeRows)
			{
				const std::size_t Neighbor = GetNodeIndex(GridX, GridY + 1);
				PotentialRoadEdges.push_back({ Node, Neighbor });
			}
		}

	std::uniform_int_distribution<std::size_t> BlockSizeDistribution(2, 3);
	const auto CreateStreetLines = [](std::size_t NodeLineCount, std::size_t BlockSize)
	{
		std::vector<std::size_t> Lines{ 0 };
		while (Lines.back() + BlockSize < NodeLineCount - 1)
			Lines.push_back(Lines.back() + BlockSize);
		if (Lines.back() != NodeLineCount - 1)
			Lines.push_back(NodeLineCount - 1);
		return Lines;
	};
	const std::vector<std::size_t> StreetColumns = CreateStreetLines(NodeColumns,
		BlockSizeDistribution(Random));
	const std::vector<std::size_t> StreetRows = CreateStreetLines(NodeRows,
		BlockSizeDistribution(Random));
	std::vector<bool> RoadEdges(PotentialRoadEdges.size(), false);
	for (std::size_t EdgeIndex = 0; EdgeIndex < PotentialRoadEdges.size(); ++EdgeIndex)
	{
		const RoadEdge& Edge = PotentialRoadEdges[EdgeIndex];
		const std::size_t StartGridX = Edge.StartNode % NodeColumns;
		const std::size_t StartGridY = Edge.StartNode / NodeColumns;
		const std::size_t EndGridY = Edge.EndNode / NodeColumns;
		const bool IsHorizontal = StartGridY == EndGridY;
		const std::vector<std::size_t>& StreetLines = IsHorizontal ? StreetRows : StreetColumns;
		const std::size_t StreetLine = IsHorizontal ? StartGridY : StartGridX;
		RoadEdges[EdgeIndex] = std::binary_search(StreetLines.begin(), StreetLines.end(), StreetLine);
	}

	const auto GetNodePosition = [](std::size_t GridX, std::size_t GridY)
	{
		return ProjectCityGrid(static_cast<float>(GridX) - 0.5f,
			static_cast<float>(GridY) - 0.5f);
	};
	CityRoadSegments.clear();
	for (std::size_t EdgeIndex = 0; EdgeIndex < PotentialRoadEdges.size(); ++EdgeIndex)
		if (RoadEdges[EdgeIndex])
		{
			const RoadEdge& Edge = PotentialRoadEdges[EdgeIndex];
			CityRoadSegments.push_back({
				GetNodePosition(Edge.StartNode % NodeColumns, Edge.StartNode / NodeColumns),
				GetNodePosition(Edge.EndNode % NodeColumns, Edge.EndNode / NodeColumns)
			});
		}

	std::vector<std::vector<SDL_FPoint>> CityRoadRoutes;
	for (std::size_t RowIndex = 0; RowIndex + 1 < StreetRows.size(); ++RowIndex)
		for (std::size_t ColumnIndex = 0; ColumnIndex + 1 < StreetColumns.size(); ++ColumnIndex)
		{
			const std::size_t TopRow = StreetRows[RowIndex];
			const std::size_t BottomRow = StreetRows[RowIndex + 1];
			const std::size_t LeftColumn = StreetColumns[ColumnIndex];
			const std::size_t RightColumn = StreetColumns[ColumnIndex + 1];
			std::vector<SDL_FPoint> Route;
			for (std::size_t GridX = LeftColumn; GridX <= RightColumn; ++GridX)
				Route.push_back(GetNodePosition(GridX, TopRow));
			for (std::size_t GridY = TopRow + 1; GridY <= BottomRow; ++GridY)
				Route.push_back(GetNodePosition(RightColumn, GridY));
			for (std::size_t GridX = RightColumn; GridX > LeftColumn; --GridX)
				Route.push_back(GetNodePosition(GridX - 1, BottomRow));
			for (std::size_t GridY = BottomRow; GridY > TopRow + 1; --GridY)
				Route.push_back(GetNodePosition(LeftColumn, GridY - 1));
			Route.push_back(Route.front());
			CityRoadRoutes.push_back(std::move(Route));
		}

	std::uniform_int_distribution<unsigned int> SpriteIndex(0,
		static_cast<unsigned int>(IsometricVehicleAssets.size() - 1));
	std::uniform_int_distribution<unsigned int> HonkInterval(45000, 105000);
	std::uniform_real_distribution<float> RouteProgress(0.0f, 1.0f);
	std::uniform_real_distribution<float> Speed(20.0f, 34.0f);
	std::uniform_int_distribution<std::size_t> RouteIndex(0, CityRoadRoutes.size() - 1);
	const std::uint64_t NowMs = SDL_GetTicks();
	for (CityTrafficCar& Car : CityTraffic)
	{
		Car.Route = CityRoadRoutes[RouteIndex(Random)];
		Car.SpriteIndex = SpriteIndex(Random);
		Car.RouteProgress = RouteProgress(Random);
		Car.Speed = Speed(Random);
		Car.HonkIntervalMs = HonkInterval(Random);
		Car.NextHonkAtMs = NowMs + Car.HonkIntervalMs;
	}
	CityRoadMaximumGridX = MaximumGridX;
	CityRoadMaximumGridY = MaximumGridY;
	CityRoadNetworkCityId = SelectedCityId;
	CityRoadNetworkInitialized = true;
}

void Game::Screens::WorldMapScreen::UpdateCityCamera()
{
	const std::uint64_t NowMs = SDL_GetTicks();
	if (MapView != WorldMapViewLevel::City)
	{
		const SDL_Rect MapBounds = GetCityMapViewportBounds();
		const SDL_Point MousePosition{ static_cast<int>(IO.MousePosition.x),
			static_cast<int>(IO.MousePosition.y) };
		if (SDL_PointInRect(&MousePosition, &MapBounds) && IO.MouseWheelY != 0.0f)
		{
			if (IO.MouseWheelY > 0.0f)
				ZoomIn();
			else
				ZoomOut();
		}
		LastCityInputUpdateMs = NowMs;
		return;
	}
	if (!CityCameraInitialized)
		Entry();

	const float DeltaSeconds = LastCityInputUpdateMs == 0 ? 0.0f :
		std::min(static_cast<float>(NowMs - LastCityInputUpdateMs) / 1000.0f, 0.1f);
	LastCityInputUpdateMs = NowMs;
	const SDL_Rect MapBounds = GetCityMapViewportBounds();
	const SDL_Point MousePosition{ static_cast<int>(IO.MousePosition.x),
		static_cast<int>(IO.MousePosition.y) };
	const bool MouseOverMap = SDL_PointInRect(&MousePosition, &MapBounds);
	if (MouseOverMap && IO.MouseWheelY != 0.0f)
	{
		if (IO.MouseWheelY < 0.0f && CityZoom <= MinimumCityZoom + 0.001f)
		{
			ZoomOut();
			return;
		}
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
	const CityWorldBounds Bounds = GetCityWorldBounds(GetCurrentCityPlots());
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
	const std::vector<Career::CityPlot>& CityPlots = GetCurrentCityPlots();
	unsigned int MaximumGridX = 0;
	unsigned int MaximumGridY = 0;
	for (const Career::CityPlot& Plot : CityPlots)
	{
		MaximumGridX = std::max(MaximumGridX, Plot.GridX);
		MaximumGridY = std::max(MaximumGridY, Plot.GridY);
	}
	if (!CityRoadNetworkInitialized || CityRoadNetworkCityId != SelectedCityId ||
		MaximumGridX != CityRoadMaximumGridX ||
		MaximumGridY != CityRoadMaximumGridY)
		InitializeTraffic();

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
	const std::vector<Career::CityPlot>& CityPlots = GetCurrentCityPlots();
	const std::vector<std::size_t> PlotOrder = GetCityPlotDrawOrder(CityPlots);
	for (auto Order = PlotOrder.rbegin(); Order != PlotOrder.rend(); ++Order)
	{
		const std::size_t Index = *Order;
		const Career::CityPlot& Plot = CityPlots[Index];
		const SDL_FPoint Center = ProjectCityToScreen(ProjectCityGrid(static_cast<float>(Plot.GridX),
			static_cast<float>(Plot.GridY)), CityCameraX, CityCameraY, CityZoom);
		const bool IsOverGroundTile = std::abs(GridPosition.x - static_cast<float>(Plot.GridX)) +
			std::abs(GridPosition.y - static_cast<float>(Plot.GridY)) <= 1.0f;
		const float SpriteScale = Plot.Type == Career::CityPlotType::Scenery ? 0.78f : 0.96f;
		const float SpriteWidth = IsometricTileWidth * SpriteScale * CityZoom;
		const float SpriteHeight = IsometricTileHeight * 2.0f * SpriteScale * CityZoom;
		const SDL_FRect SpriteBounds{ Center.x - SpriteWidth * 0.5f,
			Center.y + IsometricTileHeight * 0.5f * CityZoom - SpriteHeight,
			SpriteWidth, SpriteHeight };
		const bool IsOverSprite = Plot.Type == Career::CityPlotType::VacantLot
			? X >= Center.x - 17.0f * CityZoom && X <= Center.x + 17.0f * CityZoom &&
				Y >= Center.y - 42.0f * CityZoom && Y <= Center.y + 17.0f * CityZoom
			: X >= SpriteBounds.x && X <= SpriteBounds.x + SpriteBounds.w &&
				Y >= SpriteBounds.y && Y <= SpriteBounds.y + SpriteBounds.h;
		if (!IsOverGroundTile && !IsOverSprite)
			continue;
		return static_cast<int>(Index);
	}
	return -1;
}

const Game::Career::CityPlot* Game::Screens::WorldMapScreen::GetInfoPlot() const
{
	const std::vector<Career::CityPlot>& CityPlots = GetCurrentCityPlots();
	if (Data.Career == nullptr || HoveredPlotIndex < 0 ||
		static_cast<std::size_t>(HoveredPlotIndex) >= CityPlots.size())
		return nullptr;
	return &CityPlots[static_cast<std::size_t>(HoveredPlotIndex)];
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

	if (Screen == nullptr || Screen->Data.Career == nullptr)
	{
		RenderText(Context->Store.GetFont(Assets::FontAssetMainFont), "CAREER WORLD", 36, 22,
			CityTextColor);
		Display();
		return SDL_GetTicks() - StartTime;
	}

	const Career::CareerData& CareerData = *Screen->Data.Career;
	const Career::CareerWorldCity* SelectedCity = FindCity(CareerData, Screen->SelectedCityId);
	const Career::CareerWorldRegion* SelectedRegion = FindRegion(CareerData, Screen->SelectedRegionId);
	const Career::CareerWorldCountry* SelectedCountry = FindCountry(CareerData, Screen->SelectedCountryId);
	std::string WorldMapTitle = "CAREER WORLD";
	if (Screen->MapView == Screens::WorldMapScreen::WorldMapViewLevel::City && SelectedCity != nullptr)
		WorldMapTitle = SelectedCity->Name + " / CITY";
	else if (Screen->MapView == Screens::WorldMapScreen::WorldMapViewLevel::Region && SelectedRegion != nullptr)
		WorldMapTitle = SelectedRegion->Name + " / REGION";
	else if (Screen->MapView == Screens::WorldMapScreen::WorldMapViewLevel::Country && SelectedCountry != nullptr)
		WorldMapTitle = SelectedCountry->Name + " / COUNTRY";
	RenderText(Context->Store.GetFont(Assets::FontAssetMainFont), WorldMapTitle, 36, 22, CityTextColor);
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
		Screen->MapView == Screens::WorldMapScreen::WorldMapViewLevel::City
			? "MOUSE WHEEL: ZOOM   |   MOVE TO A MAP EDGE: PAN   |   HOVER TO INSPECT, CLICK TO ENTER"
			: Screen->MapView == Screens::WorldMapScreen::WorldMapViewLevel::Region
				? "SELECT A CITY TO OPEN ITS MAP   |   ZOOM OUT FOR THE COUNTRY"
				: "SELECT A REGION TO OPEN IT   |   ZOOM IN FOR THE REGION",
		36, 96, CityMutedTextColor);
	const SDL_FRect MapBounds{ CityMapLeft, CityMapTop, CityMapWidth, CityMapHeight };
	SDL_SetRenderDrawColor(Context->MainRenderer, 35, 44, 39, 255);
	SDL_RenderFillRect(Context->MainRenderer, &MapBounds);
	const SDL_Rect MapClip = GetCityMapViewportBounds();
	SDL_SetRenderClipRect(Context->MainRenderer, &MapClip);

	if (Screen->MapView != Screens::WorldMapScreen::WorldMapViewLevel::City)
	{
		const SDL_Color LandColor{ 58, 76, 62, 255 };
		const SDL_Color RoadColor{ 113, 118, 100, 255 };
		const SDL_Color CapitalColor{ 237, 191, 102, 255 };
		DrawFilledEllipse(Context->MainRenderer,
			{ CityMapLeft + CityMapWidth * 0.5f, CityMapTop + CityMapHeight * 0.5f },
			CityMapWidth * 0.39f, CityMapHeight * 0.37f, LandColor);

		if (Screen->MapView == Screens::WorldMapScreen::WorldMapViewLevel::Country &&
			SelectedCountry != nullptr)
		{
			const Career::CareerWorldRegion* CapitalRegion = FindRegion(CareerData,
				SelectedCountry->CapitalRegionId);
			const SDL_FPoint CapitalPosition = CapitalRegion == nullptr ? SDL_FPoint{} :
				GetOverviewMapPosition(CapitalRegion->MapX, CapitalRegion->MapY);
			for (const std::uint64_t RegionId : SelectedCountry->RegionIds)
			{
				const Career::CareerWorldRegion* Region = FindRegion(CareerData, RegionId);
				if (Region == nullptr)
					continue;
				const SDL_FPoint Position = GetOverviewMapPosition(Region->MapX, Region->MapY);
				if (CapitalRegion != nullptr && Region->Id != CapitalRegion->Id)
					DrawThickLine(Context->MainRenderer, CapitalPosition, Position, 5.0f, RoadColor);
			}
			for (const std::uint64_t RegionId : SelectedCountry->RegionIds)
			{
				const Career::CareerWorldRegion* Region = FindRegion(CareerData, RegionId);
				if (Region == nullptr)
					continue;
				const SDL_FPoint Position = GetOverviewMapPosition(Region->MapX, Region->MapY);
				const bool IsCapital = Region->Id == SelectedCountry->CapitalRegionId;
				const bool IsHovered = Region->Id == Screen->HoveredRegionId;
				DrawFilledEllipse(Context->MainRenderer, Position, IsCapital ? 13.0f : 10.0f,
					IsCapital ? 13.0f : 10.0f, IsCapital ? CapitalColor : SDL_Color{ 184, 197, 162, 255 });
				if (IsHovered)
					DrawEllipseRing(Context->MainRenderer, Position, 19.0f, 19.0f, 16.0f, 16.0f,
						CityTextColor);
				RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), Region->Name,
					static_cast<int>(Position.x + 17.0f), static_cast<int>(Position.y - 15.0f), CityTextColor);
				RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
					(IsCapital ? "CAPITAL REGION  |  " : "REGION  |  ") + FormatPopulation(Region->Population),
					static_cast<int>(Position.x + 17.0f), static_cast<int>(Position.y + 3.0f), CityMutedTextColor);
			}
		}
		else if (Screen->MapView == Screens::WorldMapScreen::WorldMapViewLevel::Region &&
			SelectedRegion != nullptr)
		{
			const Career::CareerWorldCity* CapitalCity = FindCity(CareerData,
				SelectedRegion->CapitalCityId);
			const SDL_FPoint CapitalPosition = CapitalCity == nullptr ? SDL_FPoint{} :
				GetOverviewMapPosition(CapitalCity->MapX, CapitalCity->MapY);
			for (const std::uint64_t CityId : SelectedRegion->CityIds)
			{
				const Career::CareerWorldCity* City = FindCity(CareerData, CityId);
				if (City == nullptr)
					continue;
				const SDL_FPoint Position = GetOverviewMapPosition(City->MapX, City->MapY);
				if (CapitalCity != nullptr && City->Id != CapitalCity->Id)
					DrawThickLine(Context->MainRenderer, CapitalPosition, Position, 4.0f, RoadColor);
			}
			for (const std::uint64_t CityId : SelectedRegion->CityIds)
			{
				const Career::CareerWorldCity* City = FindCity(CareerData, CityId);
				if (City == nullptr)
					continue;
				const SDL_FPoint Position = GetOverviewMapPosition(City->MapX, City->MapY);
				const bool IsHovered = City->Id == Screen->HoveredCityId;
				DrawFilledEllipse(Context->MainRenderer, Position, City->IsCapital ? 12.0f : 9.0f,
					City->IsCapital ? 12.0f : 9.0f, City->IsCapital ? CapitalColor : SDL_Color{ 184, 197, 162, 255 });
				if (IsHovered)
					DrawEllipseRing(Context->MainRenderer, Position, 18.0f, 18.0f, 15.0f, 15.0f,
						CityTextColor);
				const float LabelX = Position.x > CityMapLeft + CityMapWidth * 0.72f
					? Position.x - 150.0f : Position.x + 16.0f;
				RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), City->Name,
					static_cast<int>(LabelX), static_cast<int>(Position.y - 14.0f), CityTextColor);
				RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
					(City->IsCapital ? "CAPITAL CITY  |  " : "CITY  |  ") + FormatPopulation(City->Population),
					static_cast<int>(LabelX), static_cast<int>(Position.y + 4.0f), CityMutedTextColor);
			}
		}

		SDL_SetRenderClipRect(Context->MainRenderer, nullptr);
		SDL_SetRenderDrawColor(Context->MainRenderer, 173, 165, 137, 255);
		SDL_RenderRect(Context->MainRenderer, &MapBounds);
		const SDL_FRect InfoBounds{ CityMapLeft, 624.0f, CityMapWidth, 52.0f };
		SDL_SetRenderDrawColor(Context->MainRenderer, 34, 33, 30, 255);
		SDL_RenderFillRect(Context->MainRenderer, &InfoBounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, 125, 111, 88, 255);
		SDL_RenderRect(Context->MainRenderer, &InfoBounds);
		if (Screen->MapView == Screens::WorldMapScreen::WorldMapViewLevel::Country &&
			SelectedCountry != nullptr)
		{
			const Career::CareerWorldRegion* CapitalRegion = FindRegion(CareerData,
				SelectedCountry->CapitalRegionId);
			RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont),
				SelectedCountry->Name + "  |  Population " + FormatPopulation(SelectedCountry->Population),
				50, 630, CityTextColor);
			RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
				"Capital region: " + (CapitalRegion == nullptr ? std::string("Unknown") : CapitalRegion->Name),
				50, 651, CityMutedTextColor);
		}
		else if (Screen->MapView == Screens::WorldMapScreen::WorldMapViewLevel::Region &&
			SelectedRegion != nullptr)
		{
			const Career::CareerWorldCity* CapitalCity = FindCity(CareerData,
				SelectedRegion->CapitalCityId);
			RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont),
				SelectedRegion->Name + "  |  Population " + FormatPopulation(SelectedRegion->Population),
				50, 630, CityTextColor);
			RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
				"Capital city: " + (CapitalCity == nullptr ? std::string("Unknown") : CapitalCity->Name),
				50, 651, CityMutedTextColor);
		}
		DrawCityButton(*this, Context, Screen->PopulationButton, true);
		DrawCityButton(*this, Context, Screen->LeagueBrowserButton, true);
		DrawCityButton(*this, Context, Screen->ZoomOutButton,
			Screen->MapView == Screens::WorldMapScreen::WorldMapViewLevel::Region);
		DrawCityButton(*this, Context, Screen->ZoomInButton,
			Screen->MapView == Screens::WorldMapScreen::WorldMapViewLevel::Region ||
			Screen->MapView == Screens::WorldMapScreen::WorldMapViewLevel::Country);
		DrawCityButton(*this, Context, Screen->CareerHubButton, true);
		Display();
		return SDL_GetTicks() - StartTime;
	}

	const std::vector<Career::CityPlot>& CityPlots = Screen->GetCurrentCityPlots();

	const std::vector<std::size_t> PlotOrder = GetCityPlotDrawOrder(CityPlots);
	for (const std::size_t Index : PlotOrder)
	{
		const Career::CityPlot& Plot = CityPlots[Index];
		const SDL_FPoint Center = ProjectCityToScreen(ProjectCityGrid(static_cast<float>(Plot.GridX),
			static_cast<float>(Plot.GridY)), Screen->CityCameraX, Screen->CityCameraY, Screen->CityZoom);
		DrawFilledDiamond(Context->MainRenderer, Center, IsometricTileWidth * 0.5f * Screen->CityZoom,
			IsometricTileHeight * 0.5f * Screen->CityZoom, GetPlotGroundColor(Plot.Type));
	}

	const SDL_Color RoadShoulder{ 160, 146, 115, 255 };
	const SDL_Color RoadColor{ 77, 81, 78, 255 };
	for (const Screens::WorldMapScreen::CityRoadSegment& Road : Screen->CityRoadSegments)
	{
		const SDL_FPoint Start = ProjectCityToScreen(Road.Start,
			Screen->CityCameraX, Screen->CityCameraY, Screen->CityZoom);
		const SDL_FPoint End = ProjectCityToScreen(Road.End,
			Screen->CityCameraX, Screen->CityCameraY, Screen->CityZoom);
		DrawThickLine(Context->MainRenderer, Start, End, 18.0f * Screen->CityZoom, RoadShoulder);
		DrawThickLine(Context->MainRenderer, Start, End, 12.0f * Screen->CityZoom, RoadColor);
	}

	const float ElapsedSeconds = static_cast<float>(SDL_GetTicks()) / 1000.0f;
	std::vector<CityTrafficSprite> TrafficSprites;
	TrafficSprites.reserve(Screen->CityTraffic.size());
	for (const Screens::WorldMapScreen::CityTrafficCar& Car : Screen->CityTraffic)
	{
		if (Car.Route.size() < 2)
			continue;
		const std::size_t SegmentCount = Car.Route.size() - 1;
		std::vector<float> SegmentLengths(SegmentCount);
		float RouteLength = 0.0f;
		for (std::size_t Index = 0; Index < SegmentCount; ++Index)
		{
			const SDL_FPoint& Start = Car.Route[Index];
			const SDL_FPoint& End = Car.Route[Index + 1];
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
		for (std::size_t Index = 0; Index < SegmentCount; ++Index)
		{
			const SDL_FPoint& Start = Car.Route[Index];
			const SDL_FPoint& End = Car.Route[Index + 1];
			if (RouteDistance <= SegmentLengths[Index] || Index + 1 == SegmentCount)
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
		const float Angle = std::atan2(DirectionY, DirectionX) * 180.0f / 3.14159265358979323846f;
		float VehicleWidth = 0.0f;
		float VehicleHeight = 0.0f;
		SDL_GetTextureSize(Vehicle->Texture, &VehicleWidth, &VehicleHeight);
		const float CarScale = Screen->CityZoom * 1.1f;
		float DrawAngle = Angle;
		SDL_FlipMode Flip = SDL_FLIP_NONE;
		if (DrawAngle > 90.0f)
		{
			DrawAngle -= 180.0f;
			Flip = SDL_FLIP_HORIZONTAL;
		}
		else if (DrawAngle < -90.0f)
		{
			DrawAngle += 180.0f;
			Flip = SDL_FLIP_HORIZONTAL;
		}
		const float AngleRadians = DrawAngle * 3.14159265358979323846f / 180.0f;
		const float RotatedHalfHeight = 0.55f * (std::abs(std::sin(AngleRadians)) * VehicleWidth +
			std::abs(std::cos(AngleRadians)) * VehicleHeight);
		TrafficSprites.push_back({ Vehicle, Position, DrawAngle, Flip, CarScale, VehicleWidth,
			VehicleHeight, Position.y + RotatedHalfHeight });
	}
	std::vector<CitySceneItem> SceneItems;
	SceneItems.reserve(PlotOrder.size() + TrafficSprites.size());
	for (const std::size_t Index : PlotOrder)
	{
		const Career::CityPlot& Plot = CityPlots[Index];
		const SDL_FPoint Position = ProjectCityGrid(static_cast<float>(Plot.GridX), static_cast<float>(Plot.GridY));
		SceneItems.push_back({ Position.y + IsometricTileHeight * 0.5f, false, Index });
	}
	for (std::size_t Index = 0; Index < TrafficSprites.size(); ++Index)
		SceneItems.push_back({ TrafficSprites[Index].Depth, true, Index });
	std::sort(SceneItems.begin(), SceneItems.end(), [&CityPlots](const CitySceneItem& Left,
		const CitySceneItem& Right)
		{
			if (Left.Depth != Right.Depth)
				return Left.Depth < Right.Depth;
			if (Left.IsTraffic != Right.IsTraffic)
				return !Left.IsTraffic;
			if (Left.IsTraffic)
				return Left.Index < Right.Index;
			const Career::CityPlot& LeftPlot = CityPlots[Left.Index];
			const Career::CityPlot& RightPlot = CityPlots[Right.Index];
			if (LeftPlot.GridX != RightPlot.GridX)
				return LeftPlot.GridX < RightPlot.GridX;
			return Left.Index < Right.Index;
		});
	for (const CitySceneItem& Item : SceneItems)
	{
		if (Item.IsTraffic)
		{
			const CityTrafficSprite& Traffic = TrafficSprites[Item.Index];
			const SDL_FPoint Center = ProjectCityToScreen(Traffic.Position, Screen->CityCameraX,
				Screen->CityCameraY, Screen->CityZoom);
			const SDL_FRect CarBounds{ Center.x - Traffic.Width * Traffic.Scale * 0.5f,
				Center.y - Traffic.Height * Traffic.Scale * 0.5f,
				Traffic.Width * Traffic.Scale, Traffic.Height * Traffic.Scale };
			SDL_RenderTextureRotated(Context->MainRenderer, Traffic.Image->Texture, nullptr, &CarBounds,
				Traffic.Angle, nullptr, Traffic.Flip);
			continue;
		}

		const Career::CityPlot& Plot = CityPlots[Item.Index];
		const SDL_FPoint Center = ProjectCityToScreen(ProjectCityGrid(static_cast<float>(Plot.GridX),
			static_cast<float>(Plot.GridY)), Screen->CityCameraX, Screen->CityCameraY, Screen->CityZoom);
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
			continue;
		}

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
	const int HoveredPlotIndex = Screen->GetHoveredPlotIndex();
	if (HoveredPlotIndex >= 0 && static_cast<std::size_t>(HoveredPlotIndex) < CityPlots.size())
	{
		const Career::CityPlot& HoveredPlot = CityPlots[static_cast<std::size_t>(HoveredPlotIndex)];
		const SDL_FPoint Center = ProjectCityToScreen(ProjectCityGrid(static_cast<float>(HoveredPlot.GridX),
			static_cast<float>(HoveredPlot.GridY)), Screen->CityCameraX, Screen->CityCameraY, Screen->CityZoom);
		DrawDiamondOutline(Context->MainRenderer, Center, IsometricTileWidth * 0.49f * Screen->CityZoom,
			IsometricTileHeight * 0.49f * Screen->CityZoom, SDL_Color{ 246, 220, 126, 255 });
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
			Screen->SelectedCityId == CareerData.HomeCityId
				? GetPlotTooltipDetails(CareerData, *InfoPlot, PlotIndex)
				: "Remote city view | Select your home city to manage buildings",
			50, 651, CityMutedTextColor);
	}
	else if (SelectedCity != nullptr)
	{
		const Career::CareerWorldRegion* Region = FindRegion(CareerData, SelectedCity->RegionId);
		const Career::CareerWorldCountry* Country = Region == nullptr ? nullptr :
			FindCountry(CareerData, Region->CountryId);
		const std::string CityStatus = std::string(SelectedCity->IsCapital ? "Capital city" : "City") +
			"  |  Population " + FormatPopulation(SelectedCity->Population);
		std::string WorldHierarchy = Region == nullptr ? std::string("Region unknown") : Region->Name;
		if (Region != nullptr && Country != nullptr)
			WorldHierarchy += "  |  " + Country->Name;
		const SDL_FRect InfoBounds{ CityMapLeft, 624.0f, CityMapWidth, 52.0f };
		SDL_SetRenderDrawColor(Context->MainRenderer, 34, 33, 30, 255);
		SDL_RenderFillRect(Context->MainRenderer, &InfoBounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, 125, 111, 88, 255);
		SDL_RenderRect(Context->MainRenderer, &InfoBounds);
		RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), SelectedCity->Name + "  |  " + CityStatus,
			50, 630, CityTextColor);
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), WorldHierarchy,
			50, 651, CityMutedTextColor);
	}

	DrawCityButton(*this, Context, Screen->PopulationButton, true);
	DrawCityButton(*this, Context, Screen->LeagueBrowserButton, true);
	DrawCityButton(*this, Context, Screen->ZoomOutButton, true);
	DrawCityButton(*this, Context, Screen->ZoomInButton, false);
	DrawCityButton(*this, Context, Screen->CareerHubButton, true);
	Display();
	return SDL_GetTicks() - StartTime;
}
