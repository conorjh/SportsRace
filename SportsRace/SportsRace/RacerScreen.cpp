#include "RacerScreen.h"
#include "Audio.h"

using namespace std;
using namespace Game::App;
using namespace Game::Audio;
using namespace Game::States;
using namespace Game::Renderer;
using namespace Game::Race;

bool Game::Renderer::RacerScreenRendererData::Load(Render::BaseRenderer& Renderer)
{
	Renderer.LoadImageFile("reido.png", Head);
	Renderer.LoadImageFile("track.png", TrackGraphic);
	Renderer.LoadImageFile("fellarun.png", FellaRun);
	Renderer.LoadImageFile("fellawait.png", FellaWait);
	Renderer.LoadImageFile("screen.png", Screen);

	return true;
}

Game::States::RacerScreenState::RacerScreenState(AppStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* _RacerToDisplay, RacerScreenStateInitType _InitType) :
	AppState(_Machine, _IO, _Data),
	RegenButton(_IO, "Regen", 400, 500, 210, 80),
	ApplyButton(_IO, "Apply", 750, 650, 210, 80),
	ExitButton(_IO, "Exit", 750, 650, 210, 80)
{
	Type = AppStateType::RacerScreen;
	InitType = _InitType;
	RacerToDisplay = _RacerToDisplay;

	if (InitType == RacerScreenStateInitType::RacerCreation)
		Data.Profile->MainFella.Name = RacerNameMaker().Make();
}

Game::States::RacerScreenState::~RacerScreenState()
{
}

AppState* Game::States::RacerScreenState::Update()
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
		Machine.Pop();
		return Machine.Top();
	}

	return this;
}



unsigned int Game::Renderer::RacerScreenRenderer::Render()
{
	//start timer
	auto StartTime = SDL_GetTicks();

	SDL_SetRenderDrawColor(Data->RenderData.MainRenderer, 255, 255, 255, 255);
	SDL_RenderClear(Data->RenderData.MainRenderer);

	SDL_Rect RenderQuad1 = { 100 , 100 , 378 , 359 };
	RenderImage(RendererData->Screen.Texture, NULL, &RenderQuad1);

	SDL_Rect RenderQuad2 = { 500 , 100 , 378 , 359 };
	RenderImage(RendererData->Screen.Texture, NULL, &RenderQuad2);


	std::string SecondScreen = "Name: " + Data->Profile->MainFella.Name + "\n" + "\n" +
		"Base Speed: " + to_string(Data->Profile->MainFella.Skills.BaseSpeed) + "\n" + "\n" +
		"Standard Velovity: " + to_string(Data->Profile->MainFella.Skills.StandardVelocity) + "\n" + "\n" +
		"Sprint Boost: " + to_string(Data->Profile->MainFella.Skills.Sprint) + "\n" + "\n" +
		"Luck: " + to_string(Data->Profile->MainFella.Skills.Luck) + "% \n" + "\n" + "\n" + "\n" + "\n" + "\n" +
		"Cash: $" + to_string(Data->Profile->Financials.Cash) + "\n" + "\n" ;
	SDL_Color White = { 255,255,255 };
	RenderText(BaseData->MainFont, SecondScreen, 550, 150, { 255,255,255 });

	if (State->InitType == RacerScreenStateInitType::RacerCreation)
	{
		//regen
		SDL_FillRect(Data->RenderData.MainSurface, &State->RegenButton.Rect, SDL_MapRGB(Data->RenderData.MainSurface->format, 255, 0, 0));
		SDL_SetRenderDrawColor(Data->RenderData.MainRenderer, 255, 255, 255, 255);
		SDL_RenderDrawRect(Data->RenderData.MainRenderer, &State->RegenButton.Rect);
		RenderText(BaseData->BigFont, State->RegenButton.Text.c_str(), State->RegenButton.x + 5, State->RegenButton.y + 5, State->RegenButton.Color);

		//apply
		SDL_FillRect(Data->RenderData.MainSurface, &State->ApplyButton.Rect, SDL_MapRGB(Data->RenderData.MainSurface->format, 255, 0, 0));
		SDL_SetRenderDrawColor(Data->RenderData.MainRenderer, 255, 255, 255, 255);
		SDL_RenderDrawRect(Data->RenderData.MainRenderer, &State->ApplyButton.Rect);
		SDL_RenderDrawRect(Data->RenderData.MainRenderer, &State->ApplyButton.Rect);
		RenderText(BaseData->BigFont, State->ApplyButton.Text.c_str(), State->ApplyButton.x + 5, State->ApplyButton.y + 5, State->ApplyButton.Color);
	}

	if (State->InitType == RacerScreenStateInitType::ViewOnly)
	{
		//exit
		SDL_FillRect(Data->RenderData.MainSurface, &State->ExitButton.Rect, SDL_MapRGB(Data->RenderData.MainSurface->format, 255, 0, 0));
		SDL_SetRenderDrawColor(Data->RenderData.MainRenderer, 255, 255, 255, 255);
		SDL_RenderDrawRect(Data->RenderData.MainRenderer, &State->ExitButton.Rect);
		RenderText(BaseData->BigFont, State->ExitButton.Text.c_str(), State->ExitButton.x + 5, State->ExitButton.y + 5, State->ExitButton.Color);
	}

	//racer
	auto& Fella = this->Data->Profile->MainFella;
	unsigned int FrameTime = 250 - (Fella.Skills.BaseSpeed * 10);
	unsigned int FrameNum =  ( (SDL_GetTicks() % 1000)  / 250) * (613 / 4);
	SDL_Rect SourceQuad = { 0 + FrameNum, 0, 613 / 4, 186 / 1 };
	SDL_Rect RenderQuad = { 180,  150, 613 / 4 , 186 / 1 };

	RenderImage(RendererData->FellaRun.Texture, &SourceQuad, &RenderQuad);	//running

	Display();
	auto EndTime = SDL_GetTicks();
	FPS.AddFrame(EndTime - StartTime);
	return EndTime - StartTime;
}
