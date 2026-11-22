#include "RaceScreen.h"
#include "..\Race.h"
#include "..\App\App.h"
#include "spdlog/spdlog.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

namespace
{
	constexpr unsigned int RacerDetailsVerticalOffset = 40;
	constexpr int RacerNameVerticalOffset = 50;
	constexpr int RacerTextLineSpacing = 20;
	constexpr unsigned int RaceScreenCameraLookAhead = 75;
	constexpr int RaceResultsFirstRowY = 310;
	constexpr int RaceResultsPodiumRowSpacing = 60;
	constexpr int RaceResultsPulledUpFirstRowY = 480;
	constexpr int RaceResultsPulledUpRowSpacing = 20;
	constexpr int RacerSpriteFrameWidth = 153;
	constexpr int RacerSpriteFrameHeight = 186;
	constexpr SDL_Color RacerLabelColor{ 255, 255, 255, 255 };

	std::string RaceEffectsToString(unsigned int Effects)
	{
		std::string Text;
		auto AppendEffect = [&Text](const char* Effect)
		{
			if (!Text.empty())
				Text += ", ";
			Text += Effect;
		};

		if ((Effects & Game::Race::RaceEffectStumble) != 0)
			AppendEffect("Stumble");
		if ((Effects & Game::Race::RaceEffectGassing) != 0)
			AppendEffect("Gassing");
		if ((Effects & Game::Race::RaceEffectLostBelief) != 0)
			AppendEffect("Lost belief");
		if ((Effects & Game::Race::RaceEffectBelievesInSelf) != 0)
			AppendEffect("Believes in self");
		if ((Effects & Game::Race::RaceEffectSecondWind) != 0)
		{
			AppendEffect("Second wind");
		}
		if ((Effects & Game::Race::RaceEffectPulledUp) != 0)
			AppendEffect("Pulled-up");

		return Text;
	}

	void SavePlayerRaceResult(Game::App::AppData& Data, Game::Race::Race& CurrentRace,
		Game::Race::RacerGUID PlayerGUID, bool& ResultSaved)
	{
		if (ResultSaved)
			return;

		Game::Race::Racer* Player = CurrentRace.Get(PlayerGUID);
		if (Player == nullptr)
			return;

		const Game::Race::RaceResult RaceResult = CurrentRace.Sim();
		if (RaceResult.Error != Game::Race::RaceError::None)
		{
			spdlog::error("Could not save career race results: {}",
				Game::Race::RaceErrorToString(RaceResult.Error));
			if (Data.Career != nullptr && Data.Profile != nullptr)
			{
				Game::Career::CareerOrchestrator Orchestrator(Data.Career, Data.Profile);
				Orchestrator.CompletePlayerLeagueRace(RaceResult);
			}
			return;
		}
		if (!CurrentRace.IsComplete())
			return;

		bool PlayerResultFound = false;
		for (const auto& Result : CurrentRace.Result.RacerResults)
		{
			if (Result.Racer == nullptr)
				continue;

			const bool Finished = Result.Position != Game::Race::PulledUpPosition;
			Game::Race::RacerCareerHistoryEntry HistoryEntry;
			HistoryEntry.WorldTimeMs = Data.Career == nullptr ? 0 : Data.Career->WorldClock.NowMs();
			HistoryEntry.EventName = Data.Career != nullptr &&
				Data.Career->PendingPlayerLeagueRace.Active
				? Data.Career->PendingPlayerLeagueRace.EventName : "Career Race";
			HistoryEntry.TrackLength = CurrentRace.Result.TrackLength;
			HistoryEntry.Position = Result.Position;
			HistoryEntry.FinishTimeMs = Finished ? Result.Ms : 0;
			Result.Racer->RecordCareerResult(Result.Position, HistoryEntry.FinishTimeMs, HistoryEntry);
			if (Result.Racer != Player)
				continue;

			PlayerResultFound = true;
			if (Data.Profile != nullptr)
			{
				// Settle entry fees and placing prizes only after the final result is known.
				Data.Profile->Financials.Cash -= CurrentRace.Financials.EntranceFee;
				if (Result.Position == Game::Race::FirstPlaceRank)
					Data.Profile->Financials.Cash += CurrentRace.Financials.FirstPlacePrize;
				else if (Result.Position == Game::Race::SecondPlaceRank)
					Data.Profile->Financials.Cash += CurrentRace.Financials.SecondPlacePrize;
				else if (Result.Position == Game::Race::ThirdPlaceRank)
					Data.Profile->Financials.Cash += CurrentRace.Financials.ThirdPlacePrize;
			}
		}
		if (!PlayerResultFound)
		{
			spdlog::error("Could not save career race results: the player's result is missing.");
			if (Data.Career != nullptr && Data.Profile != nullptr)
			{
				Game::Career::CareerOrchestrator Orchestrator(Data.Career, Data.Profile);
				Orchestrator.CompletePlayerLeagueRace(CurrentRace.Result);
			}
			return;
		}

		if (Data.Career != nullptr && Data.Profile != nullptr)
		{
			Game::Career::CareerOrchestrator Orchestrator(Data.Career, Data.Profile);
			Orchestrator.CompleteUpcomingRace(CurrentRace.Result);
		}
		Data.RaceStateOutput = CurrentRace.Result;
		ResultSaved = true;
	}
}

