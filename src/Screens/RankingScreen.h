#ifndef RANKINGSCREEN_H
#define RANKINGSCREEN_H
#include "..\Render\Renderer.h"
#include "..\Screens.h"
#include "..\Race.h"
#include "..\Util.h"

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
			RankingScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* _RacerToDisplay);

			~RankingScreen();

			AppScreen* Update();
		};
	}

	namespace Render
	{

		class RankingScreenRenderer : public Render::BaseRenderer
		{
		public:
			Screens::RankingScreen* Screen;

			RankingScreenRenderer(Game::Render::AppRenderContext* Context);

			unsigned int Render();
		};
	}
}
#endif