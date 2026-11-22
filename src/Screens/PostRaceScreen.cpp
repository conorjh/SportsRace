#include "PostRaceScreen.h"
#include "..\Audio.h"
#include "..\App\App.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace
{
	constexpr int ResultsPanelLeft = 28;
	constexpr int ResultsPanelWidth = 968;
	constexpr int SummaryPanelTop = 90;
	constexpr int ResultsPanelTop = 218;
	constexpr int ProgressPanelTop = 492;
	constexpr SDL_Color ResultsBackground{ 23, 22, 20, 255 };
	constexpr SDL_Color ResultsPanelFill{ 34, 33, 30, 255 };
	constexpr SDL_Color ResultsPanelBorder{ 75, 70, 60, 255 };
	constexpr SDL_Color ResultsText{ 235, 229, 215, 255 };
	constexpr SDL_Color ResultsMuted{ 164, 155, 138, 255 };
	constexpr SDL_Color ResultsAccent{ 178, 61, 49, 255 };
	constexpr SDL_Color ResultsSelected{ 74, 43, 35, 255 };

	void DrawPanel(SDL_Renderer* Renderer, const SDL_FRect& Bounds, SDL_Color Accent)
	{
		SDL_SetRenderDrawColor(Renderer, ResultsPanelFill.r, ResultsPanelFill.g, ResultsPanelFill.b,
			ResultsPanelFill.a);
		SDL_RenderFillRect(Renderer, &Bounds);
		SDL_SetRenderDrawColor(Renderer, ResultsPanelBorder.r, ResultsPanelBorder.g,
			ResultsPanelBorder.b, ResultsPanelBorder.a);
		SDL_RenderRect(Renderer, &Bounds);
		const SDL_FRect AccentBar{ Bounds.x, Bounds.y, 4.0f, Bounds.h };
		SDL_SetRenderDrawColor(Renderer, Accent.r, Accent.g, Accent.b, Accent.a);
		SDL_RenderFillRect(Renderer, &AccentBar);
	}

	void DrawButton(Game::Render::BaseRenderer& Renderer, Game::Render::AppRenderContext* Context,
		const Game::GUI::Button& Button, bool Enabled = true)
	{
		const SDL_FRect Bounds{ static_cast<float>(Button.Rect.x), static_cast<float>(Button.Rect.y),
			static_cast<float>(Button.Rect.w), static_cast<float>(Button.Rect.h) };
		const SDL_Color Fill = !Enabled ? SDL_Color{ 63, 61, 55, 255 } :
			Button.IsMouseOver() ? SDL_Color{ 134, 54, 45, 255 } : SDL_Color{ 104, 42, 36, 255 };
		SDL_SetRenderDrawColor(Context->MainRenderer, Fill.r, Fill.g, Fill.b, Fill.a);
		SDL_RenderFillRect(Context->MainRenderer, &Bounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, ResultsAccent.r, ResultsAccent.g,
			ResultsAccent.b, ResultsAccent.a);
		SDL_RenderRect(Context->MainRenderer, &Bounds);
		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), Button.Text,
			Button.Rect.x + 8, Button.Rect.y + (Button.Rect.h - 16) / 2, ResultsText);
	}

	std::uint64_t GetDelta(std::uint64_t Current, std::uint64_t Before)
	{
		return Current >= Before ? Current - Before : 0;
	}

	std::string FormatDelta(std::uint64_t Current, std::uint64_t Before, const char* Suffix)
	{
		return "+" + std::to_string(GetDelta(Current, Before)) + Suffix;
	}

	std::string FormatPosition(unsigned int Position)
	{
		if (Position == Game::Race::PulledUpPosition)
			return "DNF";
		const unsigned int LastTwo = Position % 100;
		const char* Suffix = LastTwo >= 11 && LastTwo <= 13 ? "th" :
			Position % 10 == 1 ? "st" : Position % 10 == 2 ? "nd" : Position % 10 == 3 ? "rd" : "th";
		return std::to_string(Position) + Suffix;
	}

	std::string FormatTime(unsigned int TimeMs)
	{
		const unsigned int Minutes = TimeMs / 60000;
		const unsigned int Seconds = (TimeMs / 1000) % 60;
		const unsigned int Milliseconds = TimeMs % 1000;
		std::ostringstream Time;
		if (Minutes > 0)
			Time << Minutes << ':' << std::setw(2) << std::setfill('0') << Seconds;
		else
			Time << Seconds;
		Time << '.' << std::setw(3) << std::setfill('0') << Milliseconds;
		return Time.str();
	}

	std::int64_t GetNetCash(const Game::Race::Race& CurrentRace,
		const Game::Race::RacerRaceResult& Result)
	{
		std::int64_t NetCash = -static_cast<std::int64_t>(CurrentRace.Financials.EntranceFee);
		if (Result.Position == Game::Race::FirstPlaceRank)
			NetCash += static_cast<std::int64_t>(CurrentRace.Financials.FirstPlacePrize);
		else if (Result.Position == Game::Race::SecondPlaceRank)
			NetCash += static_cast<std::int64_t>(CurrentRace.Financials.SecondPlacePrize);
		else if (Result.Position == Game::Race::ThirdPlaceRank)
			NetCash += static_cast<std::int64_t>(CurrentRace.Financials.ThirdPlacePrize);
		return NetCash;
	}

	std::string FormatCash(std::int64_t NetCash)
	{
		return NetCash > 0 ? "$+" + std::to_string(NetCash) : "$" + std::to_string(NetCash);
	}

	std::string FormatSkillDelta(const char* SkillName, double Current, double Before)
	{
		const double Delta = Current - Before;
		if (std::abs(Delta) < 0.0001)
			return {};
		std::ostringstream Text;
		Text << SkillName << ' ' << (Delta > 0.0 ? "+" : "") << std::fixed << std::setprecision(1) << Delta;
		return Text.str();
	}
}