Game::Screens::RaceScreen::RaceScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* TrainingRacer) 
	: AppScreen(_Machine, _IO, _Data), 
	RaceSM(std::make_shared<Race::RaceStateMachine>(Race::Race()))
{
	Type = AppScreenType::RaceScreen;
	LastFrameEnd = SDL_GetTicks();
}

Game::Screens::RaceScreen::RaceScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Race RaceToRun) 
	:AppScreen(_Machine, _IO, _Data), 
	RaceSM(std::make_shared<Race::RaceStateMachine>(std::move(RaceToRun)))
{
	Type = AppScreenType::RaceScreen;
	ShowPostRaceOnComplete = true;
	RaceEventName = "Exhibition race";
	CapturePreRaceProgress();
	LastFrameEnd = SDL_GetTicks();
}

Game::Screens::RaceScreen::RaceScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Race RaceToRun, Race::RacerGUID PlayerRacer) 
	: AppScreen(_Machine, _IO, _Data), 
	RaceSM(std::make_shared<Race::RaceStateMachine>(std::move(RaceToRun)))
{
	Type = AppScreenType::RaceScreen;
	LastFrameEnd = SDL_GetTicks();
	PlayerGUID = PlayerRacer;
	PlayerRacerSupplied = true;
	ShowPostRaceOnComplete = true;
	RaceEventName = Data.Career != nullptr && Data.Career->PendingPlayerLeagueRace.Active &&
		!Data.Career->PendingPlayerLeagueRace.EventName.empty()
		? Data.Career->PendingPlayerLeagueRace.EventName : "Career race";
	CapturePreRaceProgress();
}

Game::Screens::RaceScreen::RaceScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO,
	App::AppData& _Data, const std::shared_ptr<Career::CareerWorldRace>& WorldRace)
	: AppScreen(_Machine, _IO, _Data),
	RaceSM(WorldRace->StateMachine)
{
	Type = AppScreenType::RaceScreen;
	LastFrameEnd = SDL_GetTicks();
	ShowPostRaceOnComplete = true;
	IsWorldRaceViewer = true;
	RaceEventName = WorldRace->EventName.empty() ? "Career world race" : WorldRace->EventName;
	CapturePreRaceProgress();
}

Game::Screens::RaceScreen::~RaceScreen()
{

}

void Game::Screens::RaceScreen::CapturePreRaceProgress()
{
	PreRaceProgress.clear();
	for (Race::Racer* Runner : RaceSM->Data.ThisRace.GetEntrants())
	{
		if (Runner == nullptr)
			continue;
		RacerProgressSnapshot Snapshot;
		Snapshot.Runner = Runner;
		Snapshot.Xp = Runner->Skills.Xp;
		Snapshot.Wins = Runner->CareerRecord.Wins;
		Snapshot.Losses = Runner->CareerRecord.Losses;
		Snapshot.RacesEntered = Runner->CareerRecord.RacesEntered;
		Snapshot.PositionPoints = Runner->CareerRecord.PositionPoints;
		Snapshot.BestTimeMs = Runner->CareerRecord.BestTimeMs;
		Snapshot.Reflex = Runner->Skills.Reflex;
		Snapshot.Composure = Runner->Skills.Composure;
		Snapshot.Acceleration = Runner->Skills.Acceleration;
		Snapshot.Endurance = Runner->Skills.Endurance;
		Snapshot.Speed = Runner->Skills.Speed;
		PreRaceProgress.push_back(Snapshot);
	}
}

void Game::Screens::RaceScreen::Entry()
{
	IO.Player.StopMusic();
}

