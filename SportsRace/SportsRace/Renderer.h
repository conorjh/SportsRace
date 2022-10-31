#ifndef RENDERER_H
#define RENDERER_H
#include "State.h"
#include "MainMenu.h"
#include "CareerHub.h"
#include "RaceState.h"
#include "RaceScreen.h"
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
			class AppRenderer : public Render::BaseRenderer
			{
				Render::BaseRendererData BaseRenData;
				Game::Renderer::MainMenuRendererData MainMenuRenData;
				Game::Renderer::MainMenuRenderer MainMenuRen;
				Game::Renderer::RaceScreenRendererData InRaceRenData;
				Game::Renderer::RaceScreenRenderer InRaceRen;
				Game::Renderer::CareerHubRendererData CareerHubRenData;
				Game::Renderer::CareerHubRenderer CareerHubRen;
				Game::Renderer::RacerScreenRendererData RacerScreenRenData;
				Game::Renderer::RacerScreenRenderer RacerScreenRen;

			public:
				Game::States::AppStateMachine* StateMachine;

				AppRenderer(AppData* _RenderData, States::AppStateMachine* _StateMachine);

				unsigned int Render();
			};
		}

	}
}
#endif