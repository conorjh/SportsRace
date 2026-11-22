#include "MainMenu.h"
#include "..\RaceState.h"
#include "CareerHub.h"
#include "RaceScreen.h"
#include "RacerScreen.h"
#include "OptionsScreen.h"
#include "SDL3/SDL.h"
#include "SDL3_mixer/SDL_mixer.h"
#include "..\App\App.h"

Game::Screens::MainMenuScreen::MainMenuScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data)
	: AppScreen(_Machine, _IO, _Data),
	RaceButton(_IO, "Race", 100, 100, 210, 80),
	CareerButton(_IO, "Career", 100, 200, 290, 80),
	OptionsButton(_IO, "Options", 100, 300, 210, 80),
	ExitButton(_IO, "Exit", 100, 400, 180, 80)
{
	Type = AppScreenType::MainMenu;

	// Keep a small generated field ready for the standalone race option.
	for (int t = 0; t < 6; ++t)
		RaceBuffer.Racers.push_back(Racers.Make(Race::RacerNameMaker().Make()));
}

Game::Screens::MainMenuScreen::~MainMenuScreen()
{

}

Game::Screens::AppScreen* Game::Screens::MainMenuScreen::Update()
{
	RaceButton.Update();
	if (RaceButton.HasMouseClicked())
	{
		RaceBuffer.Reset();

		//actually run our race
		ScreenStack.Push(new RaceScreen(ScreenStack, IO, Data, RaceBuffer));
		return ScreenStack.Top();
	}

	CareerButton.Update();
	if (CareerButton.HasMouseClicked())
	{
		if (Data.Profile == nullptr)
			Data.Profile = new Career::CareerProfile();
		if (Data.Career == nullptr)
			Data.Career = new Career::CareerData();
		const bool IsNewCareer = Data.Career->CityPlots.empty();
		ScreenStack.Push(new CareerHubScreen(ScreenStack, IO, Data, IsNewCareer));
		return ScreenStack.Top();
	}

	OptionsButton.Update();
	if (OptionsButton.HasMouseClicked())
	{
		ScreenStack.Push(new OptionsScreen(ScreenStack, IO, Data));
		return ScreenStack.Top();
	}

	ExitButton.Update();
	if (ExitButton.HasMouseClicked())
		Data.Halted = true;

	return this;
}



Game::Render::MainMenuRenderer::MainMenuRenderer(Game::Render::AppRenderContext* RenderContext)
	: BaseRenderer(RenderContext),
	Screen()
{}

Game::Render::MainMenuRenderer::MainMenuRenderer(Game::Render::AppRenderContext* RenderContext, Game::Screens::MainMenuScreen* State)
	: BaseRenderer(RenderContext), 
	Screen(State)
{}


void Game::Render::MainMenuRenderer::DrawLeague()
{
	SDL_Color White = { 255, 255, 255 };
	SDL_Rect RenderQuad2 = { 500 , 100 , 378 , 359 };
	RenderImage(Context->Store.GetImage(Assets::ImageAssetMainMenuBackground)->Texture, NULL, &RenderQuad2);

	unsigned int OtherStandings = 4;

	/*
	auto ranl = Data->League.Standings.Get(Data->League.MainGuy).Rank;
	int TopEnd = Data->League.Standings.Get(Data->League.MainGuy).Rank - 4;
	if (TopEnd < 0)
		TopEnd = 0;
	int BottomEnd = Data->League.Standings.Get(Data->League.MainGuy).Rank + 4;
	if (BottomEnd >= Data->League.Standings.Entries.size())
		BottomEnd = int(Data->League.Standings.Entries.size() - 1);


	for (int t = 0; t <= BottomEnd - TopEnd; ++t)
	{
		auto Standing = Data->League.Standings.Get(TopEnd + t);
		if (Standing.IsNull())
			continue;

		RenderText(BaseData->InfoFont, to_string(Standing.Rank) + "  " + Standing.Racer->Name + ": " + to_string(Standing.Points) + "pts ", 600, (t * 32) + 133, White);

	}
	*/
}

unsigned int Game::Render::MainMenuRenderer::Render()
{
	//start timer
	auto StartTime = SDL_GetTicks();

	SDL_SetRenderDrawColor(Context->MainRenderer, 255, 255, 255, 255);
	SDL_RenderClear(Context->MainRenderer);
	auto RedColor = SDL_MapRGB(SDL_GetPixelFormatDetails(Context->MainSurface->format), SDL_GetSurfacePalette(Context->MainSurface), 255, 0, 0);

	//race button
	SDL_FillSurfaceRect(Context->MainSurface, &Screen->RaceButton.Rect, RedColor);
	SDL_SetRenderDrawColor(Context->MainRenderer, 255, 255, 255, 255);
	SDL_FRect RaceButtonFRect; SDL_RectToFRect(&Screen->RaceButton.Rect, &RaceButtonFRect);
	SDL_RenderRect(Context->MainRenderer, &RaceButtonFRect);
	RenderText(Context->Store.GetFont(Assets::FontAssetMainMenuFont), Screen->RaceButton.Text.c_str(), Screen->RaceButton.x + 5, Screen->RaceButton.y + 5, Screen->RaceButton.Color);

	//career button
	SDL_FillSurfaceRect(Context->MainSurface, &Screen->CareerButton.Rect, RedColor);
	SDL_SetRenderDrawColor(Context->MainRenderer, 255, 255, 255, 255);
	SDL_FRect CareerButtonFRect; SDL_RectToFRect(&Screen->CareerButton.Rect, &CareerButtonFRect);
	SDL_RenderRect(Context->MainRenderer, &CareerButtonFRect);
	RenderText(Context->Store.GetFont(Assets::FontAssetMainMenuFont), Screen->CareerButton.Text.c_str(), Screen->CareerButton.x + 5, Screen->CareerButton.y + 5, Screen->CareerButton.Color);

	// Options button
	SDL_FillSurfaceRect(Context->MainSurface, &Screen->OptionsButton.Rect, RedColor);
	SDL_SetRenderDrawColor(Context->MainRenderer, 255, 255, 255, 255);
	SDL_FRect OptionsButtonFRect; SDL_RectToFRect(&Screen->OptionsButton.Rect, &OptionsButtonFRect);
	SDL_RenderRect(Context->MainRenderer, &OptionsButtonFRect);
	RenderText(Context->Store.GetFont(Assets::FontAssetMainMenuFont), Screen->OptionsButton.Text.c_str(),
		Screen->OptionsButton.x + 5, Screen->OptionsButton.y + 5, Screen->OptionsButton.Color);

	//exit button
	SDL_FillSurfaceRect(Context->MainSurface, &Screen->ExitButton.Rect, RedColor);
	SDL_SetRenderDrawColor(Context->MainRenderer, 255, 255, 255, 255);
	SDL_FRect ExitButtonFRect; SDL_RectToFRect(&Screen->ExitButton.Rect, &ExitButtonFRect);
	SDL_RenderRect(Context->MainRenderer, &ExitButtonFRect);
	RenderText(Context->Store.GetFont(Assets::FontAssetMainMenuFont), Screen->ExitButton.Text.c_str(), Screen->ExitButton.x + 5, Screen->ExitButton.y + 5, Screen->ExitButton.Color);

	//League standings
	DrawLeague();

	//fps
	RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), "FPS: " + std::to_string(FPS.GetFPS()), 975, 5, { 255,255,255 });

	Display();

	auto EndTime = SDL_GetTicks();

	return EndTime - StartTime;
}