void Game::Screens::RaceScreen::Exit()
{
	IO.Player.StopMusic();
}

Game::Screens::AppScreen* Game::Screens::RaceScreen::Update()
{
	using namespace Game::Race;
	using namespace Game::Audio;

	// Consume fixed-size race ticks to keep simulation cadence independent of render frame rate.
	while (!IsWorldRaceViewer && SDL_GetTicks() > LastFrameEnd + RaceTickIntervalMs)
	{
		auto OldType = RaceSM->State->Type;
		auto NewType = RaceSM->Tick(RaceTickIntervalMs);
		LastFrameEnd += RaceTickIntervalMs;

		if (NewType != OldType && NewType == RaceStateType::Racing)
		{
			IO.Player.Play(BuiltInSounds::StartersPistol);
			IO.Player.Play(Soundtrack::Race);
		}

	}
	if (ShowPostRaceOnComplete && !PostRaceShown && RaceSM->State->Type == RaceStateType::Finished)
	{
		if (PlayerRacerSupplied)
		{
			SavePlayerRaceResult(Data, RaceSM->Data.ThisRace, PlayerGUID, PlayerRaceResultSaved);
		}
		PostRaceShown = true;
		ScreenStack.Push(new PostRaceScreen(ScreenStack, IO, Data, RaceSM->Data.ThisRace,
			std::move(PreRaceProgress), PlayerGUID, PlayerRacerSupplied, RaceEventName));
		return ScreenStack.Top();
	}

	if (IO.Esc)
	{
		IO.Player.Play(BuiltInSounds::Click);

		if (PlayerRacerSupplied)
			SavePlayerRaceResult(Data, RaceSM->Data.ThisRace, PlayerGUID, PlayerRaceResultSaved);

		//go back to main menu
		ScreenStack.Pop();
		return ScreenStack.Top();
	}

	return this;
}

Game::Render::RaceScreenRenderer::RaceScreenRenderer(Game::Render::AppRenderContext* Context) 
	: Screen(nullptr), 
	BaseRenderer(Context)
{
	Camera.X = 0;
	Camera.W = Context->ScreenWidth;
	Camera.H = Context->ScreenHeight;
}

Game::Render::RaceScreenRenderer::RaceScreenRenderer(Game::Render::AppRenderContext* Context, Screens::RaceScreen* _State) 
	: Screen(_State), 
	BaseRenderer(Context)
{
	Camera.X = 0;
	Camera.W = Context->ScreenWidth;
	Camera.H = Context->ScreenHeight;
}

void Game::Render::RaceScreenRendererCamera::PointAt(unsigned int X)
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

void Game::Render::RaceScreenRenderer::DrawRacer(const Race::Racer& Racer, unsigned int Track)
{
	using namespace Game::Race;

	unsigned int yOffset = ((Track + 1) * 40) + 300;
	unsigned int StaggerOffset = (Track + 1) * 20;
	int xOffset = 200;
	if (Camera.CameraX > 0)
		xOffset = 150 - Camera.CameraX > 0 ? 150 - Camera.CameraX : 0;
	const int XDraw = xOffset + Racer.Pos.X -
		Camera.CameraX - static_cast<int>(StaggerOffset);
	const unsigned int ActiveEffects = Screen->RaceSM->Data.ThisRace.GetActiveEffects(&Racer);
	const bool PulledUp = (ActiveEffects & RaceEffectPulledUp) != 0;
	const bool Stumbling = (ActiveEffects & RaceEffectStumble) != 0;
	const auto RunningImageId = Stumbling ? Assets::ImageAssetFellaStumble : Assets::ImageAssetFellaRun;
	const int FrameNum = RacerSpriteFrameWidth * static_cast<int>(Racer.RunFrame);
	SDL_Rect SourceQuad = { FrameNum, 0, RacerSpriteFrameWidth, RacerSpriteFrameHeight };
	SDL_Rect RenderQuad = { XDraw, static_cast<int>(yOffset), RacerSpriteFrameWidth, RacerSpriteFrameHeight };

	SDL_Rect RenderQuad2 = { XDraw  ,  yOffset + 79, 144,107 };
	if (Screen->RaceSM->State->Type == RaceStateType::StartersOrders || PulledUp)
		RenderImage(Context->Store.GetImage(Assets::ImageAssetFellaWait)->Texture, NULL, &RenderQuad2);	//starters orders
	else
	{
		auto* RunningImage = Context->Store.GetImage(RunningImageId);
		if (RunningImage != nullptr)
			RenderImage(RunningImage->Texture, &SourceQuad, &RenderQuad);
	}

	const int RacerNameY = static_cast<int>(yOffset) + RacerNameVerticalOffset;
	const int RacerPaceY = RacerNameY - RacerTextLineSpacing;
	const std::string RacerPace = "Pace: " + std::to_string(static_cast<unsigned int>(std::lround(Racer.Pos.Velocity)));
	const std::string Effects = RaceEffectsToString(ActiveEffects);
	RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), RacerPace, XDraw, RacerPaceY, RacerLabelColor);
	RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), Racer.Name, XDraw, RacerNameY, RacerLabelColor);
	if (!Effects.empty())
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), "Effects: " + Effects, XDraw,
			static_cast<int>(yOffset) - static_cast<int>(RacerDetailsVerticalOffset), RacerLabelColor);

}

