#ifndef CAREERHUB_H
#define CAREERHUB_H
#include "..\Screens.h"
#include "..\Career.h"
#include "..\Render\Renderer.h"
#include "..\Util.h"
#include <string>

namespace Game
{
	namespace Screens
	{
		class CareerHubScreen : public AppScreen
		{
			Career::CareerOrchestrator Orchestrator;
			void UpdateLayout();
		public:
			GUI::Button LeagueBrowserButton;
			GUI::Button DatabaseButton;
			GUI::Button WorldMapButton, StableButton, BreedingFarmButton, SaveCareerButton;
			std::string StatusMessage;

			CareerHubScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data);
			~CareerHubScreen();

			void Entry(), Exit();

			AppScreen* Update();
			Career::CareerData* GetCareerData() const;
			int GetPlayerCash() const;
		};
	}

	namespace Render
	{

		class CareerHubRenderer : public Render::BaseRenderer
		{
		public:
			Screens::CareerHubScreen* Screen;

			CareerHubRenderer(Game::Render::AppRenderContext* Context);
			CareerHubRenderer(Game::Render::AppRenderContext* Context, Screens::CareerHubScreen* State);

			unsigned int Render();

		};
	}
}
#endif