#include "AppRenderer.h"
#include "..\Util.h"

#include "SDL3_image/SDL_image.h"
#include "spdlog/spdlog.h"
#include <iostream>

bool Game::Assets::AssetStore::RegisterAll(Game::Render::BaseRenderer& Renderer)
{
	using namespace Game::Assets;

	RegisterFont(FontAssetBigFont, TTF_OpenFont("media/menu_font.ttf", 96));
	RegisterFont(FontAssetMainFont, TTF_OpenFont("media/PIXELLARI.ttf", 16));
	RegisterFont(FontAssetInfoFont, TTF_OpenFont("media/OpenSans.ttf", 14));
	RegisterFont(FontAssetDebugFont, TTF_OpenFont("media/OpenSans.ttf", 12));
	RegisterFont(FontAssetWinningFont, TTF_OpenFont("media/menu_font.ttf", 16));
	RegisterFont(FontAssetMainMenuFont, TTF_OpenFont("media/menu_font.ttf", 96));

	RegisterImage(ImageAssetMainMenuBackground, Renderer.LoadImageFile("media/screen.png"));
	RegisterImage(ImageAssetRaceBackground, Renderer.LoadImageFile("media/screen.png"));
	RegisterImage(ImageAssetRaceIcon, Renderer.LoadImageFile("media/race_icon.png"));
	RegisterImage(ImageAssetRacerIcon, Renderer.LoadImageFile("media/racer_icon.png"));
	RegisterImage(ImageAssetTrainingIcon, Renderer.LoadImageFile("media/training_icon.png"));
	RegisterImage(ImageAssetRankingIcon, Renderer.LoadImageFile("media/ranking_icon.png"));
	RegisterImage(ImageAssetHead, Renderer.LoadImageFile("media/reido.png"));
	RegisterImage(ImageAssetTrack, Renderer.LoadImageFile("media/track.png"));
	RegisterImage(ImageAssetStadium, Renderer.LoadImageFile("media/stadium.png"));
	RegisterImage(ImageAssetStartingBlock, Renderer.LoadImageFile("media/startingblock.png"));
	RegisterImage(ImageAssetClouds, Renderer.LoadImageFile("media/clouds.png"));
	RegisterImage(ImageAssetMountains, Renderer.LoadImageFile("media/mountains.png"));
	RegisterImage(ImageAssetFellaRun, Renderer.LoadImageFile("media/fellarun.png"));
	RegisterImage(ImageAssetFellaWait, Renderer.LoadImageFile("media/fellawait.png"));
	RegisterImage(ImageAssetScreen, Renderer.LoadImageFile("media/screen.png"));

	return true;
}

void Game::Assets::AssetStore::RegisterFont(Game::Assets::FontAssetId FontId, TTF_Font* LoadedFont)
{
	Fonts.insert(std::make_pair(FontId, LoadedFont));
}

void Game::Assets::AssetStore::RegisterImage(Game::Assets::ImageAssetId ImageId, Game::Render::Image* LoadedImage)
{
	Images.insert(std::make_pair(ImageId, LoadedImage));
}

TTF_Font* Game::Assets::AssetStore::GetFont(Game::Assets::FontAssetId Id)
{
	return Fonts[Id];
}

Game::Render::Image* Game::Assets::AssetStore::GetImage(Game::Assets::ImageAssetId Id)
{
	return Images[Id];
}

Game::Render::BaseRenderer::BaseRenderer(AppRenderContext* Context)
	:Context(Context)
{


	LastFrameEnd = 0;
}

Game::Render::Image* Game::Render::BaseRenderer::LoadImageFile(const std::string File)
{
	using namespace std;
	using namespace	Game::Render;

	Image* Img = new Image();

	spdlog::trace("Loading image: " + File);
	if ((Img->Surface = IMG_Load(File.c_str())) == nullptr)
	{
		spdlog::error("Failed to load image: " + string(SDL_GetError()));
		return nullptr;
	}

	if ((Img->Texture = SDL_CreateTextureFromSurface(Context->MainRenderer, Img->Surface)) == nullptr)
	{
		spdlog::error("Failed to create texture: " + string(SDL_GetError()));
		return nullptr;
	}
	return Img;
}

