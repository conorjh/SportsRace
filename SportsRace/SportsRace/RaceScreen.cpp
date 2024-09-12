#include "RaceScreen.h"
#include "Race.h"

using namespace Game::App;
using namespace Game::Audio;
using namespace Game::Race;
using namespace Game::Render;
using namespace Game::Renderer;
using namespace Game::Screens;
using namespace std;

bool Game::Renderer::RaceScreenRendererData::Load(Render::BaseRenderer& Renderer)
{
	if (!Renderer.LoadFontFile("menu_font.ttf", 16, WinningFont) ||
		!Renderer.LoadImageFile("reido.png", Head) ||
		!Renderer.LoadImageFile("track.png", TrackGraphic) ||
		!Renderer.LoadImageFile("stadium.png", StadiumGraphic) ||
		!Renderer.LoadImageFile("startingblock.png", StartingBlocksGraphic) ||
		!Renderer.LoadImageFile("clouds.png", CloudsGraphic) ||
		!Renderer.LoadImageFile("mountains.png", MountainsGraphic) ||
		!Renderer.LoadImageFile("fellarun.png", FellaRun) ||
		!Renderer.LoadImageFile("fellawait.png", FellaWait) ||
		!Renderer.LoadImageFile("screen.png", Background))
		return false;
	return true;
}


Game::Screens::RaceScreen::RaceScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* TrainingRacer) : 
	AppScreen(_Machine, _IO, _Data), RaceSM(*new Race::Race())
{
	Type = AppScreenType::RaceScreen;
	LastFrameEnd = SDL_GetTicks();
}

Game::Screens::RaceScreen::RaceScreen(AppScreenStateMachine& _Machine, AppIO& _IO, AppData& _Data, Race::Race RaceToRun) : 
	AppScreen(_Machine, _IO, _Data), RaceSM(RaceToRun)
{
	Type = AppScreenType::RaceScreen;
	LastFrameEnd = SDL_GetTicks();
}

Game::Screens::RaceScreen::RaceScreen(AppScreenStateMachine& _Machine, AppIO& _IO, AppData& _Data, Race::Race RaceToRun, RacerGUID PlayerRacer) : 
	AppScreen(_Machine, _IO, _Data), RaceSM(RaceToRun)
{
	Type = AppScreenType::RaceScreen;
	LastFrameEnd = SDL_GetTicks();
	PlayerGUID = PlayerRacer;
}

Game::Screens::RaceScreen::~RaceScreen()
{

}

void Game::Screens::RaceScreen::Entry()
{
	IO.Player.StopMusic();
}

void Game::Screens::RaceScreen::Exit()
{
	IO.Player.StopMusic();

}

AppScreen* Game::Screens::RaceScreen::Update()
{
	while (SDL_GetTicks() > LastFrameEnd + 33)
	{
		auto OldType = RaceSM.State->Type;
		auto NewType = RaceSM.Tick(33);
		LastFrameEnd += 33;

		if (NewType != OldType && NewType == RaceStateType::Racing)
		{
			IO.Player.Play(BuiltInSounds::StartersPistol);
			IO.Player.Play(Soundtrack::Race);
		}

	}

	if (IO.Esc)
	{
		IO.Player.Play(BuiltInSounds::Click);

		//save this races result to the AppData buffer
		if (&Data.Profile->MainFella != nullptr && RaceSM.Data.ThisRace.HasFinished(&Data.Profile->MainFella))
		{
			if (!RaceSM.Data.ThisRace.HasFinished())
				RaceSM.Data.ThisRace.Sim();

			this->Data.Profile->Financials.Cash -= RaceSM.Data.ThisRace.Financials.EntranceFee;

			if (RaceSM.Data.ThisRace.Result.RacerResults[0].Racer->GUID.GUID == PlayerGUID.GUID)
				this->Data.Profile->Financials.Cash += RaceSM.Data.ThisRace.Financials.FirstPlacePrize;
			if (RaceSM.Data.ThisRace.Result.RacerResults[1].Racer->GUID.GUID == PlayerGUID.GUID)
				this->Data.Profile->Financials.Cash += RaceSM.Data.ThisRace.Financials.SecondPlacePrize;
			if (RaceSM.Data.ThisRace.Result.RacerResults[2].Racer->GUID.GUID == PlayerGUID.GUID)
				this->Data.Profile->Financials.Cash += RaceSM.Data.ThisRace.Financials.ThirdPlacePrize;

			Data.RaceStateOutput = RaceSM.Data.ThisRace.Result;
		}

		//go back to main menu
		ScreenStack.Pop();
		return ScreenStack.Top();
	}

	return this;
}