void Game::Render::RaceScreenRenderer::DrawWinners()
{
	using namespace std;
	auto& Race = Screen->RaceSM->Data.ThisRace;
	const auto& Results = Race.Result.RacerResults;

	// Keep standings hidden until the career racer's outcome is known; full completion also resolves a pull-up.
	if (Screen->HasPlayerRacer() && !Race.IsComplete() &&
		!Race.HasFinished(Race.Get(Screen->PlayerGUID)))
		return;

	if (Results.empty() || (!Race.IsComplete() && Race.FinishedCount() == 0))
		return;

	SDL_Rect RenderQuad2 = { 280, 220 , 378 , 359 };
	RenderImage(Context->Store.GetImage(Assets::ImageAssetRaceBackground)->Texture, NULL, &RenderQuad2);
	unsigned int PodiumRowCount = 0;
	for (unsigned int Rank = Game::Race::FirstPlaceRank; Rank <= Game::Race::ThirdPlaceRank; ++Rank)
	{
		const Game::Race::RacerRaceResult* RankedResult = nullptr;
		for (const auto& Result : Results)
		{
			if (Result.Position == Rank)
			{
				RankedResult = &Result;
				break;
			}
		}
		if (RankedResult == nullptr)
			continue;

		const int RowOffset = static_cast<int>(Rank - Game::Race::FirstPlaceRank) * RaceResultsPodiumRowSpacing;
		const unsigned int WinAmount = Rank == Game::Race::FirstPlaceRank
			? Race.Financials.FirstPlacePrize
			: Rank == Game::Race::SecondPlaceRank
				? Race.Financials.SecondPlacePrize
				: Race.Financials.ThirdPlacePrize;
		RenderText(Context->Store.GetFont(Assets::FontAssetMainFont), "Wins " + to_string(WinAmount),
			290, RaceResultsFirstRowY + RowOffset, { 255,255,255 });
		RenderText(Context->Store.GetFont(Assets::FontAssetMainFont), "#" + to_string(Rank) + ": " + RankedResult->Racer->Name,
			470, RaceResultsFirstRowY + RowOffset, { 255,255,255 });
		RenderText(Context->Store.GetFont(Assets::FontAssetMainFont),
			to_string(static_cast<double>(RankedResult->Ms) / 1000.0) + "sec",
			470, RaceResultsFirstRowY + RowOffset + 22, { 255,255,255 });

		SDL_Rect RenderQuad = { 380, RaceResultsFirstRowY - 10 + RowOffset, 50, 50 };
		RenderImage(Context->Store.GetImage(Assets::ImageAssetHead)->Texture, NULL, &RenderQuad);
		++PodiumRowCount;
	}

	int PulledUpY = RaceResultsFirstRowY + static_cast<int>(PodiumRowCount) * RaceResultsPodiumRowSpacing;
	if (PodiumRowCount >= Game::Race::ThirdPlaceRank)
		PulledUpY = RaceResultsPulledUpFirstRowY;
	unsigned int PulledUpRow = 0;
	for (const auto& Result : Results)
	{
		if (Result.Position != Game::Race::PulledUpPosition)
			continue;

		RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "Pulled-up: " + Result.Racer->Name,
			290, PulledUpY + static_cast<int>(PulledUpRow) * RaceResultsPulledUpRowSpacing, { 255,255,255 });
		++PulledUpRow;
	}
}

