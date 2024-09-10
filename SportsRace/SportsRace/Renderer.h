#ifndef RENDERER_H
#define RENDERER_H
#include "Screens.h"
#include "App.h"
#include "SDL3_ttf/SDL_ttf.h"

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
	
	
}
#endif