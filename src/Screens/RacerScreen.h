#ifndef RACERSCREEN_H
#define RACERSCREEN_H
#include "..\Render/Renderer.h"
#include "..\Screens.h"
#include "..\Race.h"
#include "..\Career.h"
#include "..\Util.h"

namespace Game
{
	namespace Screens
	{
		enum class RacerScreenStateInitType
		{
			RacerCreation, ViewOnly
		};

		class RacerScreen : public AppScreen
		{
		protected:
			Race::Racer* RacerToDisplay;

		public:
			RacerScreenStateInitType InitType;

			GUI::Button RegenButton, ApplyButton, ExitButton;
			RacerScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* _RacerToDisplay, RacerScreenStateInitType _Type);

			~RacerScreen();

			AppScreen* Update();
		};

	}

	namespace Render
	{

		class RacerScreenRenderer : public Render::BaseRenderer
		{
			Game::Career::CareerProfile* RacerProfile;

		public:
			Screens::RacerScreen* Screen;

			RacerScreenRenderer(Game::Render::AppRenderContext* Context, Game::Career::CareerProfile* RacerProfile);

			unsigned int Render();

		};
	}
}
#endif