void Game::Render::RaceScreenRenderer::DrawBackground()
{
	auto ScreenWidth = 1024, ScreenHeight = 768;

	//track
	auto TrackWidth = ScreenWidth, TrackHeight = ScreenHeight;

	int TrackDrawOffset = Camera.CameraX2 % unsigned int(TrackWidth);
	SDL_Rect RenderQuad1 = { 0 - TrackDrawOffset ,0, TrackWidth, TrackHeight };
	SDL_Rect RenderQuad2 = { RenderQuad1.w - TrackDrawOffset, 0, RenderQuad1.w, RenderQuad1.h };
	RenderImage(Context->Store.GetImage(Assets::ImageAssetTrack)->Texture, NULL, &RenderQuad1);
	RenderImage(Context->Store.GetImage(Assets::ImageAssetTrack)->Texture, NULL, &RenderQuad2);

	auto MountainWidth = TrackWidth * 2, MountainHeight = Context->Store.GetImage(Assets::ImageAssetMountains)->Surface->h;
	int xOffset2 = int(Camera.CameraX2 * 0.05) % MountainWidth;
	SDL_Rect RenderQuad3 = { 0 - xOffset2,0, MountainWidth, MountainHeight };
	SDL_Rect RenderQuad4 = { RenderQuad3.w - xOffset2,0, RenderQuad3.w, RenderQuad3.h };
	RenderImage(Context->Store.GetImage(Assets::ImageAssetMountains)->Texture, NULL, &RenderQuad3);
	RenderImage(Context->Store.GetImage(Assets::ImageAssetMountains)->Texture, NULL, &RenderQuad4);

	int xOffset3 = int(Camera.CameraX2 * 0.1) % Context->Store.GetImage(Assets::ImageAssetClouds)->Surface->w;
	SDL_Rect RenderQuad5 = { 0 - xOffset3,0, Context->Store.GetImage(Assets::ImageAssetClouds)->Surface->w , Context->Store.GetImage(Assets::ImageAssetClouds)->Surface->h };
	SDL_Rect RenderQuad6 = { RenderQuad5.w - xOffset3,0, Context->Store.GetImage(Assets::ImageAssetClouds)->Surface->w, Context->Store.GetImage(Assets::ImageAssetClouds)->Surface->h };
	RenderImage(Context->Store.GetImage(Assets::ImageAssetClouds)->Texture, NULL, &RenderQuad5);
	RenderImage(Context->Store.GetImage(Assets::ImageAssetClouds)->Texture, NULL, &RenderQuad6);

	int xOffset4 = int(Camera.CameraX2 * 0.75) % unsigned int(TrackWidth);
	SDL_Rect RenderQuad7 = { 0 - xOffset4,0, TrackWidth ,Context->Store.GetImage(Assets::ImageAssetStadium)->Surface->h };
	SDL_Rect RenderQuad8 = { RenderQuad7.w - xOffset4,0, TrackWidth, Context->Store.GetImage(Assets::ImageAssetStadium)->Surface->h };
	RenderImage(Context->Store.GetImage(Assets::ImageAssetStadium)->Texture, NULL, &RenderQuad7);
	RenderImage(Context->Store.GetImage(Assets::ImageAssetStadium)->Texture, NULL, &RenderQuad8);

	int BlockX = 10 - Camera.CameraX;
	SDL_Rect RenderQuad9 = { BlockX , 490, 243 * 0.75, 324 * 0.75 };
	RenderImage(Context->Store.GetImage(Assets::ImageAssetStartingBlock)->Texture, NULL, &RenderQuad9);
}

void Game::Render::RaceScreenRenderer::DrawProgressBar()
{
	//Render red filled quad
	int W = 200, H = 7;
	int X = (1024 / 2) - (W / 2), Y = 740;
	SDL_Rect fillRect = { X ,Y , W,H };
	SDL_SetRenderDrawColor(Context->MainRenderer, 0, 0, 0, 255);
	SDL_FRect fillRectF;
	SDL_RectToFRect(&fillRect, &fillRectF);
	SDL_RenderFillRect(Context->MainRenderer, &fillRectF);

	auto& Race = Screen->RaceSM->Data.ThisRace;
	const unsigned int TrackLength = Race.GetTrackLength();
	if (TrackLength == 0)
		return;
	for (auto Rank = Race.GetEntrants().size(); Rank > 0; --Rank)
	{
		//Draw racers circle
		auto* R = Race.GetByRank(static_cast<unsigned int>(Rank));
		if (R == nullptr) // skip null results
			continue;

		double ScaleFactor = double(R->Pos.X) / TrackLength;
		
		SDL_Rect RenderQuad = { X + (W * ScaleFactor) - 10, Y - 15, 32,32 };
		if (Screen->HasPlayerRacer() && R->GUID == Screen->PlayerGUID)
		{
			RenderQuad.x -= 8;
			RenderQuad.y -= 8;
			RenderQuad.w = RenderQuad.h = 48;
		}
		RenderImage(Context->Store.GetImage(Assets::ImageAssetHead)->Texture, NULL, &RenderQuad);
	}
}

