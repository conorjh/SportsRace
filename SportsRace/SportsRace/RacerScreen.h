#ifndef RACERSCREEN_H
#define RACERSCREEN_H
#include "State.h"
#include "App.h"
#include "Race.h"

namespace Game
{
	namespace States
	{
		enum class RacerScreenStateInitType
		{
			RacerCreation, ViewOnly
		};

		class RacerScreenState : public AppState
		{
		protected:
			Race::Racer* RacerToDisplay;

		public:
			RacerScreenStateInitType InitType;

			GUI::Button RegenButton, ApplyButton, ExitButton;
			RacerScreenState(AppStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* _RacerToDisplay, RacerScreenStateInitType _Type);

			~RacerScreenState();

			AppState* Update();
		};

	}

	namespace Renderer
	{
		struct RacerScreenRendererData
		{
			bool Load(Render::BaseRenderer& Renderer);


			Render::Image RacerGraphic, Head, TrackGraphic, FellaRun, FellaWait, Screen;
			SDL_Texture* InfoFontT, * DebugFontT, * WinningFontT;
			SDL_Texture* RacerTexture;

		};

		class RacerScreenRenderer : public Render::BaseRenderer
		{
			RacerScreenRendererData* RendererData;

		public:
			States::RacerScreenState* State;

			RacerScreenRenderer(AppData*, RacerScreenRendererData* _Data, Render::BaseRendererData* _BaseData);

			unsigned int Render();

		};
	}
}
#endif