#include "CareerHub.h"
#include "..\Audio.h"
#include "..\RaceState.h"
#include "RaceScreen.h"
#include "RacerScreen.h"
#include "RankingScreen.h"
#include "SDL3\SDL.h"
#include "..\App\App.h"


Game::Screens::CareerHubScreen::CareerHubScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data)
	: AppScreen(_Machine, _IO, _Data),
	Orchestrator(_Data.Career, _Data.Profile),
	RaceIcon(_IO, GUI::IconButtonType::Race, 700, 450, 200, 200),
	TrainingIcon(_IO, GUI::IconButtonType::Training, 700, 50, 200, 200),
	RankingIcon(_IO, GUI::IconButtonType::Ranking, 100, 450, 200, 200),
	RacerIcon(_IO, GUI::IconButtonType::Racer, 100, 50, 200, 200)
{
	Type = AppScreenType::CareerHub;
}

Game::Screens::CareerHubScreen::~CareerHubScreen()
{

}

void Game::Screens::CareerHubScreen::Entry()
{
	IO.Player.Play(Audio::Soundtrack::Catering);
}

void Game::Screens::CareerHubScreen::Exit()
{
	IO.Player.StopMusic();
}


Game::Screens::AppScreen* Game::Screens::CareerHubScreen::Update()
{
	using namespace Game::Race;

	RaceIcon.Update();
	if (RaceIcon.HasMouseClicked())
	{
		Game::Race::Race* RaceBuffer = new Game::Race::Race();
		RaceBuffer->Racers.push_back(&Orchestrator.Profile->MainFella);
		for (int t = 0; t < 5; ++t)
			RaceBuffer->Racers.push_back(Orchestrator.Data->Racers.Make(RacerNameMaker().Make()));
		RaceBuffer->Financials = RaceFinancials(unsigned int(500 * (rand() % 10)), 50 * (rand() % 4));
		
		//actually run our race
		ScreenStack.Push(new RaceScreen(ScreenStack, IO, Data, *RaceBuffer, Orchestrator.Profile->MainFella.GUID));
		return ScreenStack.Top();
	}

	TrainingIcon.Update();
	if (TrainingIcon.HasMouseClicked())
	{
		RacerDB* DB = new RacerDB();

		//actually run our race
		ScreenStack.Push(new RaceScreen(ScreenStack, IO, Data, &Orchestrator.Profile->MainFella));
		return ScreenStack.Top();
	}

	RacerIcon.Update();
	if (RacerIcon.HasMouseClicked())
	{
		//actually run our race
		ScreenStack.Push(new RacerScreen(ScreenStack, IO, Data, &Orchestrator.Profile->MainFella, RacerScreenStateInitType::ViewOnly));
		return ScreenStack.Top();
	}

	RankingIcon.Update();
	if (RankingIcon.HasMouseClicked())
	{
		//actually run our race
		ScreenStack.Push(new RankingScreen(ScreenStack, IO, Data, &Orchestrator.Profile->MainFella));
		return ScreenStack.Top();
	}

	if (IO.Esc)
	{
		IO.Player.Play(Audio::BuiltInSounds::Click);

		ScreenStack.Pop();
		return ScreenStack.Top();
	}	

	return this;
}



Game::Render::CareerHubRenderer::CareerHubRenderer(Game::Render::AppRenderContext* Context, Screens::CareerHubScreen* _State) 
	: Screen(_State), 
	BaseRenderer(Context)
{
}

Game::Render::CareerHubRenderer::CareerHubRenderer(Game::Render::AppRenderContext* Context) 
	: Screen(), 
	BaseRenderer(Context)
{
}

unsigned int Game::Render::CareerHubRenderer::Render()
{
	//start timerv=
	auto StartTime = SDL_GetTicks();

	SDL_SetRenderDrawColor(Context->MainRenderer, 255, 255, 255, 255);
	SDL_RenderClear(Context->MainRenderer);

	if (Screen->RacerIcon.IsMouseOver())
	{
		SDL_Rect RenderQuad1 = Screen->RacerIcon.Rect;
		RenderQuad1.x += 7;
		RenderQuad1.y -= 7;
		RenderImage(Context->Store.GetImage(Assets::ImageAssetRacerIcon)->Texture, NULL, &RenderQuad1);
	}
	else
		RenderImage(Context->Store.GetImage(Assets::ImageAssetRacerIcon)->Texture, NULL, &Screen->RacerIcon.Rect);

	if (Screen->TrainingIcon.IsMouseOver())
	{
		SDL_Rect RenderQuad2 = Screen->TrainingIcon.Rect;
		RenderQuad2.x += 7;
		RenderQuad2.y -= 7;
		RenderImage(Context->Store.GetImage(Assets::ImageAssetTrainingIcon)->Texture, NULL, &RenderQuad2);
	}
	else
		RenderImage(Context->Store.GetImage(Assets::ImageAssetTrainingIcon)->Texture, NULL, &Screen->TrainingIcon.Rect);

	if (Screen->RaceIcon.IsMouseOver())
	{
		SDL_Rect RenderQuad3 = Screen->RaceIcon.Rect;
		RenderQuad3.x += 7;
		RenderQuad3.y -= 7;
		RenderImage(Context->Store.GetImage(Assets::ImageAssetRaceIcon)->Texture, NULL, &RenderQuad3);
	}
	else
		RenderImage(Context->Store.GetImage(Assets::ImageAssetRaceIcon)->Texture, NULL, &Screen->RaceIcon.Rect);

	if (Screen->RankingIcon.IsMouseOver())
	{
		SDL_Rect RenderQuad3 = Screen->RankingIcon.Rect;
		RenderQuad3.x += 7;
		RenderQuad3.y -= 7;
		RenderImage(Context->Store.GetImage(Assets::ImageAssetRankingIcon)->Texture, NULL, &RenderQuad3);
	}
	else
		RenderImage(Context->Store.GetImage(Assets::ImageAssetRankingIcon)->Texture, NULL, &Screen->RankingIcon.Rect);

	//TODO make financials accessible from here
	//RenderText(Context->Store.GetFont(Assets::FontAssetMainFont), "Cash: $" + to_string(Financials.Cash), 450,720, {0,0,0});

	Display();
	auto EndTime = SDL_GetTicks();
	FPS.AddFrame(EndTime - StartTime);
	return EndTime - StartTime;
}

