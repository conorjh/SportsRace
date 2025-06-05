#include "RankingScreen.h"
#include "..\Audio.h"
#include "..\App\App.h"

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

Game::Screens::AppScreen* Game::Screens::RankingScreen::Update()
{
	ExitButton.Update();
	if (ExitButton.HasMouseClicked() || IO.Esc)
	{
		IO.Player.Play(Audio::BuiltInSounds::Click);

		//go back to last screen
		ScreenStack.Pop();
		return ScreenStack.Top();
	}

	return this;
}



Game::Render::RankingScreenRenderer::RankingScreenRenderer(Game::Render::AppRenderContext* Context) 
	: Screen(nullptr), 
	BaseRenderer(Context)
{
}

unsigned int Game::Render::RankingScreenRenderer::Render()
{
	//start timer
	auto StartTime = SDL_GetTicks();

	SDL_SetRenderDrawColor(Context->MainRenderer, 255, 255, 255, 255);
	SDL_RenderClear(Context->MainRenderer);

	SDL_Rect RenderQuad1 = { 120 , 50 , 378 * 2 , 359 };
	RenderImage(Context->Store.GetImage(Assets::ImageAssetMainMenuBackground)->Texture, NULL, &RenderQuad1);


	//exit
	auto MappedRgb = SDL_MapRGB(SDL_GetPixelFormatDetails(Context->MainSurface->format), SDL_GetSurfacePalette(Context->MainSurface), 255, 0, 0);
	SDL_FillSurfaceRect(Context->MainSurface, &Screen->ExitButton.Rect, MappedRgb);
	SDL_SetRenderDrawColor(Context->MainRenderer, 255, 255, 255, 255);
	SDL_FRect RectF;	SDL_RectToFRect(&Screen->ExitButton.Rect, &RectF);
	SDL_RenderRect(Context->MainRenderer, &RectF);
	RenderText(Context->Store.GetFont(Assets::FontAssetBigFont), Screen->ExitButton.Text.c_str(), Screen->ExitButton.x + 5, Screen->ExitButton.y + 5, Screen->ExitButton.Color);


	Display();
	auto EndTime = SDL_GetTicks();
	FPS.AddFrame(EndTime - StartTime);
	return EndTime - StartTime;
}
