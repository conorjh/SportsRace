#include "Renderer.h"
#include "Util.h"
#include "SDL2/SDL_image.h"
#include "spdlog/spdlog.h"
#include <iostream>

using namespace std;
using namespace Game;
using namespace Game::App;
using namespace Game::States;

using namespace Game::Race;


Game::App::Renderer::AppRenderer::AppRenderer(AppData* _Data, States::AppStateMachine* _StateMachine) : MainMenuRen(_Data, &MainMenuRenData), InRaceRen(_Data, &InRaceRenData), BaseRenderer(_Data)
{
	spdlog::debug("Starting AppRenderer");
	StateMachine = _StateMachine;

	//Load datas
	spdlog::debug("Loading renderer data...");
	MainMenuRenData.Load();
	MainMenuRen.Load();
	InRaceRenData.Load();
	InRaceRen.Load();

	spdlog::trace("Loading renderer data done");
	spdlog::trace("Starting AppRenderer done");
}

unsigned int Game::App::Renderer::AppRenderer::Render()
{
	if (StateMachine->Top() == nullptr)
		return 0;

	switch (StateMachine->Top()->Type)
	{
	case AppStateType::InRace:
	{
		InRaceState* RaceState = reinterpret_cast<InRaceState*>(StateMachine->Top());
		InRaceRen.State = RaceState;
		return InRaceRen.Render();
	}

	case AppStateType::MainMenu:
	{
		MainMenuState* MenuState = reinterpret_cast<MainMenuState*>(StateMachine->Top());
		MainMenuRen.State = MenuState;
		return MainMenuRen.Render();
	}
	}

	return 0;
}

Game::Render::BaseRenderer::BaseRenderer(AppData* _Data)
{
	Data = _Data;
}

void Game::Render::BaseRenderer::LoadImageFile(std::string File, SDL_Texture*& Tex, SDL_Surface*& Surf)
{
	Surf = IMG_Load(File.c_str());
	Tex = SDL_CreateTextureFromSurface(Data->RenderData.MainRenderer, Surf);
}

void Game::Render::BaseRenderer::LoadFontFile(std::string File, unsigned int Size, TTF_Font* Font)
{
	Font = TTF_OpenFont(File.c_str(), Size);
}

void Game::Render::BaseRenderer::RenderTextSingleLine(TTF_Font* Font, std::string Text, int x, int y, SDL_Color Color)
{
	SDL_Surface* surfaceMessage = TTF_RenderText_Blended(Font, Text.c_str(), Color);
	SDL_Texture* Message = SDL_CreateTextureFromSurface(Data->RenderData.MainRenderer, surfaceMessage);
	SDL_Point size;
	SDL_QueryTexture(Message, NULL, NULL, &size.x, &size.y);;
	SDL_Rect r = { 0, 0, size.x, size.y };
	SDL_Rect Message_rect; //create a rect
	Message_rect.x = x;  //controls the rect's x coordinate 
	Message_rect.y = y; // controls the rect's y coordinte
	Message_rect.w = size.x; // controls the width of the rect
	Message_rect.h = size.y; // controls the height of the rect
	SDL_RenderCopy(Data->RenderData.MainRenderer, Message, &r, &Message_rect);

	// Don't forget to free your surface and texture
	SDL_FreeSurface(surfaceMessage);
	SDL_DestroyTexture(Message);
}

void Game::Render::BaseRenderer::RenderText(TTF_Font* Font, std::string Text, int x, int y, SDL_Color Color)
{
	//split text into lines
	std::string buffer = ""; unsigned int offset = 0;
	for (auto it = Text.begin(); it != Text.end(); ++it)
	{
		if (*it == '\n')
		{
			RenderTextSingleLine(Font, buffer, x, y + offset, Color);
			offset += 12;
			buffer = "";
			continue;
		}

		buffer += *it;
	}

	RenderTextSingleLine(Font, buffer, x, y + offset, Color);
}


