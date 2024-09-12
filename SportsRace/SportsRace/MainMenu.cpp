#include "MainMenu.h"
#include "RaceState.h"
#include "CareerHub.h"
#include "RaceScreen.h"
#include "RacerScreen.h"
#include "SDL3/SDL.h"
#include "SDL3_mixer/SDL_mixer.h"

using namespace std;
using namespace Game;
using namespace Game::Screens;
using namespace Game::App;
using namespace Game::Audio;
using namespace Game::Race;
using namespace Game::Career;


bool Game::Renderer::MainMenuRendererData::Load(Render::BaseRenderer& Renderer)
{
	Renderer.LoadImageFile("screen.png", Background);
	MainFont = TTF_OpenFont("menu_font.ttf", 96);

	return true;
}

Game::Screens::MainMenuScreen::MainMenuScreen(AppScreenStateMachine& _Machine, AppIO& _IO, AppData& _Data) 
	: AppScreen(_Machine, _IO, _Data),
	RaceButton(_IO, "Race", 100, 100, 210, 80),
	CareerButton(_IO, "Career", 100, 200, 290, 80),
	ExitButton(_IO, "Exit", 100, 400, 180, 80)
{
	Type = AppScreenType::MainMenu;

	for (int t = 0; t < 6; ++t)
		RaceBuffer.Racers.push_back(Racers.Make(RacerNameMaker().Make()));
}

Game::Screens::MainMenuScreen::~MainMenuScreen()
{

}

AppScreen* Game::Screens::MainMenuScreen::Update()
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
		//check if we dont have a saved profile
		if (Data.Profile == nullptr)
		{
			//create a new profile + career
			Data.Profile = new CareerProfile();
			Data.Career = new CareerData();

			//call character creation screen
			ScreenStack.Push(new CareerHubScreen(ScreenStack, IO, Data));
			ScreenStack.Push(new RacerScreen(ScreenStack, IO, Data, &Data.Profile->MainFella, RacerScreenStateInitType::RacerCreation));
			return ScreenStack.Top();
		}

		//run career hub with it
		ScreenStack.Push(new CareerHubScreen(ScreenStack, IO, Data));
		return ScreenStack.Top();
	}

	ExitButton.Update();
	if (ExitButton.HasMouseClicked())
		Data.Halted = true;

	return this;
}



Game::Renderer::MainMenuRenderer::MainMenuRenderer(AppData* _Data, MainMenuRendererData* _RenData, Render::BaseRendererData* _BaseData) 
	: RendererData(_RenData), BaseRenderer(_Data, _BaseData)
{
	State = nullptr;
}

Game::Renderer::MainMenuRenderer::MainMenuRenderer(AppData* _Data, MainMenuScreen* _State, MainMenuRendererData* _RenData, Render::BaseRendererData* _BaseData) 
	: State(_State), RendererData(_RenData), BaseRenderer(_Data, _BaseData)
{

}


void Game::Renderer::MainMenuRenderer::DrawLeague()
{
	SDL_Color White = { 255, 255, 255 };
	SDL_Rect RenderQuad2 = { 500 , 100 , 378 , 359 };
	RenderImage(RendererData->Background.Texture, NULL, &RenderQuad2);

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

unsigned int Game::Renderer::MainMenuRenderer::Render()
{
	//start timer
	auto StartTime = SDL_GetTicks();

	SDL_SetRenderDrawColor(Data->RenderData.MainRenderer, 255, 255, 255, 255);
	SDL_RenderClear(Data->RenderData.MainRenderer);
	auto RedColor = SDL_MapRGB(SDL_GetPixelFormatDetails(Data->RenderData.MainSurface->format), SDL_GetSurfacePalette(Data->RenderData.MainSurface), 255, 0, 0);

	//race button
	SDL_FillSurfaceRect(Data->RenderData.MainSurface, &State->RaceButton.Rect, RedColor);
	SDL_SetRenderDrawColor(Data->RenderData.MainRenderer, 255, 255, 255, 255);
	SDL_FRect RaceButtonFRect; SDL_RectToFRect(&State->RaceButton.Rect, &RaceButtonFRect);
	SDL_RenderRect(Data->RenderData.MainRenderer, &RaceButtonFRect);
	RenderText(RendererData->MainFont, State->RaceButton.Text.c_str(), State->RaceButton.x + 5, State->RaceButton.y + 5, State->RaceButton.Color);

	//career button
	SDL_FillSurfaceRect(Data->RenderData.MainSurface, &State->CareerButton.Rect, RedColor);
	SDL_SetRenderDrawColor(Data->RenderData.MainRenderer, 255, 255, 255, 255);
	SDL_FRect CareerButtonFRect; SDL_RectToFRect(&State->CareerButton.Rect, &CareerButtonFRect);
	SDL_RenderRect(Data->RenderData.MainRenderer, &CareerButtonFRect);
	RenderText(RendererData->MainFont, State->CareerButton.Text.c_str(), State->CareerButton.x + 5, State->CareerButton.y + 5, State->CareerButton.Color);

	//exit button
	SDL_FillSurfaceRect(Data->RenderData.MainSurface, &State->ExitButton.Rect, RedColor);
	SDL_SetRenderDrawColor(Data->RenderData.MainRenderer, 255, 255, 255, 255);
	SDL_FRect ExitButtonFRect; SDL_RectToFRect(&State->ExitButton.Rect, &ExitButtonFRect);
	SDL_RenderRect(Data->RenderData.MainRenderer, &ExitButtonFRect);
	RenderText(RendererData->MainFont, State->ExitButton.Text.c_str(), State->ExitButton.x + 5, State->ExitButton.y + 5, State->ExitButton.Color);

	//League standings
	DrawLeague();

	//fps
	if (Data->ShowFPS)
		RenderText(BaseData->DebugFont, "FPS: " + to_string(FPS.GetFrameTime()), 975, 5, { 255,255,255 });

	Display();

	auto EndTime = SDL_GetTicks();
	FPS.AddFrame(EndTime - StartTime);

	return EndTime - StartTime;
}