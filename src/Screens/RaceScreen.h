#ifndef RACESCREEN_H
#define RACESCREEN_H
#include "..\Render\Renderer.h"
#include "..\Race.h"
#include "..\RaceState.h"

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
			RaceScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* TrainingRacer);					//InRaceStateInitType::Training
			RaceScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Race RaceToRun);						//demo
			RaceScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Race RaceToRun, Race::RacerGUID);		//InRaceStateInitType::Race
			~RaceScreen();

			void Entry(), Exit();

			AppScreen* Update();
			Race::RaceStateMachine RaceSM;
		};
	}
	
	namespace Render
	{

		struct RaceScreenRendererCamera
		{
			void PointAt(unsigned int X);

			unsigned int X, TrackLength;
			int  W, H;

			int CameraX, CameraX2;
		};

		class RaceScreenRenderer : public Render::BaseRenderer
		{
			void RenderDebugText();
			void DrawBackground();
			void DrawProgressBar();
			RaceScreenRendererCamera Camera;

		public:
			Screens::RaceScreen* Screen;
			RaceScreenRenderer(Game::Render::AppRenderContext* Context);
			RaceScreenRenderer(Game::Render::AppRenderContext* Context, Screens::RaceScreen* _State);

			void DrawRacer(Race::Racer, unsigned int Track);
			void DrawWinners();

			unsigned int Render();

		};
	}
}
#endif