Game::Screens::PostRaceScreen::PostRaceScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO,
	App::AppData& _Data, const Race::Race& RaceToReview,
	std::vector<RacerProgressSnapshot> _PreRaceProgress, Race::RacerGUID _PlayerGUID,
	bool _PlayerRacerSupplied, std::string _EventName)
	: AppScreen(_Machine, _IO, _Data),
	CompletedRace(&RaceToReview),
	PreRaceProgress(std::move(_PreRaceProgress)),
	EventName(std::move(_EventName)),
	PlayerGUID(_PlayerGUID),
	PlayerRacerSupplied(_PlayerRacerSupplied),
	PreviousRacerButton(_IO, "< Racer", 28, 704, 110, 42),
	NextRacerButton(_IO, "Racer >", 148, 704, 110, 42),
	ContinueButton(_IO, "Continue", 844, 704, 152, 42)
{
	Type = AppScreenType::PostRaceScreen;
	for (const Race::RacerRaceResult& Result : CompletedRace->Result.RacerResults)
		OrderedResults.push_back(&Result);
	std::stable_sort(OrderedResults.begin(), OrderedResults.end(),
		[](const Race::RacerRaceResult* Left, const Race::RacerRaceResult* Right)
		{
			const bool LeftDidNotFinish = Left->Position == Race::PulledUpPosition;
			const bool RightDidNotFinish = Right->Position == Race::PulledUpPosition;
			if (LeftDidNotFinish != RightDidNotFinish)
				return !LeftDidNotFinish;
			if (Left->Position != Right->Position)
				return Left->Position < Right->Position;
			return Left->Ms < Right->Ms;
		});
	if (PlayerRacerSupplied)
		for (std::size_t Index = 0; Index < OrderedResults.size(); ++Index)
			if (OrderedResults[Index]->GUID.GUID == PlayerGUID.GUID)
			{
				SelectedResultIndex = Index;
				break;
			}
}

Game::Screens::AppScreen* Game::Screens::PostRaceScreen::Update()
{
	using namespace Game::Audio;
	PreviousRacerButton.Update();
	NextRacerButton.Update();
	ContinueButton.Update();
	if (!OrderedResults.empty() && PreviousRacerButton.HasMouseClicked())
		SelectedResultIndex = SelectedResultIndex == 0 ? OrderedResults.size() - 1 : SelectedResultIndex - 1;
	if (!OrderedResults.empty() && NextRacerButton.HasMouseClicked())
		SelectedResultIndex = (SelectedResultIndex + 1) % OrderedResults.size();
	if (IO.Esc || ContinueButton.HasMouseClicked())
	{
		IO.Player.Play(BuiltInSounds::Click);
		ScreenStack.Pop();
		ScreenStack.Pop();
		return ScreenStack.Top();
	}
	return this;
}