bool Game::Render::BaseRenderer::LoadFontFile(std::string File, unsigned int Size, TTF_Font* Font)
{
	if ((Font = TTF_OpenFont(File.c_str(), Size)) == nullptr)
	{
		spdlog::error("Failed to load font " + File + ": " + SDL_GetError());
		return false;
	}
	return true;
}

void Game::Render::BaseRenderer::RenderTextSingleLine(TTF_Font* Font, std::string Text, int x, int y, SDL_Color Color)
{
	SDL_Surface* surfaceMessage = TTF_RenderText_Blended(Font, Text.c_str(), Text.size(), Color);
	SDL_Texture* Message = SDL_CreateTextureFromSurface(Context->MainRenderer, surfaceMessage);
	
	SDL_FRect Message_rect; //create a rect
	Message_rect.x = x;  //controls the rect's x coordinate 
	Message_rect.y = y; // controls the rect's y coordinte
	SDL_FPoint size; SDL_GetTextureSize(Message, &size.x, &size.y);
	Message_rect.w = size.x; // controls the width of the rect
	Message_rect.h = size.y; // controls the height of the rect
	SDL_RenderTexture(Context->MainRenderer, Message, NULL, &Message_rect);

	// Don't forget to free your surface and texture
	SDL_DestroySurface(surfaceMessage);
	SDL_DestroyTexture(Message);
}

void Game::Render::BaseRenderer::RenderText(TTF_Font* Font, std::string Text, int x, int y, SDL_Color Color)
{
	//split text into lines
	std::string buffer = ""; unsigned int offset = 0;
	for (auto it = Text.begin(); it != Text.end(); ++it)
	{
		if (*it == '\n')
		{
			RenderTextSingleLine(Font, buffer, x, y + offset, Color);
			offset += 12;
			buffer = "";
			continue;
		}

		buffer += *it;
	}

	RenderTextSingleLine(Font, buffer, x, y + offset, Color);
}

void Game::Render::BaseRenderer::Display()
{
	auto FrameTime = SDL_GetTicks();

	FPS.AddFrame(FrameTime - LastFrameEnd);
	SDL_RenderPresent(Context->MainRenderer);

	LastFrameEnd = FrameTime;
}

void Game::Render::BaseRenderer::RenderImage(SDL_Texture* Texture, SDL_Rect* SourceQuad, SDL_Rect* RenderQuad)
{
	SDL_FRect SourceQuadF, RenderQuadF;
	if (SourceQuad == nullptr)
	{
		SourceQuadF.x = SourceQuadF.y = 0;
		SDL_GetTextureSize(Texture, &SourceQuadF.w, &SourceQuadF.h);
	}
	else
		SDL_RectToFRect(SourceQuad, &SourceQuadF);
	SDL_RectToFRect(RenderQuad, &RenderQuadF);
	SDL_RenderTexture(Context->MainRenderer, Texture, &SourceQuadF, &RenderQuadF);
}

void Game::Render::FPSCounter::AddFrame(unsigned int TimeTakenMs)
{
	if (TimeTakenMs > 1000)
		TimeTakenMs = 1000;

	//max 3k samples
	while (TotalFrameTime + TimeTakenMs > 1000)
	{
		TotalFrameTime -= *FrameTimes.begin();
		FrameTimes.erase(FrameTimes.begin());
	}

	FrameTimes.push_back(TimeTakenMs);
	TotalFrameTime += TimeTakenMs;
}

void Game::Render::FPSCounter::Clear()
{
	FrameTimes.clear();
	TotalFrameTime = 0;
}

unsigned int Game::Render::FPSCounter::GetFrameTime()
{
	if (FrameTimes.size() == 0)
		return 0;

	return FrameTimes.back();
}

unsigned int Game::Render::FPSCounter::GetFPS()
{
	if (!FrameTimes.size() || !TotalFrameTime)
		return 0;

	return 1000 / GetFrameTime();
}

