#ifndef RACESCREEN_H
#define RACESCREEN_H
#include "App.h"
#include "Renderer.h"
#include "Race.h"
#include "RaceState.h"

namespace Game
{
	namespace Screens
	{
		enum class InRaceStateInitType
		{
			Race, Training
		};

		class RaceScreen : public AppScreen
		{
			unsigned int LastFrameEnd;
		public:
			Race::RacerGUID PlayerGUID;
			RaceScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* TrainingRacer);	//InRaceStateInitType::Training
			RaceScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Race RaceToRun);			//demo
			RaceScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Race RaceToRun, Race::RacerGUID);			//InRaceStateInitType::Race
			~RaceScreen();

			void Entry(), Exit();

			AppScreen* Update();
			Race::RaceStateMachine RaceSM;
		};
	}
	
	namespace Renderer
	{
		struct RaceScreenRendererData
		{
			bool Load(Render::BaseRenderer& Renderer);

			TTF_Font* MainFont, * InfoFont, * DebugFont, * WinningFont;

			Game::Render::Image RacerGraphic, Head, TrackGraphic, StadiumGraphic, StartingBlocksGraphic, CloudsGraphic, MountainsGraphic, FellaRun, FellaWait, Background;

			SDL_Texture* MainFontT, * InfoFontT, * DebugFontT, * WinningFontT;
			SDL_Texture* RacerTexture;

		};

		struct RaceScreenRendererCamera
		{
			void PointAt(unsigned int X);

			unsigned int X, TrackLength;
			int  W, H;

			int CameraX, CameraX2;
		};

		class RaceScreenRenderer : public Render::BaseRenderer
		{
			RaceScreenRendererData* RendererData;
			void RenderDebugText();
			void DrawBackground();
			void DrawProgressBar();
			RaceScreenRendererCamera Camera;

		public:
			Screens::RaceScreen* State;
			RaceScreenRenderer(App::AppData*, RaceScreenRendererData* _Data, Render::BaseRendererData* _BaseData);
			RaceScreenRenderer(App::AppData*, Screens::RaceScreen* _State, RaceScreenRendererData* _Data, Render::BaseRendererData* _BaseData);

			void DrawRacer(Race::Racer, unsigned int Track);
			void DrawWinners();

			unsigned int Render();

		};
	}
}
#endif