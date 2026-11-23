#ifndef RENDERER_H
#define RENDERER_H
#include <chrono>
#include <vector>
#include <string>
#include <unordered_map>
#include "SDL3/SDL.h"
#include "SDL3_ttf/SDL_ttf.h"

namespace Game
{
	namespace Race
	{
		class Racer;
	}

	namespace Render
	{
		inline constexpr SDL_Color PlayerOwnedRacerHighlightColor{ 255, 202, 96, 255 };

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

			ImageAssetTrack, ImageAssetHead, ImageAssetFellaRun, ImageAssetFellaStumble, ImageAssetFellaWait, ImageAssetScreen,

			ImageAssetRacer, ImageAssetStadium, ImageAssetStartingBlock, ImageAssetClouds, ImageAssetMountains,
			ImageAssetRaceBackground,
			ImageAssetWorldMap,
			ImageAssetCityTile000, ImageAssetCityTile001, ImageAssetCityTile002, ImageAssetCityTile003,
			ImageAssetCityTile004, ImageAssetCityTile005, ImageAssetCityTile006, ImageAssetCityTile007,
			ImageAssetCityTile008, ImageAssetCityTile009, ImageAssetCityTile010, ImageAssetCityTile011,
			ImageAssetIsometricLandscape000, ImageAssetIsometricLandscape001,
			ImageAssetIsometricLandscape002, ImageAssetIsometricLandscape003,
			ImageAssetIsometricCity000, ImageAssetIsometricCity001, ImageAssetIsometricCity002,
			ImageAssetIsometricCity003, ImageAssetIsometricCity004, ImageAssetIsometricCity005,
			ImageAssetIsometricCity006, ImageAssetIsometricCity007, ImageAssetIsometricCity008,
			ImageAssetIsometricCity009, ImageAssetIsometricCity010, ImageAssetIsometricCity011,
			ImageAssetIsometricBuilding000, ImageAssetIsometricBuilding001,
			ImageAssetIsometricBuilding002, ImageAssetIsometricBuilding003,
			ImageAssetIsometricBuilding004, ImageAssetIsometricBuilding005,
			ImageAssetIsometricBuilding006, ImageAssetIsometricBuilding007,
			ImageAssetIsometricBuilding008, ImageAssetIsometricBuilding009,
			ImageAssetIsometricBuilding010, ImageAssetIsometricBuilding011,
			ImageAssetIsometricVehicleBlack, ImageAssetIsometricVehicleBlue,
			ImageAssetIsometricVehicleGreen, ImageAssetIsometricVehicleRed,
			ImageAssetIsometricVehicleSilver
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
			struct RacerInfoCandidate
			{
				std::string Name;
				Game::Race::Racer* Runner = nullptr;
			};

			struct RacerInfoClickTarget
			{
				SDL_Rect Bounds{};
				Game::Race::Racer* Runner = nullptr;
			};

			unsigned int ScreenWidth = 1024;
			unsigned int ScreenHeight = 768;

			SDL_Window* MainWindow = nullptr;
			SDL_Renderer* MainRenderer = nullptr;
			SDL_Surface* MainSurface = nullptr;
			bool ShowDebugMenu = false;
			bool NodeViewerEnabled = false;
			std::string DebugMenuStatus;
			bool RacerInfoLinksEnabled = false;
			std::vector<RacerInfoCandidate> RacerInfoCandidates;
			std::vector<RacerInfoClickTarget> RacerInfoClickTargets;
			Game::Assets::AssetStore Store;
		};

		class FPSCounter
		{
			std::vector<double> FrameTimes;
			double TotalFrameTime = 0.0;

		public:
			void AddFrame(double TimeTakenMs);
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
			std::chrono::steady_clock::time_point LastFrameEnd;
			bool HasLastFrameEnd;
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