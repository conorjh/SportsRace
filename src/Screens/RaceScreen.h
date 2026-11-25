#ifndef RACESCREEN_H
#define RACESCREEN_H
#include "..\Screens.h"
#include "..\Render\Renderer.h"
#include "..\Race.h"
#include "..\RaceState.h"
#include "..\Career.h"
#include "PostRaceScreen.h"
#include "..\Util.h"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace Game
{
	namespace Render
	{
		class RaceScreenRenderer;
	}

	namespace Screens
	{
		enum class InRaceStateInitType
		{
			Race, Training
		};

		class RaceScreen : public AppScreen
		{
			friend class Game::Render::RaceScreenRenderer;

			unsigned int LastFrameEnd;
			bool ShowPostRaceOnComplete = false;
			bool PostRaceShown = false;
			bool PlayerRacerSupplied = false;
			bool PlayerRaceResultSaved = false;
			bool IsWorldRaceViewer = false;
			std::vector<RacerProgressSnapshot> PreRaceProgress;
			std::vector<SDL_Rect> SpectatorRacerHitBounds;
			std::string RaceEventName;
			std::size_t ViewedRacerIndex = 0;
			GUI::Button PreviousViewedRacerButton;
			GUI::Button NextViewedRacerButton;

			void CapturePreRaceProgress();
			Race::Racer* GetViewedRacer() const;

		public:
			Race::RacerGUID PlayerGUID;
			RaceScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* TrainingRacer);					//InRaceStateInitType::Training
			RaceScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Race RaceToRun);						//demo
			RaceScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Race RaceToRun, Race::RacerGUID);		//InRaceStateInitType::Race
			~RaceScreen();
			bool HasPlayerRacer() const
			{
				return PlayerRacerSupplied;
			}

			void Entry(), Exit();

			AppScreen* Update();
			std::shared_ptr<Race::RaceStateMachine> RaceSM;
			std::shared_ptr<Career::CareerWorldRace> ViewedWorldRace;
			RaceScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data,
				const std::shared_ptr<Career::CareerWorldRace>& WorldRace);
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
			void DrawSpectatorControls();
			RaceScreenRendererCamera Camera;

		public:
			Screens::RaceScreen* Screen;
			RaceScreenRenderer(Game::Render::AppRenderContext* Context);
			RaceScreenRenderer(Game::Render::AppRenderContext* Context, Screens::RaceScreen* _State);

			void DrawRacer(const Race::Racer& Racer, unsigned int Track);
			void DrawWinners();

			unsigned int Render();

		};
	}
}
#endif