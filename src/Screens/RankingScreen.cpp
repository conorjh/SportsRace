#include "RankingScreen.h"
#include "..\Audio.h"
#include "..\App\App.h"
#include <algorithm>

Game::Screens::RankingScreen::RankingScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* _RacerToDisplay) :
	AppScreen(_Machine, _IO, _Data),
	ExitButton(_IO, "Exit", 800, 650, 210, 80)
{
	Type = AppScreenType::RankingScreen;
	RacerToDisplay = _RacerToDisplay;
	UpdateLayout();
}

Game::Screens::RankingScreen::~RankingScreen()
{
}

Game::Screens::AppScreen* Game::Screens::RankingScreen::Update()
{
	UpdateLayout();
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

void Game::Screens::RankingScreen::UpdateLayout()
{
	const int Width = Data.RenderContext == nullptr ? 1024 : static_cast<int>(Data.RenderContext->ScreenWidth);
	const int Height = Data.RenderContext == nullptr ? 768 : static_cast<int>(Data.RenderContext->ScreenHeight);
	const GUI::RelativeLayout Layout(static_cast<float>(Width), static_cast<float>(Height));
	ExitButton.SetBounds(Width - Layout.MapX(14.0f) - 210, Height - Layout.MapY(38.0f) - 80, 210, 80);
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

	if (Screen == nullptr)
	{
		Display();
		return SDL_GetTicks() - StartTime;
	}
	Image* Background = Context->Store.GetImage(Assets::ImageAssetMainMenuBackground);
	if (Background != nullptr && Background->Texture != nullptr && Background->Surface != nullptr)
	{
		const int MaximumWidth = static_cast<int>(Context->ScreenWidth * 0.78f);
		const int MaximumHeight = static_cast<int>(Context->ScreenHeight * 0.68f);
		const float Scale = std::min(static_cast<float>(MaximumWidth) / Background->Surface->w,
			static_cast<float>(MaximumHeight) / Background->Surface->h);
		const int ImageWidth = static_cast<int>(Background->Surface->w * Scale);
		const int ImageHeight = static_cast<int>(Background->Surface->h * Scale);
		SDL_Rect RenderQuad1{ (static_cast<int>(Context->ScreenWidth) - ImageWidth) / 2,
			(static_cast<int>(Context->ScreenHeight) - ImageHeight) / 2, ImageWidth, ImageHeight };
		RenderImage(Background->Texture, nullptr, &RenderQuad1);
	}


	//exit
	SDL_FRect RectF;	SDL_RectToFRect(&Screen->ExitButton.Rect, &RectF);
	SDL_SetRenderDrawColor(Context->MainRenderer, 255, 0, 0, 255);
	SDL_RenderFillRect(Context->MainRenderer, &RectF);
	SDL_SetRenderDrawColor(Context->MainRenderer, 255, 255, 255, 255);
	SDL_RenderRect(Context->MainRenderer, &RectF);
	RenderText(Context->Store.GetFont(Assets::FontAssetBigFont), Screen->ExitButton.Text.c_str(), Screen->ExitButton.x + 5, Screen->ExitButton.y + 5, Screen->ExitButton.Color);


	Display();
	auto EndTime = SDL_GetTicks();
	return EndTime - StartTime;
}