Game::Renderer::RaceScreenRenderer::RaceScreenRenderer(AppData* _Data, RaceScreenRendererData* _RendererData, BaseRendererData* _BaseData) :
	State(nullptr), RendererData(_RendererData), BaseRenderer(_Data, _BaseData)
{
	Camera.X = 0;
	Camera.W = Data->ScreenWidth;
	Camera.H = Data->ScreenHeight;
}

Game::Renderer::RaceScreenRenderer::RaceScreenRenderer(AppData* _Data, Screens::RaceScreen* _State, RaceScreenRendererData* _RendererData, BaseRendererData* _BaseData) :
	State(_State), RendererData(_RendererData), BaseRenderer(_Data, _BaseData)
{
	Camera.X = 0;
	Camera.W = Data->ScreenWidth;
	Camera.H = Data->ScreenHeight;
}

void Game::Renderer::RaceScreenRendererCamera::PointAt(unsigned int X)
{
	CameraX = X - (W / 2);
	CameraX2 = X + (W / 2);

	if (CameraX2 > TrackLength)
	{
		CameraX = TrackLength - W;
		CameraX2 = TrackLength;
	}
	if (CameraX < 0)
	{
		CameraX = 0;
		CameraX2 = CameraX + W;
	}
}


void Game::Renderer::RaceScreenRenderer::DrawRacer(Race::Racer Racer, unsigned int Track)
{
	unsigned int yOffset = ((Track + 1) * 40) + 300;
	unsigned int StaggerOffset = (Track + 1) * 20;
	unsigned int xOffset = 200;
	if (Camera.CameraX > 0)
		xOffset = 150 - Camera.CameraX > 0 ? 150 - Camera.CameraX : 0;
	unsigned int XDraw = xOffset + (Racer.Pos.X - Camera.CameraX) - (StaggerOffset);

	unsigned int RacerWidth = 153;
	//if (XDraw + RacerWidth + 200 < 0 || XDraw > Camera.W)
		//return;

	unsigned int FrameNum = (613 / 4) * (Racer.RunFrame);
	SDL_Rect SourceQuad = { 0 + FrameNum, 0, 613 / 4, 186 / 1 };
	SDL_Rect RenderQuad = { XDraw   ,  yOffset, 613 / 4,186 / 1 };


	SDL_Rect RenderQuad2 = { XDraw  ,  yOffset + 79, 144,107 };
	if (State->RaceSM.State->Type == RaceStateType::StartersOrders)
		RenderImage(RendererData->FellaWait.Texture, NULL, &RenderQuad2);	//starters orders
	else
		RenderImage(RendererData->FellaRun.Texture, &SourceQuad, &RenderQuad);	//running

	RenderText(BaseData->DebugFont, Racer.Name, XDraw, yOffset + 50, { 255,255,255 });


}

void Game::Renderer::RaceScreenRenderer::DrawWinners()
{
	double ScaleFactor = double(this->Data->ScreenWidth) / double(unsigned int(State->RaceSM.Data.ThisRace.ThisTrack->Length) + 200);
	unsigned int yOffset = 50;
	if (State->RaceSM.Data.ThisRace.Result.RacerResults.size() > 0)
	{
		SDL_Rect RenderQuad2 = { 280, 220 , 378 , 359 };
		RenderImage(RendererData->Background.Texture, NULL, &RenderQuad2);
		for (int t = 0; t < State->RaceSM.Data.ThisRace.Result.RacerResults.size() && t < 3; ++t)
		{
			auto& Result = State->RaceSM.Data.ThisRace.Result.RacerResults[t];
			auto& Racer = State->RaceSM.Data.ThisRace.Result.RacerResults[t].Racer;

			auto WinAmount = t == 0 ? State->RaceSM.Data.ThisRace.Financials.FirstPlacePrize :
				t == 1 ? State->RaceSM.Data.ThisRace.Financials.SecondPlacePrize : State->RaceSM.Data.ThisRace.Financials.ThirdPlacePrize;

			RenderText(BaseData->MainFont, "Wins " + to_string(WinAmount), 290, 290 + (t * 60) + 20, { 255,255,255 });
			RenderText(BaseData->MainFont, "#: " + Racer->Name, 470, 290 + (t * 60) + 20, { 255,255,255 });
			RenderText(BaseData->MainFont, to_string(double(double(Result.Ms) / double(1000))) + "sec", 470, 300 + (t * 60) + 32, { 255,255,255 });

			SDL_Rect RenderQuad = { 380, 300 + (t * 60), 50, 50 };
			RenderImage(RendererData->Head.Texture, NULL, &RenderQuad);
		}
	}
}

