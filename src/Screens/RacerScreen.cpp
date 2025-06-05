#include "RacerScreen.h"
#include "..\Audio.h"
#include "..\App\App.h"

Game::Screens::RacerScreen::RacerScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* _RacerToDisplay, RacerScreenStateInitType _InitType) :
	AppScreen(_Machine, _IO, _Data),
	RegenButton(_IO, "Regen", 400, 500, 210, 80),
	ApplyButton(_IO, "Apply", 750, 650, 210, 80),
	ExitButton(_IO, "Exit", 750, 650, 210, 80),
	InitType(_InitType),
	RacerToDisplay(_RacerToDisplay)
{
	Type = AppScreenType::RacerScreen;
	if (InitType == RacerScreenStateInitType::RacerCreation)
		Data.Profile->MainFella.Name = Race::RacerNameMaker().Make();
}

Game::Screens::RacerScreen::~RacerScreen()
{
}

Game::Screens::AppScreen* Game::Screens::RacerScreen::Update()
{
	using namespace Game::Race;
	using namespace Game::Audio;

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


Game::Render::RacerScreenRenderer::RacerScreenRenderer(Game::Render::AppRenderContext* Context, Game::Career::CareerProfile* RacerProfile)
	: BaseRenderer(Context),
	RacerProfile(RacerProfile),
	Screen()
{

}

unsigned int Game::Render::RacerScreenRenderer::Render()
{
	using namespace std;

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

	if (Screen->InitType == Screens::RacerScreenStateInitType::RacerCreation)
	{
		auto MappedRgb = SDL_MapRGB(SDL_GetPixelFormatDetails(Context->MainSurface->format), SDL_GetSurfacePalette(Context->MainSurface), 255,0,0);

		//regen
		SDL_FillSurfaceRect(Context->MainSurface, &Screen->RegenButton.Rect, MappedRgb);
		SDL_SetRenderDrawColor(Context->MainRenderer, 255, 255, 255, 255);
		SDL_FRect RegenButtonFRect; SDL_RectToFRect(&Screen->RegenButton.Rect, &RegenButtonFRect);
		SDL_RenderRect(Context->MainRenderer, &RegenButtonFRect);
		RenderText(Context->Store.GetFont(Assets::FontAssetBigFont), Screen->RegenButton.Text.c_str(), Screen->RegenButton.x + 5, Screen->RegenButton.y + 5, Screen->RegenButton.Color);

		//apply
		SDL_FillSurfaceRect(Context->MainSurface, &Screen->ApplyButton.Rect, MappedRgb);
		SDL_SetRenderDrawColor(Context->MainRenderer, 255, 255, 255, 255);

		SDL_FRect ApplyButtonFRect; SDL_RectToFRect(&Screen->ApplyButton.Rect, &ApplyButtonFRect);
		SDL_RenderRect(Context->MainRenderer, &ApplyButtonFRect);
		RenderText(Context->Store.GetFont(Assets::FontAssetBigFont), Screen->ApplyButton.Text.c_str(), Screen->ApplyButton.x + 5, Screen->ApplyButton.y + 5, Screen->ApplyButton.Color);
	}

	if (Screen->InitType == Screens::RacerScreenStateInitType::ViewOnly)
	{
		auto MappedRgb = SDL_MapRGB(SDL_GetPixelFormatDetails(Context->MainSurface->format), SDL_GetSurfacePalette(Context->MainSurface), 255, 0, 0);
		
		//exit
		SDL_FillSurfaceRect(Context->MainSurface, &Screen->ExitButton.Rect, MappedRgb);
		SDL_SetRenderDrawColor(Context->MainRenderer, 255, 255, 255, 255);
		SDL_FRect ExitButtonFRect; SDL_RectToFRect(&Screen->ExitButton.Rect, &ExitButtonFRect);
		SDL_RenderRect(Context->MainRenderer, &ExitButtonFRect);
		RenderText(Context->Store.GetFont(Assets::FontAssetBigFont), Screen->ExitButton.Text.c_str(), Screen->ExitButton.x + 5, Screen->ExitButton.y + 5, Screen->ExitButton.Color);
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
