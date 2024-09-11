#include "AppRenderer.h"
#include "Util.h"

#include "SDL3_image/SDL_image.h"
#include "spdlog/spdlog.h"
#include <iostream>

using namespace std;
using namespace Game;
using namespace Game::App;
using namespace Game::Screens;
using namespace Game::Render;

using namespace Game::Race;



Game::Render::BaseRenderer::BaseRenderer(AppData* _Data, BaseRendererData* _BaseData) : BaseData(_BaseData)
{
	Data = _Data;
	LastFrameEnd = 0;
}

bool Game::Render::BaseRenderer::LoadImageFile(std::string File, Image& Img)
{
	if ((Img.Surface = IMG_Load(File.c_str())) == nullptr)
		return false;

	if ((Img.Texture = SDL_CreateTextureFromSurface(Data->RenderData.MainRenderer, Img.Surface)) == nullptr)
		return false;
}

bool Game::Render::BaseRenderer::LoadFontFile(std::string File, unsigned int Size, TTF_Font* Font)
{
	return (Font = TTF_OpenFont(File.c_str(), Size)) != nullptr;
}

void Game::Render::BaseRenderer::RenderTextSingleLine(TTF_Font* Font, std::string Text, int x, int y, SDL_Color Color)
{
	SDL_Surface* surfaceMessage = TTF_RenderText_Blended(Font, Text.c_str(), Color);
	SDL_Texture* Message = SDL_CreateTextureFromSurface(Data->RenderData.MainRenderer, surfaceMessage);
	
	SDL_FRect Message_rect; //create a rect
	Message_rect.x = x;  //controls the rect's x coordinate 
	Message_rect.y = y; // controls the rect's y coordinte
	SDL_FPoint size; SDL_GetTextureSize(Message, &size.x, &size.y);
	Message_rect.w = size.x; // controls the width of the rect
	Message_rect.h = size.y; // controls the height of the rect
	SDL_RenderTexture(Data->RenderData.MainRenderer, Message, NULL, &Message_rect);

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
	SDL_RenderPresent(Data->RenderData.MainRenderer);

	LastFrameEnd = FrameTime;
}


void Game::Render::BaseRenderer::RenderImage(SDL_Texture* Texture, SDL_Rect* SourceQuad, SDL_Rect* RenderQuad)
{
	SDL_FRect SourceQuadF, RenderQuadF;
	if (SourceQuad == nullptr)
		SDL_GetTextureSize(Texture, &SourceQuadF.w, &SourceQuadF.h);
	else
		SDL_RectToFRect(SourceQuad, &SourceQuadF);
	SDL_RectToFRect(RenderQuad, &RenderQuadF);
	SDL_RenderTexture(Data->RenderData.MainRenderer, Texture, &SourceQuadF, &RenderQuadF);
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
bool Game::Render::BaseRendererData::Load(BaseRenderer& Renderer)
{
	Renderer.BaseData->BigFont = TTF_OpenFont("menu_font.ttf", 96);
	Renderer.BaseData->MainFont = TTF_OpenFont("PIXELLARI.ttf", 16);
	Renderer.BaseData->InfoFont = TTF_OpenFont("OpenSans.ttf", 14);
	Renderer.BaseData->DebugFont = TTF_OpenFont("OpenSans.ttf", 12);

	return true;
}