void Game::Renderer::RaceScreenRenderer::DrawBackground()
{
	auto ScreenWidth = 1024, ScreenHeight = 768;

	//track
	auto TrackWidth = ScreenWidth, TrackHeight = ScreenHeight;

	int TrackDrawOffset = Camera.CameraX2 % unsigned int(TrackWidth);
	SDL_Rect RenderQuad1 = { 0 - TrackDrawOffset ,0, TrackWidth, TrackHeight };
	SDL_Rect RenderQuad2 = { RenderQuad1.w - TrackDrawOffset, 0, RenderQuad1.w, RenderQuad1.h };
	RenderImage(RendererData->TrackGraphic.Texture, NULL, &RenderQuad1);
	RenderImage(RendererData->TrackGraphic.Texture, NULL, &RenderQuad2);

	auto MountainWidth = TrackWidth * 2, MountainHeight = RendererData->MountainsGraphic.Surface->h;
	int xOffset2 = int(Camera.CameraX2 * 0.05) % MountainWidth;
	SDL_Rect RenderQuad3 = { 0 - xOffset2,0, MountainWidth, MountainHeight };
	SDL_Rect RenderQuad4 = { RenderQuad3.w - xOffset2,0, RenderQuad3.w, RenderQuad3.h };
	RenderImage(RendererData->MountainsGraphic.Texture, NULL, &RenderQuad3);
	RenderImage(RendererData->MountainsGraphic.Texture, NULL, &RenderQuad4);

	int xOffset3 = int(Camera.CameraX2 * 0.1) % RendererData->CloudsGraphic.Surface->w;
	SDL_Rect RenderQuad5 = { 0 - xOffset3,0, RendererData->CloudsGraphic.Surface->w , RendererData->CloudsGraphic.Surface->h };
	SDL_Rect RenderQuad6 = { RenderQuad5.w - xOffset3,0, RendererData->CloudsGraphic.Surface->w, RendererData->CloudsGraphic.Surface->h };
	RenderImage(RendererData->CloudsGraphic.Texture, NULL, &RenderQuad5);
	RenderImage(RendererData->CloudsGraphic.Texture, NULL, &RenderQuad6);

	int xOffset4 = int(Camera.CameraX2 * 0.75) % unsigned int(TrackWidth);
	SDL_Rect RenderQuad7 = { 0 - xOffset4,0, TrackWidth , RendererData->StadiumGraphic.Surface->h };
	SDL_Rect RenderQuad8 = { RenderQuad7.w - xOffset4,0, TrackWidth, RendererData->StadiumGraphic.Surface->h };
	RenderImage(RendererData->StadiumGraphic.Texture, NULL, &RenderQuad7);
	RenderImage(RendererData->StadiumGraphic.Texture, NULL, &RenderQuad8);

	int BlockX = 10 - Camera.CameraX;
	SDL_Rect RenderQuad9 = { BlockX , 490, 243 * 0.75, 324 * 0.75 };
	RenderImage(RendererData->StartingBlocksGraphic.Texture, NULL, &RenderQuad9);
}

