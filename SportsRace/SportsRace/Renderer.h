#ifndef RENDERER_H
#define RENDERER_H
#include "State.h"
#include "MainMenu.h"
#include "Race.h"
#include "Renderer.h"
#include "App.h"
#include "SDL2/SDL_ttf.h"

namespace Game
{
	namespace Render
	{
		class FPSCounter
		{
			std::vector<unsigned int> FrameTimes;
			unsigned int TotalFrameTime = 0;

		public:
			void AddFrame(unsigned int TimeTakenMs);
			void Clear();
			unsigned int GetFrameTime();
			unsigned int GetFPS();
		};



		class BaseRenderer
		{
			unsigned int LastFrameEnd;
		protected:
			App::AppData* Data;
			FPSCounter FPS;

			virtual void RenderTextSingleLine(TTF_Font*, std::string Text, int x, int y, SDL_Color Color);
		public:
			BaseRenderer(App::AppData* _Data);

			virtual void RenderText(TTF_Font*, std::string Text, int x, int y, SDL_Color Color);
			void DrawImage(SDL_Surface* Surface, SDL_Rect* SourceQuad, SDL_Rect* RenderQuad);
			virtual void Display();
		};
	}

	namespace App
	{
		namespace Renderer
		{
			struct MainMenuRendererData
			{
				void Load();

				SDL_Surface* Screen;

				TTF_Font* MainFont, * InfoFont, * DebugFont;
			};

			class MainMenuRenderer : public Render::BaseRenderer
			{
				MainMenuRendererData* RendererData;

				void DrawLeague();

			public:
				States::MainMenuState* State;
				MainMenuRenderer(AppData*, MainMenuRendererData* _Data);
				MainMenuRenderer(AppData*, States::MainMenuState* _State, MainMenuRendererData* _Data);

				unsigned int Render();
			};

			struct InRaceRendererData
			{
				void Load();

				TTF_Font* MainFont, * InfoFont, * DebugFont, * WinningFont;

				SDL_Surface* RacerGraphic, * Head, * TrackGraphic, * StadiumGraphic, * StartingBlocksGraphic, * CloudsGraphic, * MountainsGraphic, * FellaRun, * FellaWait, * Screen;
				SDL_Texture* RacerTexture;

			};

			struct InRaceRendererCamera
			{
				void PointAt(unsigned int X);

				unsigned int X, TrackLength;
				int  W, H;

				int CameraX, CameraX2;
			};

			class InRaceRenderer : public Render::BaseRenderer
			{
				InRaceRendererData* RendererData;
				void RenderDebugText();
				void DrawBackground();
				InRaceRendererCamera Camera;

			public:
				States::InRaceState* State;
				InRaceRenderer(AppData*, InRaceRendererData* _Data);
				InRaceRenderer(AppData*, States::InRaceState* _State, InRaceRendererData* _Data);

				void DrawRacer(Race::Racer, unsigned int Track);
				void DrawWinners();

				unsigned int Render();
			};

			class AppRenderer : public Render::BaseRenderer
			{
				MainMenuRendererData MainMenuRenData;
				MainMenuRenderer MainMenuRen;
				InRaceRendererData InRaceRenData;
				InRaceRenderer InRaceRen;

			public:
				Game::States::AppStateMachine* StateMachine;

				AppRenderer(AppData* _RenderData, States::AppStateMachine* _StateMachine);

				unsigned int Render();
			};
		}

	}
}
#endif