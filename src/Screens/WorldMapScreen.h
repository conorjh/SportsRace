#ifndef WORLDMAPSCREEN_H
#define WORLDMAPSCREEN_H
#include "..\Career.h"
#include "..\Render\Renderer.h"
#include "..\Screens.h"
#include "..\Util.h"
#include <array>
#include <cstdint>

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

			Career::CareerOrchestrator Orchestrator;
			GUI::Button PopulationButton;
			GUI::Button CareerHubButton;
			GUI::Button LeagueBrowserButton;
			struct CityTrafficCar
			{
				unsigned int RouteRow = 0;
				unsigned int SpriteIndex = 0;
				float RouteProgress = 0.0f;
				float Speed = 0.0f;
				std::uint64_t NextHonkAtMs = 0;
				std::uint64_t HonkIntervalMs = 0;
			};
			std::array<CityTrafficCar, 6> CityTraffic{};
			float CityZoom = 0.82f;
			float CityCameraX = 0.0f;
			float CityCameraY = 0.0f;
			std::uint64_t LastCityInputUpdateMs = 0;
			int HoveredPlotIndex = -1;
			bool CityCameraInitialized = false;

			int GetPlotAt(float X, float Y) const;
			void InitializeTraffic();
			void UpdateCityCamera();
			void ClampCityCamera();
			void UpdateCityTraffic();

		public:
			WorldMapScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data);

			void Entry() override;
			AppScreen* Update() override;
			const Career::CityPlot* GetInfoPlot() const;
			bool IsPlotInfoVisible() const;
			int GetHoveredPlotIndex() const;
		};
	}

	namespace Render
	{
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
