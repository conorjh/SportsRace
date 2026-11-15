#include "RacerScreen.h"
#include "..\Audio.h"
#include "..\App\App.h"
#include "..\Render\RaceCurveVisualization.h"

namespace
{
	constexpr int CurvePlotLeft = 520;
	constexpr int CurvePlotTop = 340;
	constexpr int CurvePlotWidth = 338;
	constexpr int CurvePlotHeight = 80;
	constexpr int CurvePlotTitleTop = 318;
	constexpr int CurvePlotLegendTop = 426;
	constexpr int RacerScreenButtonTextLeftPadding = 5;
	constexpr SDL_Color RacerScreenBackgroundColor{ 0, 0, 0, 255 };
	constexpr SDL_Color RacerScreenButtonFillColor{ 255, 0, 0, 255 };
	constexpr SDL_Color RacerScreenButtonBorderColor{ 255, 255, 255, 255 };

	void DrawRacerScreenButton(Game::Render::BaseRenderer& Renderer, Game::Render::AppRenderContext* Context,
		const Game::GUI::Button& Button)
	{
		SDL_FRect ButtonBounds;
		SDL_RectToFRect(&Button.Rect, &ButtonBounds);

		SDL_SetRenderDrawColor(Context->MainRenderer, RacerScreenButtonFillColor.r, RacerScreenButtonFillColor.g,
			RacerScreenButtonFillColor.b, RacerScreenButtonFillColor.a);
		SDL_RenderFillRect(Context->MainRenderer, &ButtonBounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, RacerScreenButtonBorderColor.r, RacerScreenButtonBorderColor.g,
			RacerScreenButtonBorderColor.b, RacerScreenButtonBorderColor.a);
		SDL_RenderRect(Context->MainRenderer, &ButtonBounds);

		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetBigFont), Button.Text,
			Button.x + RacerScreenButtonTextLeftPadding, Button.y + RacerScreenButtonTextLeftPadding, Button.Color);
	}
}

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

		Racer Generated(RacerNameMaker().Make());
		Generated.GUID = Data.Profile->MainFella.GUID;
		Data.Profile->MainFella = Generated;
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

	SDL_SetRenderDrawColor(Context->MainRenderer, RacerScreenBackgroundColor.r, RacerScreenBackgroundColor.g,
		RacerScreenBackgroundColor.b, RacerScreenBackgroundColor.a);
	SDL_RenderClear(Context->MainRenderer);

	SDL_Rect RenderQuad1 = { 100 , 100 , 378 , 359 };
	RenderImage(Context->Store.GetImage(Assets::ImageAssetRaceBackground)->Texture, NULL, &RenderQuad1);

	SDL_Rect RenderQuad2 = { 500 , 100 , 378 , 359 };
	RenderImage(Context->Store.GetImage(Assets::ImageAssetRaceBackground)->Texture, NULL, &RenderQuad2);


	auto& Runner = RacerProfile->MainFella;
	std::string SecondScreen = "Name: " + Runner.Name + "\n" +
		"Reflex: " + to_string(static_cast<int>(Runner.Skills.Reflex)) + "\n" +
		"Composure: " + to_string(static_cast<int>(Runner.Skills.Composure)) + "\n" +
		"Acceleration: " + to_string(static_cast<int>(Runner.Skills.Acceleration)) + "\n" +
		"Endurance: " + to_string(static_cast<int>(Runner.Skills.Endurance)) + "\n" +
		"Speed: " + to_string(static_cast<int>(Runner.Skills.Speed)) + "\n" +
		"Height: " + to_string(static_cast<int>(Runner.Details.HeightCm)) + " cm\n" +
		"Weight: " + to_string(static_cast<int>(Runner.Details.WeightKg)) + " kg\n" +
		"Age: " + to_string(Runner.Details.Age) + "\n" +
		"Fatigue: " + to_string(static_cast<int>(Runner.Conditions.Fatigue * 100)) + "%\n" +
		"Confidence: " + to_string(static_cast<int>(Runner.Conditions.Confidence * 100)) + "%\n" +
		"Injury: " + to_string(static_cast<int>(Runner.Conditions.Injury * 100)) + "%\n" +
		"Cash: $" + to_string(RacerProfile->Financials.Cash);
	RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), SecondScreen, 550, 150, { 255,255,255 });
	const SDL_FRect CurvePlotBounds{ static_cast<float>(CurvePlotLeft), static_cast<float>(CurvePlotTop),
		static_cast<float>(CurvePlotWidth), static_cast<float>(CurvePlotHeight) };
	DrawRacerCurveVisualization(*this, Context, Runner, CurvePlotBounds,
		CurvePlotLeft, CurvePlotTitleTop, CurvePlotLegendTop);

	DrawRacerScreenButton(*this, Context, Screen->RegenButton);
	if (Screen->InitType == Screens::RacerScreenStateInitType::RacerCreation)
	{
		DrawRacerScreenButton(*this, Context, Screen->ApplyButton);
	}

	if (Screen->InitType == Screens::RacerScreenStateInitType::ViewOnly)
	{
		DrawRacerScreenButton(*this, Context, Screen->ExitButton);
	}

	//racer
	unsigned int FrameNum =  ( (SDL_GetTicks() % 1000)  / 250) * (613 / 4);
	SDL_Rect SourceQuad = { 0 + FrameNum, 0, 613 / 4, 186 / 1 };
	SDL_Rect RenderQuad = { 180,  150, 613 / 4 , 186 / 1 };

	RenderImage(Context->Store.GetImage(Assets::ImageAssetFellaRun)->Texture, &SourceQuad, &RenderQuad);	//running

	Display();
	auto EndTime = SDL_GetTicks();
	return EndTime - StartTime;
}
