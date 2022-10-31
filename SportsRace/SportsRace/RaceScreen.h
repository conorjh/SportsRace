#ifndef RACESCREEN_H
#define RACESCREEN_H
#include "App.h"
#include "Renderer.h"
#include "Race.h"

namespace Game
{
	namespace States
	{
		enum class InRaceStateInitType
		{
			Race, Training
		};

		class RaceScreenState : public AppState
		{
			unsigned int LastFrameEnd;
		public:
			Race::RacerGUID PlayerGUID;
			RaceScreenState(AppStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* TrainingRacer);	//InRaceStateInitType::Training
			RaceScreenState(AppStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Race RaceToRun);			//demo
			RaceScreenState(AppStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Race RaceToRun, Race::RacerGUID);			//InRaceStateInitType::Race
			~RaceScreenState();

			void Entry(), Exit();

			AppState* Update();
			Race::RaceStateMachine RaceSM;
		};
	}
	
	namespace Renderer
	{
		struct RaceScreenRendererData
		{
			bool Load(Render::BaseRenderer& Renderer);

			TTF_Font* MainFont, * InfoFont, * DebugFont, * WinningFont;

			Render::Image RacerGraphic, Head, TrackGraphic, StadiumGraphic, StartingBlocksGraphic, CloudsGraphic, MountainsGraphic, FellaRun, FellaWait, Screen;

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
			RaceScreenRendererCamera Camera;

		public:
			States::RaceScreenState* State;
			RaceScreenRenderer(App::AppData*, RaceScreenRendererData* _Data, Render::BaseRendererData* _BaseData);
			RaceScreenRenderer(App::AppData*, States::RaceScreenState* _State, RaceScreenRendererData* _Data, Render::BaseRendererData* _BaseData);

			void DrawRacer(Race::Racer, unsigned int Track);
			void DrawWinners();

			unsigned int Render();

		};
	}
}
#endif