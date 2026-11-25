#ifndef WORLDMAPSCREEN_H
#define WORLDMAPSCREEN_H
#include "..\Career.h"
#include "..\Render\Renderer.h"
#include "..\Screens.h"
#include "..\Util.h"
#include <array>
#include <cstdint>
#include <vector>

namespace Game
{
	namespace Render
	{
		class WorldMapRenderer;
	}

	namespace Screens
	{
		class WorldMapScreen : public AppScreen
		{
			friend class Game::Render::WorldMapRenderer;
			enum class WorldMapViewLevel
			{
				City,
				Region,
				Country
			};

			Career::CareerOrchestrator Orchestrator;
			GUI::Button PopulationButton;
			GUI::Button CareerHubButton;
			GUI::Button LeagueBrowserButton;
			GUI::Button ZoomOutButton;
			GUI::Button ZoomInButton;
			struct CityRoadSegment
			{
				SDL_FPoint Start;
				SDL_FPoint End;
			};
			struct CityTrafficCar
			{
				std::vector<SDL_FPoint> Route;
				unsigned int SpriteIndex = 0;
				float RouteProgress = 0.0f;
				float Speed = 0.0f;
				std::uint64_t NextHonkAtMs = 0;
				std::uint64_t HonkIntervalMs = 0;
			};
			std::array<CityTrafficCar, 6> CityTraffic{};
			std::vector<CityRoadSegment> CityRoadSegments;
			WorldMapViewLevel MapView = WorldMapViewLevel::City;
			std::uint64_t SelectedCityId = 0;
			std::uint64_t SelectedRegionId = 0;
			std::uint64_t SelectedCountryId = 0;
			std::uint64_t HoveredCityId = 0;
			std::uint64_t HoveredRegionId = 0;
			float CityZoom = 0.82f;
			float CityCameraX = 0.0f;
			float CityCameraY = 0.0f;
			std::uint64_t LastCityInputUpdateMs = 0;
			int HoveredPlotIndex = -1;
			int FocusedPlotIndex = -1;
			bool CityCameraInitialized = false;
			bool CityRoadNetworkInitialized = false;
			std::uint64_t CityRoadNetworkCityId = 0;
			unsigned int CityRoadMaximumGridX = 0;
			unsigned int CityRoadMaximumGridY = 0;

			const std::vector<Career::CityPlot>& GetCurrentCityPlots() const;
			SDL_FRect GetCityMapBounds() const;
			int GetPlotAt(float X, float Y) const;
			std::uint64_t GetCityMarkerAt(float X, float Y) const;
			std::uint64_t GetRegionMarkerAt(float X, float Y) const;
			void ZoomOut();
			void ZoomIn();
			void InitializeTraffic();
			void UpdateCityCamera();
			void UpdateLayout();
			void ClampCityCamera();
			void UpdateCityTraffic();

		public:
			WorldMapScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data);

			void Entry() override;
			AppScreen* Update() override;
			void FocusOnLocation(std::uint64_t CityId, int PlotIndex = -1);
			const Career::CityPlot* GetInfoPlot() const;
			bool IsPlotInfoVisible() const;
			int GetHoveredPlotIndex() const;
		};
	}

	namespace Render
	{
		void DrawCareerHubMapPreview(AppRenderContext* Context, const Career::CareerData& Data,
			std::uint64_t CityId, const SDL_FRect& Bounds);

		class WorldMapRenderer : public BaseRenderer
		{
		public:
			Screens::WorldMapScreen* Screen = nullptr;

			WorldMapRenderer(AppRenderContext* Context);
			unsigned int Render();
		};
	}
}
#endif
