#ifndef RANKINGSCREEN_H
#define RANKINGSCREEN_H
#include "Renderer.h"
#include "Screens.h"
#include "App.h"
#include "Race.h"

namespace Game
{
	namespace Screens
	{
		class RankingScreen : public AppScreen
		{
		protected:
			Race::Racer* RacerToDisplay;

		public:

			GUI::Button ExitButton;
			RankingScreen(AppStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* _RacerToDisplay);

			~RankingScreen();

			AppScreen* Update();
		};

	}

	namespace Renderer
	{
		struct RankingScreenRendererData
		{
			bool Load(Render::BaseRenderer& Renderer);


			Render::Image Screen;
			SDL_Texture* InfoFontT, * DebugFontT, * WinningFontT;
			SDL_Texture* RacerTexture;

		};

		class RankingScreenRenderer : public Render::BaseRenderer
		{
			RankingScreenRendererData* RendererData;

		public:
			Screens::RankingScreen* State;

			RankingScreenRenderer(App::AppData*, RankingScreenRendererData* _Data, Render::BaseRendererData* _BaseData);

			unsigned int Render();

		};
	}
}
#endif