const Game::Race::Race& Game::Screens::PostRaceScreen::GetCompletedRace() const
{
	return *CompletedRace;
}

const Game::Race::RacerRaceResult* Game::Screens::PostRaceScreen::GetPlayerResult() const
{
	if (!PlayerRacerSupplied)
		return nullptr;
	const auto Result = std::find_if(OrderedResults.begin(), OrderedResults.end(),
		[this](const Race::RacerRaceResult* Candidate)
		{
			return Candidate->GUID.GUID == PlayerGUID.GUID;
		});
	return Result == OrderedResults.end() ? nullptr : *Result;
}

const Game::Race::RacerRaceResult* Game::Screens::PostRaceScreen::GetSelectedResult() const
{
	return GetResultAt(SelectedResultIndex);
}

const Game::Race::RacerRaceResult* Game::Screens::PostRaceScreen::GetResultAt(std::size_t Index) const
{
	return Index < OrderedResults.size() ? OrderedResults[Index] : nullptr;
}

const Game::Screens::RacerProgressSnapshot* Game::Screens::PostRaceScreen::GetProgressSnapshot(
	const Race::Racer* Runner) const
{
	const auto Snapshot = std::find_if(PreRaceProgress.begin(), PreRaceProgress.end(),
		[Runner](const RacerProgressSnapshot& Candidate) { return Candidate.Runner == Runner; });
	return Snapshot == PreRaceProgress.end() ? nullptr : &*Snapshot;
}

std::size_t Game::Screens::PostRaceScreen::GetResultCount() const
{
	return OrderedResults.size();
}

std::size_t Game::Screens::PostRaceScreen::GetSelectedResultIndex() const
{
	return SelectedResultIndex;
}

const std::string& Game::Screens::PostRaceScreen::GetEventName() const
{
	return EventName;
}

bool Game::Screens::PostRaceScreen::HasPlayerRacer() const
{
	return PlayerRacerSupplied;
}

Game::Render::PostRaceRenderer::PostRaceRenderer(AppRenderContext* Context)
	: BaseRenderer(Context)
{
}

