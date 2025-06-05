#ifndef RENDERER_H
#define RENDERER_H
#include <vector>
#include <string>
#include <unordered_map>
#include "SDL3_ttf/SDL_ttf.h"

namespace Game
{
	namespace Render
	{
		class BaseRenderer;
		struct Image;
	}

	namespace Assets
	{
		enum FontAssetId
		{
			FontAssetMainFont, FontAssetInfoFont, FontAssetDebugFont, FontAssetWinningFont, FontAssetBigFont, FontAssetMainMenuFont
		};

		enum ImageAssetId
		{
			ImageAssetMainMenuBackground,

			ImageAssetRaceIcon,
			ImageAssetRacerIcon,
			ImageAssetTrainingIcon,
			ImageAssetRankingIcon,

			ImageAssetTrack, ImageAssetHead, ImageAssetFellaRun, ImageAssetFellaWait, ImageAssetScreen,

			ImageAssetRacer, ImageAssetStadium, ImageAssetStartingBlock, ImageAssetClouds, ImageAssetMountains,
			ImageAssetRaceBackground
		};

		class AssetStore
		{
			std::unordered_map<FontAssetId, TTF_Font*> Fonts;
			std::unordered_map<ImageAssetId, Game::Render::Image*> Images;

		public:
			bool RegisterAll(Game::Render::BaseRenderer& Renderer);

			void RegisterFont(FontAssetId FontId, TTF_Font* LoadedFont);
			void RegisterImage(ImageAssetId ImageId, Game::Render::Image* LoadedImage);

			TTF_Font* GetFont(FontAssetId Id);
			Game::Render::Image* GetImage(ImageAssetId Id);
		};
	}

	namespace Render
	{
		struct AppRenderContext
		{
			unsigned int ScreenWidth = 800;
			unsigned int ScreenHeight = 600;

			SDL_Window* MainWindow = nullptr;
			SDL_Renderer* MainRenderer = nullptr;
			SDL_Surface* MainSurface = nullptr;
			Game::Assets::AssetStore Store;
		};

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
		
		class BaseRenderer
		{
			unsigned int LastFrameEnd;
		protected:
			AppRenderContext* Context;
			FPSCounter FPS;

			virtual void RenderTextSingleLine(TTF_Font*, std::string Text, int x, int y, SDL_Color Color);
		public:
			BaseRenderer(AppRenderContext* Context);

			Image* LoadImageFile(const std::string File);
			bool LoadFontFile(std::string File, unsigned int Size, TTF_Font*);

			virtual void RenderText(TTF_Font*, std::string Text, int x, int y, SDL_Color Color);
			void RenderImage(SDL_Texture* Surface, SDL_Rect* SourceQuad, SDL_Rect* RenderQuad);

			virtual void Display();
		};
	}
	
	
}
#endif