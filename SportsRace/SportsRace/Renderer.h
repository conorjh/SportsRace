#ifndef RENDERER_H
#define RENDERER_H
#include "State.h"
#include "MainMenu.h"
#include "CareerHub.h"
#include "RaceState.h"
#include "RacerScreen.h"
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

		struct Image
		{
			SDL_Surface* Surface;
			SDL_Texture* Texture;
		};

		class BaseRenderer;

		struct BaseRendererData
		{
			bool Load(BaseRenderer& Renderer);

			TTF_Font* MainFont, * InfoFont, * DebugFont, * WinningFont, *BigFont;
		};

		class BaseRenderer
		{
			friend struct BaseRendererData;

			unsigned int LastFrameEnd;
		protected:
			BaseRendererData* BaseData;
			App::AppData* Data;
			FPSCounter FPS;

			virtual void RenderTextSingleLine(TTF_Font*, std::string Text, int x, int y, SDL_Color Color);
		public:
			BaseRenderer(App::AppData* _Data, Render::BaseRendererData* _BaseData);

			void LoadImageFile(std::string File, Image&);
			void LoadFontFile(std::string File, unsigned int Size, TTF_Font*);

			virtual void RenderText(TTF_Font*, std::string Text, int x, int y, SDL_Color Color);
			void RenderImage(SDL_Texture* Surface, SDL_Rect* SourceQuad, SDL_Rect* RenderQuad);

			virtual void Display();
		};
	}

	namespace App
	{
		namespace Renderer
		{
			struct MainMenuRendererData
			{
				bool Load(Render::BaseRenderer& Renderer);

				Render::Image Screen;

				TTF_Font* MainFont;
			};

			class MainMenuRenderer : public Render::BaseRenderer
			{
				MainMenuRendererData* RendererData;

				void DrawLeague();

			public:
				States::MainMenuState* State;
				MainMenuRenderer(AppData*, MainMenuRendererData* _Data, Render::BaseRendererData* _BaseData);
				MainMenuRenderer(AppData*, States::MainMenuState* _State, MainMenuRendererData* _Data, Render::BaseRendererData* _BaseData);


				unsigned int Render();
			};

			struct InRaceRendererData
			{
				bool Load(Render::BaseRenderer& Renderer);

				TTF_Font* MainFont, * InfoFont, * DebugFont, * WinningFont;

				Render::Image RacerGraphic, Head, TrackGraphic, StadiumGraphic, StartingBlocksGraphic, CloudsGraphic, MountainsGraphic, FellaRun, FellaWait, Screen;

				SDL_Texture* MainFontT, * InfoFontT, * DebugFontT, * WinningFontT;
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
				InRaceRenderer(AppData*, InRaceRendererData* _Data, Render::BaseRendererData* _BaseData);
				InRaceRenderer(AppData*, States::InRaceState* _State, InRaceRendererData* _Data, Render::BaseRendererData* _BaseData);

				void DrawRacer(Race::Racer, unsigned int Track);
				void DrawWinners();

				unsigned int Render();

			};

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

			struct CareerHubRendererData
			{
				bool Load(Render::BaseRenderer& Renderer);


				Render::Image RacerIconGraphic, RaceIconGraphic, TrainingIconGraphic;
			};

			class CareerHubRenderer : public Render::BaseRenderer
			{
				CareerHubRendererData* RendererData;

			public:
				States::CareerHubState* State;

				CareerHubRenderer(AppData*, CareerHubRendererData* _Data, Render::BaseRendererData* _BaseData);
				CareerHubRenderer(AppData*, States::CareerHubState* State, CareerHubRendererData* _Data, Render::BaseRendererData* _BaseData);

				unsigned int Render();

			};


			class AppRenderer : public Render::BaseRenderer
			{
				Render::BaseRendererData BaseRenData;
				MainMenuRendererData MainMenuRenData;
				MainMenuRenderer MainMenuRen;
				InRaceRendererData InRaceRenData;
				InRaceRenderer InRaceRen;
				CareerHubRendererData CareerHubRenData;
				CareerHubRenderer CareerHubRen;
				RacerScreenRendererData RacerScreenRenData;
				RacerScreenRenderer RacerScreenRen;

			public:
				Game::States::AppStateMachine* StateMachine;

				AppRenderer(AppData* _RenderData, States::AppStateMachine* _StateMachine);

				unsigned int Render();
			};
		}

	}
}
#endif