unsigned int Game::Render::PostRaceRenderer::Render()
{
	const auto StartTime = SDL_GetTicks();
	SDL_SetRenderDrawColor(Context->MainRenderer, ResultsBackground.r, ResultsBackground.g,
		ResultsBackground.b, ResultsBackground.a);
	SDL_RenderClear(Context->MainRenderer);
	RenderText(Context->Store.GetFont(Assets::FontAssetMainFont), "POST-RACE REPORT", 28, 20, ResultsAccent);

	if (Screen == nullptr)
	{
		RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "Race results are unavailable.", 40, 100,
			ResultsText);
		Display();
		return static_cast<unsigned int>(SDL_GetTicks() - StartTime);
	}

	const Race::Race& CompletedRace = Screen->GetCompletedRace();
	const SDL_FRect SummaryBounds{ static_cast<float>(ResultsPanelLeft), static_cast<float>(SummaryPanelTop),
		static_cast<float>(ResultsPanelWidth), 112.0f };
	const SDL_FRect ResultsBounds{ static_cast<float>(ResultsPanelLeft), static_cast<float>(ResultsPanelTop),
		static_cast<float>(ResultsPanelWidth), 258.0f };
	const SDL_FRect ProgressBounds{ static_cast<float>(ResultsPanelLeft), static_cast<float>(ProgressPanelTop),
		static_cast<float>(ResultsPanelWidth), 178.0f };
	DrawPanel(Context->MainRenderer, SummaryBounds, ResultsAccent);
	DrawPanel(Context->MainRenderer, ResultsBounds, ResultsMuted);
	DrawPanel(Context->MainRenderer, ProgressBounds, ResultsAccent);
	RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), Screen->GetEventName(),
		ResultsPanelLeft + 18, SummaryPanelTop + 12, ResultsText);

	const Race::RacerRaceResult* PlayerResult = Screen->GetPlayerResult();
	if (Screen->HasPlayerRacer())
	{
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), "YOUR RACE", ResultsPanelLeft + 18,
			SummaryPanelTop + 42, ResultsAccent);
		if (PlayerResult != nullptr && PlayerResult->Racer != nullptr)
		{
			const Screens::RacerProgressSnapshot* Before = Screen->GetProgressSnapshot(PlayerResult->Racer);
			const std::uint64_t XpBefore = Before == nullptr ? PlayerResult->Racer->Skills.Xp : Before->Xp;
			const std::uint64_t PointsBefore = Before == nullptr
				? PlayerResult->Racer->CareerRecord.PositionPoints : Before->PositionPoints;
			const std::string ResultLine = PlayerResult->Racer->Name + "  |  " +
				FormatPosition(PlayerResult->Position) + "  |  " +
				(PlayerResult->Position == Race::PulledUpPosition ? "Pulled up" : FormatTime(PlayerResult->Ms)) +
				"  |  XP " + FormatDelta(PlayerResult->Racer->Skills.Xp, XpBefore, "") +
				"  |  Career points " + FormatDelta(PlayerResult->Racer->CareerRecord.PositionPoints,
					PointsBefore, "") + "  |  Net " + FormatCash(GetNetCash(CompletedRace, *PlayerResult));
			RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), ResultLine,
				ResultsPanelLeft + 18, SummaryPanelTop + 68, ResultsText);
		}
		else
			RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
				"Your result was not present in the completed field.", ResultsPanelLeft + 18,
				SummaryPanelTop + 68, ResultsMuted);
	}
	else
	{
		const Race::RacerRaceResult* Winner = Screen->GetResultAt(0);
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), "FIELD RESULTS  |  SPECTATING",
			ResultsPanelLeft + 18, SummaryPanelTop + 42, ResultsAccent);
		if (Winner != nullptr && Winner->Racer != nullptr)
			RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
				"Winner: " + Winner->Racer->Name + "  |  " + FormatTime(Winner->Ms) +
				"  |  Inspect each racer below for individual gains.", ResultsPanelLeft + 18,
				SummaryPanelTop + 68, ResultsText);
		else
			RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), "No classified finishers.",
				ResultsPanelLeft + 18, SummaryPanelTop + 68, ResultsMuted);
	}

	RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
		"PLACE  RACER                         FINISH       XP       CAREER PTS       NET CASH       PB",
		ResultsPanelLeft + 18, ResultsPanelTop + 18, ResultsMuted);
	for (std::size_t Index = 0; Index < Screen->GetResultCount(); ++Index)
	{
		const Race::RacerRaceResult* Result = Screen->GetResultAt(Index);
		if (Result == nullptr || Result->Racer == nullptr)
			continue;
		const Screens::RacerProgressSnapshot* Before = Screen->GetProgressSnapshot(Result->Racer);
		const std::uint64_t XpBefore = Before == nullptr ? Result->Racer->Skills.Xp : Before->Xp;
		const std::uint64_t PointsBefore = Before == nullptr
			? Result->Racer->CareerRecord.PositionPoints : Before->PositionPoints;
		const int RowY = ResultsPanelTop + 48 + static_cast<int>(Index) * 32;
		if (Index == Screen->GetSelectedResultIndex())
		{
			const SDL_FRect SelectedRow{ static_cast<float>(ResultsPanelLeft + 10), static_cast<float>(RowY - 4),
				static_cast<float>(ResultsPanelWidth - 20), 28.0f };
			SDL_SetRenderDrawColor(Context->MainRenderer, ResultsSelected.r, ResultsSelected.g,
				ResultsSelected.b, ResultsSelected.a);
			SDL_RenderFillRect(Context->MainRenderer, &SelectedRow);
		}
		const bool IsPlayer = Screen->HasPlayerRacer() &&
			PlayerResult != nullptr && PlayerResult->Racer == Result->Racer;
		const SDL_Color RowColor = IsPlayer ? ResultsAccent : ResultsText;
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), FormatPosition(Result->Position),
			ResultsPanelLeft + 18, RowY, RowColor);
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), Result->Racer->Name,
			ResultsPanelLeft + 88, RowY, RowColor);
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
			Result->Position == Race::PulledUpPosition ? "Pulled up" : FormatTime(Result->Ms),
			ResultsPanelLeft + 370, RowY, RowColor);
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
			FormatDelta(Result->Racer->Skills.Xp, XpBefore, ""), ResultsPanelLeft + 482, RowY, RowColor);
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
			FormatDelta(Result->Racer->CareerRecord.PositionPoints, PointsBefore, ""),
			ResultsPanelLeft + 572, RowY, RowColor);
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
			FormatCash(GetNetCash(CompletedRace, *Result)), ResultsPanelLeft + 712, RowY, RowColor);
		const bool NewPersonalBest = Before != nullptr && Result->Position != Race::PulledUpPosition &&
			Result->Racer->CareerRecord.BestTimeMs != 0 &&
			(Before->BestTimeMs == 0 || Result->Racer->CareerRecord.BestTimeMs < Before->BestTimeMs);
		if (NewPersonalBest)
			RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), "NEW",
				ResultsPanelLeft + 858, RowY, ResultsAccent);
	}
	if (Screen->GetResultCount() == 0)
		RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "No racer results are available.",
			ResultsPanelLeft + 18, ResultsPanelTop + 54, ResultsText);

	const Race::RacerRaceResult* SelectedResult = Screen->GetSelectedResult();
	if (SelectedResult != nullptr && SelectedResult->Racer != nullptr)
	{
		const Race::Racer& Runner = *SelectedResult->Racer;
		const Screens::RacerProgressSnapshot* Before = Screen->GetProgressSnapshot(&Runner);
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
			"PROGRESS: " + Runner.Name + "  |  " +
			FormatDelta(Runner.Skills.Xp, Before == nullptr ? Runner.Skills.Xp : Before->Xp, " XP") +
			"  |  Races " + FormatDelta(Runner.CareerRecord.RacesEntered,
				Before == nullptr ? Runner.CareerRecord.RacesEntered : Before->RacesEntered, "") +
			"  |  Record " + FormatDelta(Runner.CareerRecord.Wins,
				Before == nullptr ? Runner.CareerRecord.Wins : Before->Wins, "W") + "/" +
			FormatDelta(Runner.CareerRecord.Losses,
				Before == nullptr ? Runner.CareerRecord.Losses : Before->Losses, "L") +
			"  |  Net " + FormatCash(GetNetCash(CompletedRace, *SelectedResult)),
			ResultsPanelLeft + 18, ProgressPanelTop + 16, ResultsText);

		std::string SkillChanges;
		if (Before != nullptr)
			for (const std::string& Change : {
				FormatSkillDelta("Speed", Runner.Skills.Speed, Before->Speed),
				FormatSkillDelta("Endurance", Runner.Skills.Endurance, Before->Endurance),
				FormatSkillDelta("Acceleration", Runner.Skills.Acceleration, Before->Acceleration),
				FormatSkillDelta("Reflex", Runner.Skills.Reflex, Before->Reflex),
				FormatSkillDelta("Composure", Runner.Skills.Composure, Before->Composure) })
				if (!Change.empty())
				{
					if (!SkillChanges.empty())
						SkillChanges += "  |  ";
					SkillChanges += Change;
				}
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
			SkillChanges.empty() ? "Attribute gains: none recorded." : "Attribute gains: " + SkillChanges,
			ResultsPanelLeft + 18, ProgressPanelTop + 46, SkillChanges.empty() ? ResultsMuted : ResultsAccent);

		const bool NewPersonalBest = Before != nullptr && SelectedResult->Position != Race::PulledUpPosition &&
			Runner.CareerRecord.BestTimeMs != 0 &&
			(Before->BestTimeMs == 0 || Runner.CareerRecord.BestTimeMs < Before->BestTimeMs);
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
			"Best time: " + (Runner.CareerRecord.BestTimeMs == 0 ? std::string("not set") :
				FormatTime(Runner.CareerRecord.BestTimeMs)) +
			(NewPersonalBest ? "  |  New personal best!" : ""),
			ResultsPanelLeft + 18, ProgressPanelTop + 76, NewPersonalBest ? ResultsAccent : ResultsText);
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
			"Inspect racer " + std::to_string(Screen->GetSelectedResultIndex() + 1) + " of " +
			std::to_string(Screen->GetResultCount()) + " in the field.",
			ResultsPanelLeft + 18, ProgressPanelTop + 106, ResultsMuted);
	}
	else
		RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "No racer progress is available.",
			ResultsPanelLeft + 18, ProgressPanelTop + 24, ResultsMuted);

	DrawButton(*this, Context, Screen->PreviousRacerButton, Screen->GetResultCount() > 0);
	DrawButton(*this, Context, Screen->NextRacerButton, Screen->GetResultCount() > 0);
	DrawButton(*this, Context, Screen->ContinueButton);
	Display();
	return static_cast<unsigned int>(SDL_GetTicks() - StartTime);
}