unsigned int Game::Render::RaceScreenRenderer::Render()
{
	//start timer
	auto StartTime = SDL_GetTicks();

	auto& CurrentRace = Screen->RaceSM->Data.ThisRace;
	Camera.TrackLength = CurrentRace.GetTrackLength();
	Race::Racer* PlayerRacer = Screen->HasPlayerRacer() && CurrentRace.Contains(Screen->PlayerGUID)
		? CurrentRace.Get(Screen->PlayerGUID) : nullptr;
	const bool PlayerPulledUp = PlayerRacer != nullptr &&
		(CurrentRace.GetActiveEffects(PlayerRacer) & Race::RaceEffectPulledUp) != 0;
	// Follow the active player; otherwise track the current race leader.
	Race::Racer* CameraRacer = PlayerRacer != nullptr && !PlayerPulledUp
		? PlayerRacer : CurrentRace.CurrentWinner();
	const unsigned int CameraPosition = CameraRacer != nullptr
		? static_cast<unsigned int>(CameraRacer->Pos.X) : CurrentRace.CurrentWinnerDistance();
	Camera.PointAt(CameraPosition + RaceScreenCameraLookAhead);

	SDL_SetRenderDrawColor(Context->MainRenderer, 0, 0, 0, 0);
	SDL_RenderClear(Context->MainRenderer);

	DrawBackground();

	//draw racers
	const auto& Racers = Screen->RaceSM->Data.ThisRace.GetEntrants();
	for (std::size_t t = 0; t < Racers.size(); ++t)
		if (Racers[t] != nullptr)
			DrawRacer(*Racers[t], static_cast<unsigned int>(t));

	DrawProgressBar();

	DrawWinners();
	RenderDebugText();

	Display();
	auto EndTime = SDL_GetTicks();
	return EndTime - StartTime;
}

void Game::Render::RaceScreenRenderer::RenderDebugText()
{
	using namespace std;
	auto* Winner = Screen->RaceSM->Data.ThisRace.CurrentWinner();

	//top right
	string Body = "AppScreen: " + AppStateTypeToString(Screen->Type) + "\n" +
		"RaceState: " + RaceStateTypeToString(Screen->RaceSM->State->Type) + "\n" + (Screen->RaceSM->State->Type == Game::Race::RaceStateType::Failed ? "Race error: " + Game::Race::RaceErrorToString(Screen->RaceSM->Data.ThisRace.Result.Error) + "\n" : std::string()) +
		"Finished: " + to_string(Screen->RaceSM->Data.ThisRace.FinishedCount()) + "\n" +
		"Leader: " + (Winner != nullptr ? Winner->Name : "-") + "\n" +
		"Entrance Fee: " + to_string(Screen->RaceSM->Data.ThisRace.Financials.EntranceFee) + "\n" +
		"1st Prize: " + to_string(Screen->RaceSM->Data.ThisRace.Financials.FirstPlacePrize) + "\n" +
		"Camera.CameraX: " + to_string(Camera.CameraX) + "\n" +
		"Camera.CameraX2: " + to_string(Camera.CameraX2) + "\n";
	RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), Body, 850, 10, {255,0,0});

	//middle top
	RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), "CurrentTick: " + to_string(Screen->RaceSM->Data.CurrentTick), 400, 10, { 255,255,255 });
	RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), "SDL_GetTicks(): " + to_string(SDL_GetTicks()), 400, 22, { 255,255,255 });
	RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), "Race.CurrentTick: " + to_string(Screen->RaceSM->Data.ThisRace.CurrentTick), 400, 34, { 255,255,255 });

	//top left
	//if (Data->ShowFPS)	//TODO FIXME
	{
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), "Frame time: " + to_string(FPS.GetFrameTime()), 5, 10, { 255,255,255 });
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), "FPS: " + to_string(FPS.GetFPS()), 5, 22, { 255,255,255 });
	}
}