void Game::Render::BaseRenderer::Display()
{
	auto FrameTime = SDL_GetTicks();

	FPS.AddFrame(FrameTime - LastFrameEnd);
	SDL_RenderPresent(Data->RenderData.MainRenderer);

	LastFrameEnd = FrameTime;
}


Game::App::Renderer::MainMenuRenderer::MainMenuRenderer(AppData* _Data, MainMenuRendererData* _RenData) : RendererData(_RenData), BaseRenderer(_Data)
{
	State = nullptr;
}

Game::App::Renderer::MainMenuRenderer::MainMenuRenderer(AppData* _Data, MainMenuState* _State, MainMenuRendererData* _RenData) : State(_State), RendererData(_RenData), BaseRenderer(_Data)
{

}

bool Game::App::Renderer::MainMenuRenderer::Load()
{
	LoadImageFile("screen.png", RendererData->ScreenT, RendererData->Screen);
	return true;
}

void Game::App::Renderer::MainMenuRenderer::DrawLeague()
{
	SDL_Color White = { 255, 255, 255 };
	SDL_Rect RenderQuad2 = { 500 , 100 , 378 , 359 };
	RenderImage(RendererData->ScreenT, NULL, &RenderQuad2);

	unsigned int OtherStandings = 4;

	auto ranl = Data->League.Standing.Get("1").Rank;
	int TopEnd = Data->League.Standing.Get("1").Rank - 4;
	if (TopEnd < 0)
		TopEnd = 0;
	int BottomEnd = Data->League.Standing.Get("1").Rank + 4;
	if (BottomEnd >= Data->League.Standing.Entries.size())
		BottomEnd = int(Data->League.Standing.Entries.size() - 1);


	for (int t = 0; t <= BottomEnd - TopEnd; ++t)
	{
		auto Standing = Data->League.Standing.Get(TopEnd + t);
		if (Standing.IsNull())
			continue;

		RenderText(RendererData->InfoFont, to_string(Standing.Rank) + "  " + Standing.Racer->Name + ": " + to_string(Standing.Points) + "pts ", 600, (t * 32) + 133, White);

	}
	RenderText(RendererData->MainFont, State->ExitButton.Text.c_str(), State->ExitButton.x + 5, State->ExitButton.y + 5, State->ExitButton.Color);
}

unsigned int Game::App::Renderer::MainMenuRenderer::Render()
{
	//start timer
	auto StartTime = SDL_GetTicks();

	SDL_SetRenderDrawColor(Data->RenderData.MainRenderer, 0, 0, 0, 0);
	SDL_RenderClear(Data->RenderData.MainRenderer);

	//draw de buttons
	SDL_FillRect(Data->RenderData.MainSurface, &State->RaceButton.Rect, SDL_MapRGB(Data->RenderData.MainSurface->format, 255, 0, 0));

	//race button
	SDL_SetRenderDrawColor(Data->RenderData.MainRenderer, 255, 255, 255, 255);
	SDL_RenderDrawRect(Data->RenderData.MainRenderer, &State->RaceButton.Rect);
	RenderText(RendererData->MainFont, State->RaceButton.Text.c_str(), State->RaceButton.x + 5, State->RaceButton.y + 5, State->RaceButton.Color);

	//exit button
	SDL_SetRenderDrawColor(Data->RenderData.MainRenderer, 255, 255, 255, 255);
	SDL_RenderDrawRect(Data->RenderData.MainRenderer, &State->ExitButton.Rect);
	RenderText(RendererData->MainFont, State->ExitButton.Text.c_str(), State->ExitButton.x + 5, State->ExitButton.y + 5, State->ExitButton.Color);

	//League standings
	DrawLeague();

	//fps
	if (Data->ShowFPS)
		RenderText(RendererData->DebugFont, "FPS: " + to_string(FPS.GetFrameTime()), 975, 5, { 255,255,255 });


	Display();

	auto EndTime = SDL_GetTicks();
	FPS.AddFrame(EndTime - StartTime);

	return EndTime - StartTime;
}

