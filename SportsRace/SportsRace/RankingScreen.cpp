#include "RankingScreen.h"
#include "Audio.h"

using namespace std;
using namespace Game::App;
using namespace Game::Audio;
using namespace Game::Screens;
using namespace Game::Render;
using namespace Game::Renderer;
using namespace Game::Race;

bool Game::Renderer::RankingScreenRendererData::Load(Render::BaseRenderer& Renderer)
{
	Renderer.LoadImageFile("screen.png", Screen);

	return true;
}

Game::Screens::RankingScreen::RankingScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* _RacerToDisplay) :
	AppScreen(_Machine, _IO, _Data),
	ExitButton(_IO, "Exit", 800, 650, 210, 80)
{
	Type = AppScreenType::RankingScreen;
	RacerToDisplay = _RacerToDisplay;
}

Game::Screens::RankingScreen::~RankingScreen()
{
}

AppScreen* Game::Screens::RankingScreen::Update()
{
	ExitButton.Update();
	if (ExitButton.HasMouseClicked() || IO.Esc)
	{
		IO.Player.Play(BuiltInSounds::Click);

		//go back to last screen
		ScreenStack.Pop();
		return ScreenStack.Top();
	}

	return this;
}



Game::Renderer::RankingScreenRenderer::RankingScreenRenderer(AppData* _Data, RankingScreenRendererData* _RendererData, BaseRendererData* _BaseData) :
	State(nullptr), RendererData(_RendererData), BaseRenderer(_Data, _BaseData)
{
}

unsigned int Game::Renderer::RankingScreenRenderer::Render()
{
	//start timer
	auto StartTime = SDL_GetTicks();

	SDL_SetRenderDrawColor(Data->RenderData.MainRenderer, 255, 255, 255, 255);
	SDL_RenderClear(Data->RenderData.MainRenderer);

	SDL_Rect RenderQuad1 = { 120 , 50 , 378 * 2 , 359 };
	RenderImage(RendererData->Screen.Texture, NULL, &RenderQuad1);


	//exit
	auto MappedRgb = SDL_MapRGB(SDL_GetPixelFormatDetails(Data->RenderData.MainSurface->format), SDL_GetSurfacePalette(Data->RenderData.MainSurface), 255, 0, 0);
	SDL_FillSurfaceRect(Data->RenderData.MainSurface, &State->ExitButton.Rect, MappedRgb);
	SDL_SetRenderDrawColor(Data->RenderData.MainRenderer, 255, 255, 255, 255);
	SDL_FRect RectF;	SDL_RectToFRect(&State->ExitButton.Rect, &RectF);
	SDL_RenderRect(Data->RenderData.MainRenderer, &RectF);
	RenderText(BaseData->BigFont, State->ExitButton.Text.c_str(), State->ExitButton.x + 5, State->ExitButton.y + 5, State->ExitButton.Color);


	Display();
	auto EndTime = SDL_GetTicks();
	FPS.AddFrame(EndTime - StartTime);
	return EndTime - StartTime;
}
