#include "RacerScreen.h"
#include "..\Audio.h"
#include "..\App\App.h"

using namespace std;
using namespace Game::App;
using namespace Game::Audio;
using namespace Game::Screens;
using namespace Game::Render;
using namespace Game::Race;


Game::Screens::RacerScreen::RacerScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* _RacerToDisplay, RacerScreenStateInitType _InitType) :
	AppScreen(_Machine, _IO, _Data),
	RegenButton(_IO, "Regen", 400, 500, 210, 80),
	ApplyButton(_IO, "Apply", 750, 650, 210, 80),
	ExitButton(_IO, "Exit", 750, 650, 210, 80)
{
	Type = AppScreenType::RacerScreen;
	InitType = _InitType;
	RacerToDisplay = _RacerToDisplay;

	if (InitType == RacerScreenStateInitType::RacerCreation)
		Data.Profile->MainFella.Name = RacerNameMaker().Make();
}

Game::Screens::RacerScreen::~RacerScreen()
{
}

AppScreen* Game::Screens::RacerScreen::Update()
{
	RegenButton.Update();
	if (RegenButton.HasMouseClicked())
	{
		IO.Player.Play(BuiltInSounds::Click);

		Data.Profile->MainFella.Name = RacerNameMaker().Make();
		Data.Profile->MainFella.Skills = RacerSkills();
	}

	if (InitType == RacerScreenStateInitType::RacerCreation)
		ApplyButton.Update();
	if (InitType == RacerScreenStateInitType::ViewOnly)
		ExitButton.Update();
	if ((ApplyButton.HasMouseClicked() || IO.Esc) || (ExitButton.HasMouseClicked() || IO.Esc))
	{
		IO.Player.Play(BuiltInSounds::Click);

		//go back to last screen
		ScreenStack.Pop();
		return ScreenStack.Top();
	}

	return this;
}


Game::Render::RacerScreenRenderer::RacerScreenRenderer(Game::Render::AppRenderContext* Context, Game::Career::CareerProfile* RacerProfile) :
	BaseRenderer(Context), RacerProfile(RacerProfile)
{

}

unsigned int Game::Render::RacerScreenRenderer::Render()
{
	//start timer
	auto StartTime = SDL_GetTicks();

	SDL_SetRenderDrawColor(Context->MainRenderer, 255, 255, 255, 255);
	SDL_RenderClear(Context->MainRenderer);

	SDL_Rect RenderQuad1 = { 100 , 100 , 378 , 359 };
	RenderImage(Context->Store.GetImage(Assets::ImageAssetRaceBackground)->Texture, NULL, &RenderQuad1);

	SDL_Rect RenderQuad2 = { 500 , 100 , 378 , 359 };
	RenderImage(Context->Store.GetImage(Assets::ImageAssetRaceBackground)->Texture, NULL, &RenderQuad2);


	std::string SecondScreen = "Name: " + RacerProfile->MainFella.Name + "\n" + "\n" +
		"Base Speed: " + to_string(RacerProfile->MainFella.Skills.BaseSpeed) + "\n" + "\n" +
		"Standard Velovity: " + to_string(RacerProfile->MainFella.Skills.StandardVelocity) + "\n" + "\n" +
		"Sprint Boost: " + to_string(RacerProfile->MainFella.Skills.Sprint) + "\n" + "\n" +
		"Luck: " + to_string(RacerProfile->MainFella.Skills.Luck) + "% \n" + "\n" + "\n" + "\n" + "\n" + "\n" +
		"Cash: $" + to_string(RacerProfile->Financials.Cash) + "\n" + "\n" ;
	SDL_Color White = { 255,255,255 };
	RenderText(Context->Store.GetFont(Assets::FontAssetMainFont), SecondScreen, 550, 150, { 255,255,255 });

	if (State->InitType == RacerScreenStateInitType::RacerCreation)
	{
		auto MappedRgb = SDL_MapRGB(SDL_GetPixelFormatDetails(Context->MainSurface->format), SDL_GetSurfacePalette(Context->MainSurface), 255,0,0);

		//regen
		SDL_FillSurfaceRect(Context->MainSurface, &State->RegenButton.Rect, MappedRgb);
		SDL_SetRenderDrawColor(Context->MainRenderer, 255, 255, 255, 255);
		SDL_FRect RegenButtonFRect; SDL_RectToFRect(&State->RegenButton.Rect, &RegenButtonFRect);
		SDL_RenderRect(Context->MainRenderer, &RegenButtonFRect);
		RenderText(Context->Store.GetFont(Assets::FontAssetBigFont), State->RegenButton.Text.c_str(), State->RegenButton.x + 5, State->RegenButton.y + 5, State->RegenButton.Color);

		//apply
		SDL_FillSurfaceRect(Context->MainSurface, &State->ApplyButton.Rect, MappedRgb);
		SDL_SetRenderDrawColor(Context->MainRenderer, 255, 255, 255, 255);

		SDL_FRect ApplyButtonFRect; SDL_RectToFRect(&State->ApplyButton.Rect, &ApplyButtonFRect);
		SDL_RenderRect(Context->MainRenderer, &ApplyButtonFRect);
		RenderText(Context->Store.GetFont(Assets::FontAssetBigFont), State->ApplyButton.Text.c_str(), State->ApplyButton.x + 5, State->ApplyButton.y + 5, State->ApplyButton.Color);
	}

	if (State->InitType == RacerScreenStateInitType::ViewOnly)
	{
		auto MappedRgb = SDL_MapRGB(SDL_GetPixelFormatDetails(Context->MainSurface->format), SDL_GetSurfacePalette(Context->MainSurface), 255, 0, 0);
		
		//exit
		SDL_FillSurfaceRect(Context->MainSurface, &State->ExitButton.Rect, MappedRgb);
		SDL_SetRenderDrawColor(Context->MainRenderer, 255, 255, 255, 255);
		SDL_FRect ExitButtonFRect; SDL_RectToFRect(&State->ExitButton.Rect, &ExitButtonFRect);
		SDL_RenderRect(Context->MainRenderer, &ExitButtonFRect);
		RenderText(Context->Store.GetFont(Assets::FontAssetBigFont), State->ExitButton.Text.c_str(), State->ExitButton.x + 5, State->ExitButton.y + 5, State->ExitButton.Color);
	}

	//racer
	auto& Fella = this->RacerProfile->MainFella;
	unsigned int FrameTime = 250 - (Fella.Skills.BaseSpeed * 10);
	unsigned int FrameNum =  ( (SDL_GetTicks() % 1000)  / 250) * (613 / 4);
	SDL_Rect SourceQuad = { 0 + FrameNum, 0, 613 / 4, 186 / 1 };
	SDL_Rect RenderQuad = { 180,  150, 613 / 4 , 186 / 1 };

	RenderImage(Context->Store.GetImage(Assets::ImageAssetFellaRun)->Texture, &SourceQuad, &RenderQuad);	//running

	Display();
	auto EndTime = SDL_GetTicks();
	FPS.AddFrame(EndTime - StartTime);
	return EndTime - StartTime;
}
