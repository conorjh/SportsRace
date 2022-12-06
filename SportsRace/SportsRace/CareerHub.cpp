#include "CareerHub.h"
#include "Audio.h"
#include "RaceState.h"
#include "RaceScreen.h"
#include "RacerScreen.h"
#include "RankingScreen.h"
#include "SDL2\SDL.h"

using namespace std;
using namespace Game::App;
using namespace Game::Audio;
using namespace Game::Screens;
using namespace Game::GUI;
using namespace Game::Race;
using namespace Game::Career;

bool Game::Renderer::CareerHubRendererData::Load(Render::BaseRenderer& Renderer)
{
	Renderer.LoadImageFile("race_icon.png", RaceIconGraphic);
	Renderer.LoadImageFile("racer_icon.png", RacerIconGraphic);
	Renderer.LoadImageFile("training_icon.png", TrainingIconGraphic);
	Renderer.LoadImageFile("ranking_icon.png", RankingIconGraphic);

	return true;
}

Game::Screens::CareerHubScreen::CareerHubScreen(AppScreenStateMachine& _Machine, AppIO& _IO, AppData& _Data) :
	AppScreen(_Machine, _IO, _Data),
	Orchestrator(_Data.Career, _Data.Profile),

	RaceIcon(_IO, IconButtonType::Race, 700, 450, 200, 200),
	TrainingIcon(_IO, IconButtonType::Training, 700, 50, 200, 200),
	RankingIcon(_IO, IconButtonType::Ranking, 100, 450, 200, 200),
	RacerIcon(_IO, IconButtonType::Racer, 100, 50, 200, 200)
{
	Type = AppScreenType::CareerHub;

}

Game::Screens::CareerHubScreen::~CareerHubScreen()
{

}

void Game::Screens::CareerHubScreen::Entry()
{
	IO.Player.Play(Soundtrack::Catering);
}

void Game::Screens::CareerHubScreen::Exit()
{
	IO.Player.StopMusic();
}


AppScreen* Game::Screens::CareerHubScreen::Update()
{
	RaceIcon.Update();
	if (RaceIcon.HasMouseClicked())
	{
		Race::Race* RaceBuffer = new Race::Race();
		RaceBuffer->Racers.push_back(&Orchestrator.Profile->MainFella);
		for (int t = 0; t < 5; ++t)
			RaceBuffer->Racers.push_back(Orchestrator.Data->Racers.Make(RacerNameMaker().Make()));
		RaceBuffer->Financials = RaceFinancials(unsigned int(500 * (rand() % 10)), 50 * (rand() % 4));
		
		//actually run our race
		ParentMachine.Push(new RaceScreen(ParentMachine, IO, Data, *RaceBuffer, Orchestrator.Profile->MainFella.GUID));
		return ParentMachine.Top();
	}

	TrainingIcon.Update();
	if (TrainingIcon.HasMouseClicked())
	{
		RacerDB* DB = new RacerDB();

		//actually run our race
		ParentMachine.Push(new RaceScreen(ParentMachine, IO, Data, &Orchestrator.Profile->MainFella));
		return ParentMachine.Top();
	}

	RacerIcon.Update();
	if (RacerIcon.HasMouseClicked())
	{
		//actually run our race
		ParentMachine.Push(new RacerScreen(ParentMachine, IO, Data, &Orchestrator.Profile->MainFella, RacerScreenStateInitType::ViewOnly));
		return ParentMachine.Top();
	}

	RankingIcon.Update();
	if (RankingIcon.HasMouseClicked())
	{
		//actually run our race
		ParentMachine.Push(new RankingScreen(ParentMachine, IO, Data, &Orchestrator.Profile->MainFella));
		return ParentMachine.Top();
	}

	if (IO.Esc)
	{
		IO.Player.Play(BuiltInSounds::Click);

		ParentMachine.Pop();
		return ParentMachine.Top();
	}

	

	return this;
}


Game::Renderer::RacerScreenRenderer::RacerScreenRenderer(AppData* _Data, RacerScreenRendererData* _RendererData, Render::BaseRendererData* _BaseData) :
	RendererData(_RendererData), BaseRenderer(_Data, _BaseData)
{

}


Game::Renderer::CareerHubRenderer::CareerHubRenderer(AppData* _Data, CareerHubScreen* _State, CareerHubRendererData* _RendererData, Render::BaseRendererData* _BaseData) :
	State(_State), RendererData(_RendererData), BaseRenderer(_Data, _BaseData)
{
}

Game::Renderer::CareerHubRenderer::CareerHubRenderer(AppData* _Data, CareerHubRendererData* _RendererData, Render::BaseRendererData* _BaseData) :
	RendererData(_RendererData), BaseRenderer(_Data, _BaseData)
{
}

unsigned int Game::Renderer::CareerHubRenderer::Render()
{
	//start timerv=
	auto StartTime = SDL_GetTicks();

	SDL_SetRenderDrawColor(Data->RenderData.MainRenderer, 255, 255, 255, 255);
	SDL_RenderClear(Data->RenderData.MainRenderer);

	if (State->RacerIcon.IsMouseOver())
	{
		SDL_Rect RenderQuad1 = State->RacerIcon.Rect;
		RenderQuad1.x += 7;
		RenderQuad1.y -= 7;
		RenderImage(RendererData->RacerIconGraphic.Texture, NULL, &RenderQuad1);
	}
	else
		RenderImage(RendererData->RacerIconGraphic.Texture, NULL, &State->RacerIcon.Rect);

	if (State->TrainingIcon.IsMouseOver())
	{
		SDL_Rect RenderQuad2 = State->TrainingIcon.Rect;
		RenderQuad2.x += 7;
		RenderQuad2.y -= 7;
		RenderImage(RendererData->TrainingIconGraphic.Texture, NULL, &RenderQuad2);
	}
	else
		RenderImage(RendererData->TrainingIconGraphic.Texture, NULL, &State->TrainingIcon.Rect);

	if (State->RaceIcon.IsMouseOver())
	{
		SDL_Rect RenderQuad3 = State->RaceIcon.Rect;
		RenderQuad3.x += 7;
		RenderQuad3.y -= 7;
		RenderImage(RendererData->RaceIconGraphic.Texture, NULL, &RenderQuad3);
	}
	else
		RenderImage(RendererData->RaceIconGraphic.Texture, NULL, &State->RaceIcon.Rect);

	if (State->RankingIcon.IsMouseOver())
	{
		SDL_Rect RenderQuad3 = State->RankingIcon.Rect;
		RenderQuad3.x += 7;
		RenderQuad3.y -= 7;
		RenderImage(RendererData->RankingIconGraphic.Texture, NULL, &RenderQuad3);
	}
	else
		RenderImage(RendererData->RankingIconGraphic.Texture, NULL, &State->RankingIcon.Rect);

	RenderText(BaseData->MainFont, "Cash: $" + to_string(Data->Profile->Financials.Cash), 450,720, {0,0,0});

	Display();
	auto EndTime = SDL_GetTicks();
	FPS.AddFrame(EndTime - StartTime);
	return EndTime - StartTime;
}