void Game::App::Renderer::MainMenuRendererData::Load()
{
	MainFont = TTF_OpenFont("menu_font.ttf", 96);
	InfoFont = TTF_OpenFont("OpenSans.ttf", 12);
	DebugFont = TTF_OpenFont("OpenSans.ttf", 10);
	
	Screen = IMG_Load("screen.png");
}

void Game::App::Renderer::InRaceRenderer::RenderDebugText()
{
	//top right
	string Body = "AppState: " + AppStateTypeToString(State->Type) + "\n" +
		"RaceState: " + RaceStateTypeToString(State->RaceSM.State->Type) + "\n" +
		"Finished: " + to_string(State->RaceSM.Data.ThisRace.FinishedCount()) + "\n" +
		"Camera.CameraX: " + to_string(Camera.CameraX) + "\n" +
		"Camera.CameraX2: " + to_string(Camera.CameraX2) + "\n";
	RenderText(RendererData->DebugFont, Body, 850, 10, { 255,0,0 });

	//middle top
	RenderText(RendererData->DebugFont, "CurrentTick: " + to_string(State->RaceSM.Data.CurrentTick), 400, 10, { 255,255,255 });
	RenderText(RendererData->DebugFont, "SDL_GetTicks(): " + to_string(SDL_GetTicks()), 400, 22, { 255,255,255 });
	RenderText(RendererData->DebugFont, "Race.CurrentTick: " + to_string(State->RaceSM.Data.ThisRace.CurrentTick), 400, 34, { 255,255,255 });

	//top left
	if (Data->ShowFPS)
	{
		RenderText(RendererData->DebugFont, "Frame time: " + to_string(FPS.GetFrameTime()), 5, 10, { 255,255,255 });
		RenderText(RendererData->DebugFont, "FPS: " + to_string(FPS.GetFPS()), 5, 22, { 255,255,255 });
	}


}

Game::App::Renderer::InRaceRenderer::InRaceRenderer(AppData* _Data, InRaceRendererData* _RendererData) : State(nullptr), RendererData(_RendererData), BaseRenderer(_Data)
{
	Camera.X = 0;
	Camera.W = Data->ScreenWidth;
	Camera.H = Data->ScreenHeight;
}

Game::App::Renderer::InRaceRenderer::InRaceRenderer(AppData* _Data, States::InRaceState* _State, InRaceRendererData* _RendererData) : State(_State), RendererData(_RendererData), BaseRenderer(_Data)
{
	Camera.X = 0;
	Camera.W = Data->ScreenWidth;
	Camera.H = Data->ScreenHeight;
}


void Game::Render::BaseRenderer::RenderImage(SDL_Texture* Texture, SDL_Rect* SourceQuad, SDL_Rect* RenderQuad)
{
	SDL_RenderCopy(Data->RenderData.MainRenderer, Texture, SourceQuad, RenderQuad);
}

void Game::App::Renderer::InRaceRenderer::DrawRacer(Race::Racer Racer, unsigned int Track)
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
		RenderImage(RendererData->FellaWaitT, NULL, &RenderQuad2);	//starters orders
	else
		RenderImage(RendererData->FellaRunT, &SourceQuad, &RenderQuad);	//running

	//RenderText(RendererData->DebugFont, "x: " + to_string(Racer.Pos.X) + "\n" +
	//	"v: " + to_string(Racer.Pos.Velocity) + "\n" +
	//	Racer.Name, XDraw, yOffset, { 255,255,255 });
}

