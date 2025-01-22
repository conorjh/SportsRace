#ifndef RACERSCREEN_H
#define RACERSCREEN_H
#include "Renderer.h"
#include "Screens.h"
#include "App.h"
#include "Race.h"

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

	namespace Renderer
	{
		struct RacerScreenRendererData
		{
			bool Load(Render::BaseRenderer& Renderer);


			Render::Image RacerGraphic, Head, TrackGraphic, FellaRun, FellaWait, Background;
			SDL_Texture* InfoFontT, * DebugFontT, * WinningFontT;
			SDL_Texture* RacerTexture;

		};

		class RacerScreenRenderer : public Render::BaseRenderer
		{
			RacerScreenRendererData* RendererData;

		public:
			Screens::RacerScreen* State;

			RacerScreenRenderer(App::AppData*, RacerScreenRendererData* _Data, Render::BaseRendererData* _BaseData);

			unsigned int Render();

		};
	}
}
#endif