void Game::Renderer::RaceScreenRenderer::DrawProgressBar()
{
	//Render red filled quad
	int W = 200, H = 7;
	int X = (1024 / 2) - (W / 2), Y = 740;
	SDL_Rect fillRect = { X ,Y , W,H };
	SDL_SetRenderDrawColor(Data->RenderData.MainRenderer, 0, 0, 0, 255);
	SDL_FRect fillRectF;
	SDL_RectToFRect(&fillRect, &fillRectF);
	SDL_RenderFillRect(Data->RenderData.MainRenderer, &fillRectF);

	auto& Race = State->RaceSM.Data.ThisRace;
	for (int t = Race.Racers.size() - 1; t > -1 ; --t)
	{
		//Draw racers circle
		auto* R = Race.GetByRank(t + 1);
		double ScaleFactor =  double(R->Pos.X) / unsigned int(Race.ThisTrack->Length);
		
		SDL_Rect RenderQuad = { X + (W * ScaleFactor) - 10, Y - 15, 32,32 };
		if (R->GUID == State->PlayerGUID)
		{
			RenderQuad.x -= 8;
			RenderQuad.y -= 8;
			RenderQuad.w = RenderQuad.h = 48;
		}
		RenderImage(this->RendererData->Head.Texture,NULL, &RenderQuad);
	}
}

unsigned int Game::Renderer::RaceScreenRenderer::Render()
{
	//start timer
	auto StartTime = SDL_GetTicks();

	Camera.TrackLength = unsigned int(State->RaceSM.Data.ThisRace.ThisTrack->Length);

	if (this->State->RaceSM.Data.ThisRace.Contains(State->PlayerGUID))
		Camera.PointAt(State->RaceSM.Data.ThisRace.Get(State->PlayerGUID)->Pos.X + 75);
	else
		Camera.PointAt(State->RaceSM.Data.ThisRace.CurrentWinnerDistance() + 75);

	SDL_SetRenderDrawColor(Data->RenderData.MainRenderer, 0, 0, 0, 0);
	SDL_RenderClear(Data->RenderData.MainRenderer);

	DrawBackground();

	//draw racers
	auto& Racers = State->RaceSM.Data.ThisRace.Racers;
	for (int t = 0; t < Racers.size(); ++t)
		DrawRacer(*Racers[t], t);

	DrawProgressBar();

	DrawWinners();


	//RenderDebugText();

	Display();
	auto EndTime = SDL_GetTicks();
	FPS.AddFrame(EndTime - StartTime);
	return EndTime - StartTime;
}

void Game::Renderer::RaceScreenRenderer::RenderDebugText()
{
	//top right
	string Body = "AppScreen: " + AppStateTypeToString(State->Type) + "\n" +
		"RaceState: " + RaceStateTypeToString(State->RaceSM.State->Type) + "\n" +
		"Finished: " + to_string(State->RaceSM.Data.ThisRace.FinishedCount()) + "\n" +
		"Leader: " + State->RaceSM.Data.ThisRace.CurrentWinner()->Name + "\n" +
		"Entrance Fee: " + to_string(State->RaceSM.Data.ThisRace.Financials.EntranceFee) + "\n" +
		"1st Prize: " + to_string(State->RaceSM.Data.ThisRace.Financials.FirstPlacePrize) + "\n" +
		"Camera.CameraX: " + to_string(Camera.CameraX) + "\n" +
		"Camera.CameraX2: " + to_string(Camera.CameraX2) + "\n";
	RenderText(BaseData->DebugFont, Body, 850, 10, { 255,0,0 });

	//middle top
	RenderText(BaseData->DebugFont, "CurrentTick: " + to_string(State->RaceSM.Data.CurrentTick), 400, 10, { 255,255,255 });
	RenderText(BaseData->DebugFont, "SDL_GetTicks(): " + to_string(SDL_GetTicks()), 400, 22, { 255,255,255 });
	RenderText(BaseData->DebugFont, "Race.CurrentTick: " + to_string(State->RaceSM.Data.ThisRace.CurrentTick), 400, 34, { 255,255,255 });

	//top left
	if (Data->ShowFPS)
	{
		RenderText(BaseData->DebugFont, "Frame time: " + to_string(FPS.GetFrameTime()), 5, 10, { 255,255,255 });
		RenderText(BaseData->DebugFont, "FPS: " + to_string(FPS.GetFPS()), 5, 22, { 255,255,255 });
	}


}