void Game::App::Renderer::InRaceRenderer::DrawWinners()
{
	double ScaleFactor = double(this->Data->ScreenWidth) / double(State->RaceSM.Data.ThisRace.ThisTrack->Length + 200);
	unsigned int yOffset = 50;
	if (State->RaceSM.Data.ThisRace.Result.RacerResults.size() > 0)
	{
		SDL_Rect RenderQuad2 = { 280, 220 , 378 , 359 };
		RenderImage(RendererData->ScreenT, NULL, &RenderQuad2);
		for (int t = 0; t < State->RaceSM.Data.ThisRace.Result.RacerResults.size() && t < 3; ++t)
		{
			auto& Result = State->RaceSM.Data.ThisRace.Result.RacerResults[t];
			auto& Racer = State->RaceSM.Data.ThisRace.Result.RacerResults[t].Racer;

			RenderText(RendererData->MainFont, "#: " + Racer->Name, 470, 290 + (t * 60) + 20, { 255,255,255 });
			RenderText(RendererData->MainFont, to_string(double(double(Result.Ms) / double(1000))) + "sec", 470, 300 + (t * 60) + 32, { 255,255,255 });

			SDL_Rect RenderQuad = { 380, 300 + (t * 60), 50, 50 };
			RenderImage(RendererData->HeadT, NULL, &RenderQuad);
		}
	}
}

void Game::App::Renderer::InRaceRenderer::DrawBackground()
{
	auto ScreenWidth = 1024, ScreenHeight = 768;

	//track
	auto TrackWidth = ScreenWidth, TrackHeight = ScreenHeight;


	int TrackDrawOffset = Camera.CameraX2 % unsigned int(TrackWidth);
	SDL_Rect RenderQuad1 = { 0 - TrackDrawOffset ,0, TrackWidth, TrackHeight };
	SDL_Rect RenderQuad2 = { RenderQuad1.w - TrackDrawOffset, 0, RenderQuad1.w, RenderQuad1.h };
	RenderImage(RendererData->TrackGraphicT, NULL, &RenderQuad1);
	RenderImage(RendererData->TrackGraphicT, NULL, &RenderQuad2);

	auto MountainWidth = TrackWidth * 2, MountainHeight = RendererData->MountainsGraphic->h;
	int xOffset2 = int(Camera.CameraX2 * 0.05) % MountainWidth;
	SDL_Rect RenderQuad3 = { 0 - xOffset2,0, MountainWidth, MountainHeight };
	SDL_Rect RenderQuad4 = { RenderQuad3.w - xOffset2,0, RenderQuad3.w, RenderQuad3.h };
	RenderImage(RendererData->MountainsGraphicT, NULL, &RenderQuad3);
	RenderImage(RendererData->MountainsGraphicT, NULL, &RenderQuad4);

	int xOffset3 = int(Camera.CameraX2 * 0.1) % RendererData->CloudsGraphic->w;
	SDL_Rect RenderQuad5 = { 0 - xOffset3,0, RendererData->CloudsGraphic->w , RendererData->CloudsGraphic->h };
	SDL_Rect RenderQuad6 = { RenderQuad5.w - xOffset3,0, RendererData->CloudsGraphic->w, RendererData->CloudsGraphic->h };
	RenderImage(RendererData->CloudsGraphicT, NULL, &RenderQuad5);
	RenderImage(RendererData->CloudsGraphicT, NULL, &RenderQuad6);

	int xOffset4 = int(Camera.CameraX2 * 0.75) % unsigned int(TrackWidth);
	SDL_Rect RenderQuad7 = { 0 - xOffset4,0, TrackWidth , RendererData->StadiumGraphic->h };
	SDL_Rect RenderQuad8 = { RenderQuad7.w - xOffset4,0, TrackWidth, RendererData->StadiumGraphic->h };
	RenderImage(RendererData->StadiumGraphicT, NULL, &RenderQuad7);
	RenderImage(RendererData->StadiumGraphicT, NULL, &RenderQuad8);

	int BlockX = 10 - Camera.CameraX;
	SDL_Rect RenderQuad9 = { BlockX , 490, 243 * 0.75, 324 * 0.75 };
	RenderImage(RendererData->StartingBlocksGraphicT, NULL, &RenderQuad9);
}

