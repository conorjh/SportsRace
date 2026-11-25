#ifndef BUILDINGSCREEN_H
#define BUILDINGSCREEN_H
#include "..\Career.h"
#include "..\Render\Renderer.h"
#include "..\Screens.h"
#include "..\Util.h"
#include <cstddef>
#include <string>

namespace Game
{
	namespace Render
	{
		class BuildingScreenRenderer;
		class SlotMachineScreenRenderer;
		class RouletteScreenRenderer;
	}

	namespace Screens
	{
		class BuildingScreen : public AppScreen
		{
			friend class Game::Render::BuildingScreenRenderer;
			friend class Game::Render::SlotMachineScreenRenderer;
			friend class Game::Render::RouletteScreenRenderer;

		protected:
			Career::CareerOrchestrator Orchestrator;
			Career::CityPlot Plot;
			GUI::Button BackButton;
			std::string StatusMessage;

			bool Clicked(float X, float Y, float Width, float Height) const;
			int MapX(float X) const;
			int MapY(float Y) const;
			virtual void UpdateActivity() = 0;
			static std::string FormatOdds(double Odds);
			static std::string FormatTrackLength(unsigned int TrackLength);
			static unsigned int LicensePrice(std::size_t Index);
			static const char* LicenseName(std::size_t Index);
			static const char* LicenseDescription(std::size_t Index);

		public:
			BuildingScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data,
				AppScreenType _Type, const Career::CityPlot& _Plot);

			AppScreen* Update() override;
		};
	}

	namespace Render
	{
		class BuildingScreenRenderer : public BaseRenderer
		{
		public:
			Screens::BuildingScreen* Screen = nullptr;

			BuildingScreenRenderer(AppRenderContext* Context);
			unsigned int Render();
		};
	}
}
#endif
