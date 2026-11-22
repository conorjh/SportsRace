#ifndef WORLDMAPSCREEN_H
#define WORLDMAPSCREEN_H
#include "..\Career.h"
#include "..\Render\Renderer.h"
#include "..\Screens.h"
#include "..\Util.h"

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
			int HoveredPlotIndex = -1;
			int InfoPlotIndex = -1;
			int RightClickPlotIndex = -1;
			unsigned int RightClickStartedAt = 0;
			std::size_t CityScrollRow = 0;
			float CityScrollRemainder = 0.0f;
			bool PlotInfoVisible = false;

			SDL_FRect GetPlotBounds(std::size_t Index) const;
			int GetPlotAt(float X, float Y) const;
			std::size_t GetMaximumCityScrollRow() const;

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