unsigned int Game::App::Renderer::InRaceRenderer::Render()
{
	//start timer
	auto StartTime = SDL_GetTicks();

	Camera.TrackLength = State->RaceSM.Data.ThisRace.ThisTrack->Length;

	if (State->RaceSM.Data.ThisRace.Contains("1"))
		Camera.PointAt(State->RaceSM.Data.ThisRace.Get("1")->Pos.X + 75);
	else
		Camera.PointAt(State->RaceSM.Data.ThisRace.CurrentWinnerDistance() + 75);

	SDL_SetRenderDrawColor(Data->RenderData.MainRenderer, 0, 0, 0, 0);
	SDL_RenderClear(Data->RenderData.MainRenderer);

	DrawBackground();

	//draw racers
	auto& Racers = State->RaceSM.Data.ThisRace.Racers;
	for (int t = 0; t < Racers.size(); ++t)
		DrawRacer(*Racers[t], t);

	DrawWinners();

	RenderDebugText();

	Display();
	auto EndTime = SDL_GetTicks();
	FPS.AddFrame(EndTime - StartTime);
	return EndTime - StartTime;
}

bool Game::App::Renderer::InRaceRenderer::Load()
{
	LoadFontFile("menu_font.ttf", 16,  RendererData->WinningFont);

	RendererData->MainFont = TTF_OpenFont("PIXELLARI.ttf", 16);
	RendererData->InfoFont = TTF_OpenFont("OpenSans.ttf", 14);
	RendererData->DebugFont = TTF_OpenFont("OpenSans.ttf", 12);
	RendererData->RacerGraphic = IMG_Load("fella.png");

	LoadImageFile("reido.png", RendererData->HeadT, RendererData->Head);
	LoadImageFile("track.png", RendererData->TrackGraphicT, RendererData->TrackGraphic);
	LoadImageFile("stadium.png", RendererData->StadiumGraphicT, RendererData->StadiumGraphic);
	LoadImageFile("startingblock.png", RendererData->StartingBlocksGraphicT, RendererData->StartingBlocksGraphic);
	LoadImageFile("clouds.png", RendererData->CloudsGraphicT, RendererData->CloudsGraphic);
	LoadImageFile("mountains.png", RendererData->MountainsGraphicT, RendererData->MountainsGraphic);
	LoadImageFile("fellarun.png", RendererData->FellaRunT, RendererData->FellaRun);
	LoadImageFile("fellawait.png", RendererData->FellaWaitT, RendererData->FellaWait);
	LoadImageFile("screen.png", RendererData->ScreenT, RendererData->Screen);

	return true;
}

void Game::App::Renderer::InRaceRendererData::Load()
{


}

void Game::Render::FPSCounter::AddFrame(unsigned int TimeTakenMs)
{
	if (TimeTakenMs > 1000)
		TimeTakenMs = 1000;

	//max 3k samples
	while (TotalFrameTime + TimeTakenMs > 1000)
	{
		TotalFrameTime -= *FrameTimes.begin();
		FrameTimes.erase(FrameTimes.begin());
	}

	FrameTimes.push_back(TimeTakenMs);
	TotalFrameTime += TimeTakenMs;
}

void Game::Render::FPSCounter::Clear()
{
	FrameTimes.clear();
	TotalFrameTime = 0;
}

unsigned int Game::Render::FPSCounter::GetFrameTime()
{
	if (FrameTimes.size() == 0)
		return 0;

	return FrameTimes.back();
}

unsigned int Game::Render::FPSCounter::GetFPS()
{
	if (!FrameTimes.size() || !TotalFrameTime)
		return 0;



	return 1000 / GetFrameTime();
}

void Game::App::Renderer::InRaceRendererCamera::PointAt(unsigned int X)
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
