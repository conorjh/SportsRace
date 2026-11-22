#include "RacerDBScreen.h"
#include "..\Appraisal.h"
#include "..\Audio.h"
#include "..\App\App.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <string>
#include <sstream>
#include <utility>

namespace
{
	constexpr int RacerDBScreenPanelTop = 94;
	constexpr int RacerDBScreenPanelHeight = 550;
	constexpr int RacerDBScreenListLeft = 32;
	constexpr int RacerDBScreenListWidth = 620;
	constexpr int RacerDBScreenDetailsLeft = 668;
	constexpr int RacerDBScreenDetailsWidth = 316;
	constexpr int RacerDBScreenHistoryRows = 7;
	constexpr int RacerDBScreenHistoryRowStep = 52;
	constexpr int RacerDBScreenRowLeft = RacerDBScreenListLeft + 16;
	constexpr int RacerDBScreenTableHeaderY = 159;
	constexpr int RacerDBScreenSortButtonHeight = 24;
	constexpr int RacerDBScreenRowTop = 190;
	constexpr int RacerDBScreenRowStep = 36;
	constexpr int RacerDBScreenRowHeight = 31;
	constexpr int RacerDBScreenScrollbarLeft = RacerDBScreenListLeft + RacerDBScreenListWidth - 22;
	constexpr int RacerDBScreenScrollbarWidth = 10;
	constexpr int RacerDBScreenScrollbarHeight = RacerDBScreenRowStep * Game::Screens::RacerDBScreen::VisibleRacerCount;
	constexpr int RacerDBScreenMinimumThumbHeight = 24;
	constexpr int RacerDBScreenRowWidth = RacerDBScreenScrollbarLeft - RacerDBScreenRowLeft - 8;
	constexpr int RacerDBScreenRankColumnWidth = 30;
	constexpr int RacerDBScreenNameColumnWidth = 130;
	constexpr int RacerDBScreenPositionPointsColumnWidth = 40;
	constexpr int RacerDBScreenWinLossColumnWidth = 24;
	constexpr int RacerDBScreenXpColumnWidth = 44;
	constexpr int RacerDBScreenRatingColumnWidth = 35;
	constexpr int RacerDBScreenNameButtonX = RacerDBScreenRowLeft + RacerDBScreenRankColumnWidth;
	constexpr int RacerDBScreenNameButtonWidth = RacerDBScreenRowWidth - RacerDBScreenRankColumnWidth;
	constexpr std::size_t RacerDBScreenRatingCount = 8;
	constexpr std::size_t RacerDBScreenSortColumnCount = 6 + RacerDBScreenRatingCount;
	constexpr int RacerDBScreenButtonY = 674;
	constexpr int RacerDBScreenButtonHeight = 48;
	constexpr int RacerDBScreenButtonTextHeight = 16;
	constexpr int RacerDBScreenRatingLabelWidth = 112;
	constexpr int RacerDBScreenRatingValueWidth = 40;
	constexpr int RacerDBScreenRatingBarTopOffset = 18;
	constexpr int RacerDBScreenRatingBarHeight = 7;
	constexpr int RacerDBScreenSkillRowTop = 250;
	constexpr int RacerDBScreenScoutingReportTop = 236;
	constexpr int RacerDBScreenSkillRowStep = 36;
	constexpr int RacerDBScreenPercentageScale = 100;
	constexpr SDL_Color RacerDBScreenBackgroundColor{ 23, 22, 20, 255 };
	constexpr SDL_Color RacerDBScreenPanelFillColor{ 34, 33, 30, 255 };
	constexpr SDL_Color RacerDBScreenPanelBorderColor{ 75, 70, 60, 255 };
	constexpr SDL_Color RacerDBScreenTextColor{ 235, 229, 215, 255 };
	constexpr SDL_Color RacerDBScreenMutedTextColor{ 164, 155, 138, 255 };
	constexpr SDL_Color RacerDBScreenAccentColor{ 178, 61, 49, 255 };
	constexpr SDL_Color RacerDBScreenBarBackgroundColor{ 60, 56, 49, 255 };
	constexpr SDL_Color RacerDBScreenBarFillColor{ 177, 70, 57, 255 };
	constexpr SDL_Color RacerDBScreenButtonFillColor{ 104, 42, 36, 255 };
	constexpr SDL_Color RacerDBScreenButtonHoverFillColor{ 134, 54, 45, 255 };
	constexpr SDL_Color RacerDBScreenSelectedFillColor{ 74, 43, 35, 255 };
	constexpr SDL_Color RacerDBScreenButtonBorderColor{ 170, 96, 72, 255 };
	constexpr SDL_Color RacerDBScreenScrollbarTrackColor{ 48, 45, 40, 255 };
	constexpr SDL_Color RacerDBScreenScrollbarThumbColor{ 112, 93, 73, 255 };
	constexpr SDL_Color RacerDBScreenScrollbarHoverColor{ 190, 129, 83, 255 };
	struct RacerDBSortColumnDefinition
	{
		Game::Screens::RacerDBSortColumn Column;
		const char* Label;
		int Width;
	};

	constexpr std::array<RacerDBSortColumnDefinition, RacerDBScreenSortColumnCount> RacerDBSortColumns{{
		{ Game::Screens::RacerDBSortColumn::Rank, "#", RacerDBScreenRankColumnWidth },
		{ Game::Screens::RacerDBSortColumn::Name, "RACER", RacerDBScreenNameColumnWidth },
		{ Game::Screens::RacerDBSortColumn::PositionPoints, "PTS", RacerDBScreenPositionPointsColumnWidth },
		{ Game::Screens::RacerDBSortColumn::Wins, "W", RacerDBScreenWinLossColumnWidth },
		{ Game::Screens::RacerDBSortColumn::Losses, "L", RacerDBScreenWinLossColumnWidth },
		{ Game::Screens::RacerDBSortColumn::Xp, "XP", RacerDBScreenXpColumnWidth },
		{ Game::Screens::RacerDBSortColumn::Reflex, "RFX", RacerDBScreenRatingColumnWidth },
		{ Game::Screens::RacerDBSortColumn::Composure, "CMP", RacerDBScreenRatingColumnWidth },
		{ Game::Screens::RacerDBSortColumn::Acceleration, "ACC", RacerDBScreenRatingColumnWidth },
		{ Game::Screens::RacerDBSortColumn::Endurance, "END", RacerDBScreenRatingColumnWidth },
		{ Game::Screens::RacerDBSortColumn::Speed, "SPD", RacerDBScreenRatingColumnWidth },
		{ Game::Screens::RacerDBSortColumn::Potential, "POT", RacerDBScreenRatingColumnWidth },
		{ Game::Screens::RacerDBSortColumn::Trainability, "TRN", RacerDBScreenRatingColumnWidth },
		{ Game::Screens::RacerDBSortColumn::Luck, "LCK", RacerDBScreenRatingColumnWidth }
	}};
	constexpr std::array<const char*, RacerDBScreenRatingCount> RacerDBScreenRatingColumnLabels{
		"Reflex", "Compos.", "Accel.", "Endur.", "Speed", "Potent.", "Train.", "Luck"
	};
	struct RacerRating
	{
		const char* Label;
		double BaseRating;
	};

	std::array<RacerRating, RacerDBScreenRatingCount> GetRacerRatings(const Game::Race::Racer& Runner)
	{
		return {{
			{ "Reflex", Runner.Skills.Reflex },
			{ "Composure", Runner.Skills.Composure },
			{ "Acceleration", Runner.Skills.Acceleration },
			{ "Endurance", Runner.Skills.Endurance },
			{ "Speed", Runner.Skills.Speed },
			{ "Potential", Runner.Skills.Potential },
			{ "Trainability", Runner.Skills.Trainability },
			{ "Luck", Runner.Skills.Luck }
		}};
	}

	int GetSortColumnLeft(std::size_t ColumnIndex)
	{
		int Left = RacerDBScreenRowLeft;
		for (std::size_t Index = 0; Index < ColumnIndex && Index < RacerDBSortColumns.size(); ++Index)
			Left += RacerDBSortColumns[Index].Width;
		return Left;
	}

	bool IsRankedAhead(const Game::Race::Racer* Left, const Game::Race::Racer* Right)
	{
		if (Left->CareerRecord.PositionPoints != Right->CareerRecord.PositionPoints)
			return Left->CareerRecord.PositionPoints > Right->CareerRecord.PositionPoints;
		if (Left->CareerRecord.Wins != Right->CareerRecord.Wins)
			return Left->CareerRecord.Wins > Right->CareerRecord.Wins;
		if (Left->Skills.Xp != Right->Skills.Xp)
			return Left->Skills.Xp > Right->Skills.Xp;
		if (Left->CareerRecord.BestTimeMs != Right->CareerRecord.BestTimeMs)
			return Left->CareerRecord.BestTimeMs != 0 && (Right->CareerRecord.BestTimeMs == 0 ||
				Left->CareerRecord.BestTimeMs < Right->CareerRecord.BestTimeMs);
		if (Left->Name != Right->Name)
			return Left->Name < Right->Name;
		return Left->GUID.GUID < Right->GUID.GUID;
	}

	std::string FormatRacerName(const std::string& Name)
	{
		constexpr std::size_t MaximumCharacters = 20;
		return Name.size() > MaximumCharacters ? Name.substr(0, MaximumCharacters - 3) + "..." : Name;
	}

	SDL_Rect GetScrollbarTrackBounds()
	{
		return { RacerDBScreenScrollbarLeft, RacerDBScreenRowTop,
			RacerDBScreenScrollbarWidth, RacerDBScreenScrollbarHeight };
	}

	SDL_Rect GetRacerListBounds()
	{
		return { RacerDBScreenListLeft, RacerDBScreenRowTop,
			RacerDBScreenListWidth, RacerDBScreenScrollbarHeight };
	}

	SDL_Point GetMousePosition(const Game::App::AppIO& IO)
	{
		return { static_cast<int>(IO.MousePosition.x), static_cast<int>(IO.MousePosition.y) };
	}

	void DrawPanel(SDL_Renderer* Renderer, const SDL_FRect& Bounds, SDL_Color AccentColor)
	{
		SDL_SetRenderDrawColor(Renderer, RacerDBScreenPanelFillColor.r, RacerDBScreenPanelFillColor.g,
			RacerDBScreenPanelFillColor.b, RacerDBScreenPanelFillColor.a);
		SDL_RenderFillRect(Renderer, &Bounds);
		SDL_SetRenderDrawColor(Renderer, RacerDBScreenPanelBorderColor.r, RacerDBScreenPanelBorderColor.g,
			RacerDBScreenPanelBorderColor.b, RacerDBScreenPanelBorderColor.a);
		SDL_RenderRect(Renderer, &Bounds);
		const SDL_FRect AccentBounds{ Bounds.x, Bounds.y, 4.0f, Bounds.h };
		SDL_SetRenderDrawColor(Renderer, AccentColor.r, AccentColor.g, AccentColor.b, AccentColor.a);
		SDL_RenderFillRect(Renderer, &AccentBounds);
	}

	void DrawButton(Game::Render::BaseRenderer& Renderer, Game::Render::AppRenderContext* Context,
		Game::GUI::Button& Button, bool IsSelected = false, bool UseCompactText = false)
	{
		SDL_FRect Bounds;
		SDL_RectToFRect(&Button.Rect, &Bounds);
		const SDL_Color FillColor = IsSelected ? RacerDBScreenSelectedFillColor :
			Button.IsMouseOver() ? RacerDBScreenButtonHoverFillColor : RacerDBScreenButtonFillColor;
		SDL_SetRenderDrawColor(Context->MainRenderer, FillColor.r, FillColor.g, FillColor.b, FillColor.a);
		SDL_RenderFillRect(Context->MainRenderer, &Bounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, RacerDBScreenButtonBorderColor.r,
			RacerDBScreenButtonBorderColor.g, RacerDBScreenButtonBorderColor.b, RacerDBScreenButtonBorderColor.a);
		SDL_RenderRect(Context->MainRenderer, &Bounds);
		const Game::Assets::FontAssetId Font = UseCompactText
			? Game::Assets::FontAssetDebugFont : Game::Assets::FontAssetInfoFont;
		Renderer.RenderText(Context->Store.GetFont(Font), Button.Text,
			Button.x + 12, Button.y + (Button.h - RacerDBScreenButtonTextHeight) / 2, RacerDBScreenTextColor);
	}

	void DrawSortButton(Game::Render::BaseRenderer& Renderer, Game::Render::AppRenderContext* Context,
		Game::GUI::Button& Button, const char* Label, bool IsSelected, bool SortDescending)
	{
		const SDL_FRect Bounds{ static_cast<float>(Button.Rect.x), static_cast<float>(Button.Rect.y),
			static_cast<float>(Button.Rect.w), static_cast<float>(Button.Rect.h) };
		const SDL_Color FillColor = IsSelected ? RacerDBScreenSelectedFillColor :
			Button.IsMouseOver() ? RacerDBScreenButtonHoverFillColor : RacerDBScreenPanelFillColor;
		SDL_SetRenderDrawColor(Context->MainRenderer, FillColor.r, FillColor.g, FillColor.b, FillColor.a);
		SDL_RenderFillRect(Context->MainRenderer, &Bounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, RacerDBScreenPanelBorderColor.r,
			RacerDBScreenPanelBorderColor.g, RacerDBScreenPanelBorderColor.b, RacerDBScreenPanelBorderColor.a);
		SDL_RenderRect(Context->MainRenderer, &Bounds);
		std::string Text = Label;
		if (IsSelected)
			Text += SortDescending ? " v" : " ^";
		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), Text,
			Button.Rect.x + 3, Button.Rect.y + 7, IsSelected ? RacerDBScreenTextColor : RacerDBScreenMutedTextColor);
	}

	std::string FormatRating(double Rating)
	{
		return std::to_string(static_cast<int>(std::lround(std::clamp(Rating, 0.0, 100.0))));
	}

	std::string FormatBaseRating(double Rating)
	{
		std::ostringstream FormattedRating;
		FormattedRating << std::fixed << std::setprecision(2) << Rating;
		std::string Value = FormattedRating.str();
		while (!Value.empty() && Value.back() == '0')
			Value.pop_back();
		if (!Value.empty() && Value.back() == '.')
			Value.pop_back();
		return Value;
	}

	std::string FormatBestTime(unsigned int TimeMs)
	{
		if (TimeMs == 0)
			return "n/a";

		std::ostringstream FormattedTime;
		FormattedTime << std::fixed << std::setprecision(2) <<
			static_cast<double>(TimeMs) / 1000.0 << 's';
		return FormattedTime.str();
	}

	std::string FormatHistoryResult(const Game::Race::RacerCareerHistoryEntry& Entry)
	{
		constexpr std::uint64_t MillisecondsPerWorldDay = 86400000;
		const std::string Place = Entry.Position == Game::Race::PulledUpPosition
			? "DNF" : "#" + std::to_string(Entry.Position);
		const std::string Distance = Entry.TrackLength == 0 ? "?m" :
			std::to_string(Entry.TrackLength * 100 / static_cast<unsigned int>(Game::Race::TrackLength::Standard100m)) + "m";
		const std::string Time = Entry.FinishTimeMs == 0 ? "DNF" : FormatBestTime(Entry.FinishTimeMs);
		return "Day " + std::to_string(Entry.WorldTimeMs / MillisecondsPerWorldDay + 1) + " | " +
			Place + " | " + Distance + (Time == "DNF" ? "" : " | " + Time);
	}

	std::string FormatCompactCount(std::uint64_t Value)
	{
		if (Value < 1000)
			return std::to_string(Value);

		const bool IsMillions = Value >= 1000000;
		const double ScaledValue = static_cast<double>(Value) / (IsMillions ? 1000000.0 : 1000.0);
		std::ostringstream FormattedValue;
		FormattedValue << std::fixed << std::setprecision(ScaledValue < 10.0 ? 1 : 0) << ScaledValue <<
			(IsMillions ? 'M' : 'k');
		return FormattedValue.str();
	}

	void DrawRatingRow(Game::Render::BaseRenderer& Renderer, Game::Render::AppRenderContext* Context,
		const std::string& Label, const std::string& Value, double NormalizedValue, int X, int Y, int Width)
	{
		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), Label,
			X, Y, RacerDBScreenTextColor);
		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), Value,
			X + Width - RacerDBScreenRatingValueWidth, Y, RacerDBScreenTextColor);

		const int BarWidth = Width - RacerDBScreenRatingLabelWidth - RacerDBScreenRatingValueWidth;
		const SDL_FRect BarBounds{ static_cast<float>(X + RacerDBScreenRatingLabelWidth),
			static_cast<float>(Y + RacerDBScreenRatingBarTopOffset), static_cast<float>(BarWidth),
			static_cast<float>(RacerDBScreenRatingBarHeight) };
		SDL_SetRenderDrawColor(Context->MainRenderer, RacerDBScreenBarBackgroundColor.r,
			RacerDBScreenBarBackgroundColor.g, RacerDBScreenBarBackgroundColor.b, RacerDBScreenBarBackgroundColor.a);
		SDL_RenderFillRect(Context->MainRenderer, &BarBounds);
		const double Fraction = std::clamp(NormalizedValue, 0.0, 1.0);
		if (Fraction > 0.0)
		{
			const SDL_FRect FillBounds{ BarBounds.x, BarBounds.y,
				static_cast<float>(BarWidth * Fraction), BarBounds.h };
			SDL_SetRenderDrawColor(Context->MainRenderer, RacerDBScreenBarFillColor.r,
				RacerDBScreenBarFillColor.g, RacerDBScreenBarFillColor.b, RacerDBScreenBarFillColor.a);
			SDL_RenderFillRect(Context->MainRenderer, &FillBounds);
		}
	}
}

Game::Screens::RacerDBScreen::RacerDBScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO,
	App::AppData& _Data, Race::RacerDB& _Database)
	: AppScreen(_Machine, _IO, _Data),
	Database(_Database),
	ExitButton(_IO, "Back", 824, RacerDBScreenButtonY, 160, RacerDBScreenButtonHeight),
	HistoryViewButton(_IO, "HISTORY", 844, 104, 116, 30)
{
	Type = AppScreenType::RacerDBScreen;
	SortButtons.reserve(RacerDBSortColumns.size());
	for (std::size_t Index = 0; Index < RacerDBSortColumns.size(); ++Index)
		SortButtons.emplace_back(_IO, RacerDBSortColumns[Index].Label, GetSortColumnLeft(Index),
			RacerDBScreenTableHeaderY, RacerDBSortColumns[Index].Width, RacerDBScreenSortButtonHeight);
	RacerButtons.reserve(VisibleRacerCount);
	for (std::size_t Index = 0; Index < VisibleRacerCount; ++Index)
		RacerButtons.emplace_back(_IO, "", RacerDBScreenNameButtonX,
			RacerDBScreenRowTop + static_cast<int>(Index) * RacerDBScreenRowStep,
			RacerDBScreenNameButtonWidth, RacerDBScreenRowHeight);
	RefreshRacers();
}

void Game::Screens::RacerDBScreen::Entry()
{
	RefreshRacers();
}

Game::Screens::AppScreen* Game::Screens::RacerDBScreen::Update()
{
	if (Racers.size() != Database.Size())
		RefreshRacers();
	HistoryViewButton.Text = ShowingHistory ? "PROFILE" : "HISTORY";
	HistoryViewButton.Update();
	if (HistoryViewButton.HasMouseClicked())
		ShowingHistory = !ShowingHistory;

	ExitButton.Update();
	if (ExitButton.HasMouseClicked() || IO.Esc)
	{
		IO.Player.Play(Audio::BuiltInSounds::Click);
		ScreenStack.Pop();
		return ScreenStack.Top();
	}
	for (std::size_t Index = 0; Index < SortButtons.size(); ++Index)
	{
		SortButtons[Index].Update();
		if (SortButtons[Index].HasMouseClicked())
		{
			SortByColumn(RacerDBSortColumns[Index].Column);
			break;
		}
	}

	const SDL_Point MousePosition = GetMousePosition(IO);
	const SDL_Rect ListBounds = GetRacerListBounds();
	if (IO.MouseWheelY != 0.0f && SDL_PointInRect(&MousePosition, &ListBounds))
	{
		WheelScrollRemainder -= IO.MouseWheelY * 3.0f;
		const int RowDelta = static_cast<int>(WheelScrollRemainder);
		if (RowDelta != 0)
		{
			const long long NewOffset = static_cast<long long>(ScrollOffset) + RowDelta;
			SetScrollOffset(static_cast<std::size_t>(std::clamp<long long>(NewOffset, 0,
				static_cast<long long>(GetMaxScrollOffset()))));
			WheelScrollRemainder -= static_cast<float>(RowDelta);
		}
	}

	UpdateScrollbar();

	for (std::size_t Row = 0; Row < RacerButtons.size(); ++Row)
	{
		const std::size_t RacerIndex = ScrollOffset + Row;
		if (GetRacerAt(RacerIndex) == nullptr)
			continue;

		RacerButtons[Row].Update();
		if (RacerButtons[Row].HasMouseClicked())
			SelectedRacer = GetRacerAt(RacerIndex);
	}

	return this;
}

void Game::Screens::RacerDBScreen::RefreshRacers()
{
	Race::Racer* PreviousSelection = SelectedRacer;
	Racers = Database.GetAll();
	std::vector<Race::Racer*> RankedRacers = Racers;
	std::stable_sort(RankedRacers.begin(), RankedRacers.end(), IsRankedAhead);
	RacerRanks.clear();
	for (std::size_t Index = 0; Index < RankedRacers.size(); ++Index)
		RacerRanks.emplace(RankedRacers[Index], Index + 1);
	SortRacers();
	ScrollOffset = std::min(ScrollOffset, GetMaxScrollOffset());

	SelectedRacer = nullptr;
	for (Race::Racer* Runner : Racers)
		if (Runner == PreviousSelection)
		{
			SelectedRacer = Runner;
			break;
		}
	if (SelectedRacer == nullptr && !Racers.empty())
		SelectedRacer = Racers.front();

	RefreshVisibleRows();
}

void Game::Screens::RacerDBScreen::RefreshVisibleRows()
{
	for (std::size_t Row = 0; Row < RacerButtons.size(); ++Row)
	{
		Race::Racer* Runner = GetRacerAt(ScrollOffset + Row);
		RacerButtons[Row].Text = Runner == nullptr ? "" : FormatRacerName(Runner->Name);
	}
}

void Game::Screens::RacerDBScreen::SortRacers()
{
	const std::size_t SortColumnIndex = static_cast<std::size_t>(SortColumn);
	std::stable_sort(Racers.begin(), Racers.end(), [this, SortColumnIndex](const Race::Racer* Left,
		const Race::Racer* Right)
		{
			int Comparison = 0;
			switch (SortColumn)
			{
			case RacerDBSortColumn::Rank:
				Comparison = RacerRanks.at(Left) < RacerRanks.at(Right) ? -1 :
					RacerRanks.at(Left) > RacerRanks.at(Right) ? 1 : 0;
				break;
			case RacerDBSortColumn::Name:
				Comparison = Left->Name < Right->Name ? -1 : Left->Name > Right->Name ? 1 : 0;
				break;
			case RacerDBSortColumn::PositionPoints:
				Comparison = Left->CareerRecord.PositionPoints < Right->CareerRecord.PositionPoints ? -1 :
					Left->CareerRecord.PositionPoints > Right->CareerRecord.PositionPoints ? 1 : 0;
				break;
			case RacerDBSortColumn::Wins:
				Comparison = Left->CareerRecord.Wins < Right->CareerRecord.Wins ? -1 :
					Left->CareerRecord.Wins > Right->CareerRecord.Wins ? 1 : 0;
				break;
			case RacerDBSortColumn::Losses:
				Comparison = Left->CareerRecord.Losses < Right->CareerRecord.Losses ? -1 :
					Left->CareerRecord.Losses > Right->CareerRecord.Losses ? 1 : 0;
				break;
			case RacerDBSortColumn::Xp:
				Comparison = Left->Skills.Xp < Right->Skills.Xp ? -1 :
					Left->Skills.Xp > Right->Skills.Xp ? 1 : 0;
				break;
			default:
			{
				const std::size_t RatingIndex = SortColumnIndex -
					static_cast<std::size_t>(RacerDBSortColumn::Reflex);
				const auto LeftRatings = GetRacerRatings(*Left);
				const auto RightRatings = GetRacerRatings(*Right);
				const double LeftRating = AppraiseRating(LeftRatings[RatingIndex].BaseRating,
					Game::Appraisal::HashRacer(*Left), RatingIndex + 1);
				const double RightRating = AppraiseRating(RightRatings[RatingIndex].BaseRating,
					Game::Appraisal::HashRacer(*Right), RatingIndex + 1);
				Comparison = LeftRating < RightRating ? -1 : LeftRating > RightRating ? 1 : 0;
				break;
			}
			}
			if (Comparison == 0)
				return Left->GUID.GUID < Right->GUID.GUID;
			return SortDescending ? Comparison > 0 : Comparison < 0;
		});
	RefreshVisibleRows();
}

void Game::Screens::RacerDBScreen::SetScrollOffset(std::size_t Offset)
{
	ScrollOffset = std::min(Offset, GetMaxScrollOffset());
	RefreshVisibleRows();
}

void Game::Screens::RacerDBScreen::UpdateScrollbar()
{
	const SDL_Point MousePosition = GetMousePosition(IO);
	const SDL_Rect TrackBounds = GetScrollbarTrackBounds();
	const float ThumbHeight = GetScrollbarThumbHeight();
	const SDL_Rect ThumbBounds{ TrackBounds.x, static_cast<int>(std::lround(GetScrollbarThumbY())),
		TrackBounds.w, static_cast<int>(std::lround(ThumbHeight)) };
	const bool IsLeftButtonDown = (IO.MouseButtons & SDL_BUTTON_LMASK) != 0;
	const bool IsPointerOverTrack = SDL_PointInRect(&MousePosition, &TrackBounds);
	const bool WasLeftButtonDown = WasLeftMouseDown;
	WasLeftMouseDown = IsLeftButtonDown;

	if (!IsLeftButtonDown)
	{
		ScrollbarDragging = false;
		return;
	}

	if (!ScrollbarDragging && !WasLeftButtonDown && IsPointerOverTrack)
	{
		ScrollbarDragging = true;
		ScrollbarDragOffset = SDL_PointInRect(&MousePosition, &ThumbBounds)
			? MousePosition.y - ThumbBounds.y : static_cast<int>(ThumbHeight / 2.0f);
	}
	if (!ScrollbarDragging)
		return;

	const float ThumbTravel = TrackBounds.h - ThumbHeight;
	if (ThumbTravel <= 0.0f || GetMaxScrollOffset() == 0)
	{
		SetScrollOffset(0);
		return;
	}

	const float ThumbTop = std::clamp(static_cast<float>(MousePosition.y - ScrollbarDragOffset),
		static_cast<float>(TrackBounds.y), static_cast<float>(TrackBounds.y) + ThumbTravel);
	const double ScrollFraction = (ThumbTop - TrackBounds.y) / ThumbTravel;
	SetScrollOffset(static_cast<std::size_t>(std::lround(ScrollFraction * GetMaxScrollOffset())));
}

std::vector<Game::GUI::Button>& Game::Screens::RacerDBScreen::GetRacerButtons()
{
	return RacerButtons;
}

std::vector<Game::GUI::Button>& Game::Screens::RacerDBScreen::GetSortButtons()
{
	return SortButtons;
}

void Game::Screens::RacerDBScreen::SortByColumn(RacerDBSortColumn Column)
{
	if (SortColumn == Column)
		SortDescending = !SortDescending;
	else
	{
		SortColumn = Column;
		SortDescending = Column != RacerDBSortColumn::Rank && Column != RacerDBSortColumn::Name;
	}
	ScrollOffset = 0;
	SortRacers();
}

Game::Race::Racer* Game::Screens::RacerDBScreen::GetRacerAt(std::size_t Index) const
{
	return Index < Racers.size() ? Racers[Index] : nullptr;
}

Game::Race::Racer* Game::Screens::RacerDBScreen::GetSelectedRacer() const
{
	return SelectedRacer;
}

bool Game::Screens::RacerDBScreen::IsShowingHistory() const
{
	return ShowingHistory;
}

std::size_t Game::Screens::RacerDBScreen::GetRankFor(const Race::Racer* Runner) const
{
	const auto Rank = RacerRanks.find(Runner);
	return Rank == RacerRanks.end() ? 0 : Rank->second;
}

Game::Screens::RacerDBSortColumn Game::Screens::RacerDBScreen::GetSortColumn() const
{
	return SortColumn;
}

bool Game::Screens::RacerDBScreen::IsSortDescending() const
{
	return SortDescending;
}

std::size_t Game::Screens::RacerDBScreen::GetScrollOffset() const
{
	return ScrollOffset;
}

float Game::Screens::RacerDBScreen::GetScrollbarThumbY() const
{
	const SDL_Rect TrackBounds = GetScrollbarTrackBounds();
	const float ThumbTravel = TrackBounds.h - GetScrollbarThumbHeight();
	if (ThumbTravel <= 0.0f || GetMaxScrollOffset() == 0)
		return static_cast<float>(TrackBounds.y);
	return static_cast<float>(TrackBounds.y) + ThumbTravel *
		static_cast<float>(ScrollOffset) / static_cast<float>(GetMaxScrollOffset());
}

float Game::Screens::RacerDBScreen::GetScrollbarThumbHeight() const
{
	const SDL_Rect TrackBounds = GetScrollbarTrackBounds();
	if (Racers.size() <= VisibleRacerCount)
		return static_cast<float>(TrackBounds.h);
	const float ProportionalHeight = static_cast<float>(TrackBounds.h) * VisibleRacerCount /
		static_cast<float>(Racers.size());
	return std::clamp(ProportionalHeight, static_cast<float>(RacerDBScreenMinimumThumbHeight),
		static_cast<float>(TrackBounds.h));
}

bool Game::Screens::RacerDBScreen::IsScrollbarHovered() const
{
	const SDL_Point MousePosition = GetMousePosition(IO);
	const SDL_Rect TrackBounds = GetScrollbarTrackBounds();
	const SDL_Rect ThumbBounds{ TrackBounds.x, static_cast<int>(std::lround(GetScrollbarThumbY())),
		TrackBounds.w, static_cast<int>(std::lround(GetScrollbarThumbHeight())) };
	return SDL_PointInRect(&MousePosition, &TrackBounds) || SDL_PointInRect(&MousePosition, &ThumbBounds);
}

bool Game::Screens::RacerDBScreen::IsScrollbarDragging() const
{
	return ScrollbarDragging;
}

bool Game::Screens::RacerDBScreen::IsRatingRowHovered(std::size_t Index) const
{
	if (Index >= RacerDBScreenRatingCount)
		return false;

	const SDL_Point MousePosition = GetMousePosition(IO);
	const SDL_Rect RowBounds{ RacerDBScreenDetailsLeft + 24,
		RacerDBScreenSkillRowTop + static_cast<int>(Index) * RacerDBScreenSkillRowStep,
		RacerDBScreenDetailsWidth - 48, RacerDBScreenSkillRowStep };
	return SDL_PointInRect(&MousePosition, &RowBounds);
}

double Game::Screens::RacerDBScreen::AppraiseRating(double BaseRating, std::uint64_t TargetFingerprint,
	std::uint64_t StatIdentity) const
{
	return Game::Appraisal::AppraiseRating(BaseRating, Data.Profile->Accuracy,
		Data.Profile->AppraisalIdentity, TargetFingerprint, StatIdentity);
}

std::size_t Game::Screens::RacerDBScreen::GetRacerCount() const
{
	return Racers.size();
}

std::size_t Game::Screens::RacerDBScreen::GetMaxScrollOffset() const
{
	return Racers.size() > VisibleRacerCount ? Racers.size() - VisibleRacerCount : 0;
}

Game::Render::RacerDBScreenRenderer::RacerDBScreenRenderer(Game::Render::AppRenderContext* Context)
	: BaseRenderer(Context),
	Screen(nullptr)
{
}

unsigned int Game::Render::RacerDBScreenRenderer::Render()
{
	using namespace Game::Assets;
	const auto StartTime = SDL_GetTicks();
	SDL_SetRenderDrawColor(Context->MainRenderer, RacerDBScreenBackgroundColor.r,
		RacerDBScreenBackgroundColor.g, RacerDBScreenBackgroundColor.b, RacerDBScreenBackgroundColor.a);
	SDL_RenderClear(Context->MainRenderer);
	RenderText(Context->Store.GetFont(FontAssetMainFont), "RACER DATABASE",
		RacerDBScreenListLeft, 26, RacerDBScreenAccentColor);

	const SDL_FRect ListPanel{ static_cast<float>(RacerDBScreenListLeft),
		static_cast<float>(RacerDBScreenPanelTop), static_cast<float>(RacerDBScreenListWidth),
		static_cast<float>(RacerDBScreenPanelHeight) };
	DrawPanel(Context->MainRenderer, ListPanel, RacerDBScreenAccentColor);
	const SDL_FRect DetailsPanel{ static_cast<float>(RacerDBScreenDetailsLeft),
		static_cast<float>(RacerDBScreenPanelTop), static_cast<float>(RacerDBScreenDetailsWidth),
		static_cast<float>(RacerDBScreenPanelHeight) };
	DrawPanel(Context->MainRenderer, DetailsPanel, RacerDBScreenAccentColor);

	RenderText(Context->Store.GetFont(FontAssetInfoFont), "CAREER RACERS",
		RacerDBScreenListLeft + 18, 112, RacerDBScreenAccentColor);
	RenderText(Context->Store.GetFont(FontAssetDebugFont),
		std::to_string(Screen->GetRacerCount()) + " runners - positional leaderboard; click a heading to sort",
		RacerDBScreenListLeft + 18, 143, RacerDBScreenMutedTextColor);
	auto& SortButtons = Screen->GetSortButtons();
	for (std::size_t Index = 0; Index < SortButtons.size(); ++Index)
		DrawSortButton(*this, Context, SortButtons[Index], RacerDBSortColumns[Index].Label,
			Screen->GetSortColumn() == RacerDBSortColumns[Index].Column, Screen->IsSortDescending());

	auto& RacerButtons = Screen->GetRacerButtons();
	for (std::size_t Row = 0; Row < RacerButtons.size(); ++Row)
	{
		Game::Race::Racer* ListedRacer = Screen->GetRacerAt(Screen->GetScrollOffset() + Row);
		if (ListedRacer == nullptr)
			continue;

		const bool IsSelected = ListedRacer == Screen->GetSelectedRacer();
		const int RowY = RacerDBScreenRowTop + static_cast<int>(Row) * RacerDBScreenRowStep;
		const SDL_FRect RankCell{ static_cast<float>(RacerDBScreenRowLeft), static_cast<float>(RowY),
			static_cast<float>(RacerDBScreenRankColumnWidth), static_cast<float>(RacerDBScreenRowHeight) };
		DrawPanel(Context->MainRenderer, RankCell,
			IsSelected ? RacerDBScreenSelectedFillColor : RacerDBScreenPanelFillColor);
		RenderText(Context->Store.GetFont(FontAssetDebugFont), std::to_string(Screen->GetRankFor(ListedRacer)),
			RacerDBScreenRowLeft + 7, RowY + 8, RacerDBScreenTextColor);
		DrawButton(*this, Context, RacerButtons[Row], IsSelected, true);
		RenderText(Context->Store.GetFont(FontAssetDebugFont),
			FormatCompactCount(ListedRacer->CareerRecord.PositionPoints), GetSortColumnLeft(2) + 4,
			RowY + 8, RacerDBScreenTextColor);
		RenderText(Context->Store.GetFont(FontAssetDebugFont), FormatCompactCount(ListedRacer->CareerRecord.Wins),
			GetSortColumnLeft(3) + 4, RowY + 8, RacerDBScreenTextColor);
		RenderText(Context->Store.GetFont(FontAssetDebugFont), FormatCompactCount(ListedRacer->CareerRecord.Losses),
			GetSortColumnLeft(4) + 4, RowY + 8, RacerDBScreenTextColor);
		RenderText(Context->Store.GetFont(FontAssetDebugFont), FormatCompactCount(ListedRacer->Skills.Xp),
			GetSortColumnLeft(5) + 4, RowY + 8, RacerDBScreenTextColor);
		const auto Stats = GetRacerRatings(*ListedRacer);
		const std::uint64_t TargetFingerprint = Game::Appraisal::HashRacer(*ListedRacer);
		for (std::size_t Index = 0; Index < Stats.size(); ++Index)
		{
			const double AppraisedRating = Screen->AppraiseRating(Stats[Index].BaseRating,
				TargetFingerprint, Index + 1);
			RenderText(Context->Store.GetFont(FontAssetDebugFont), FormatRating(AppraisedRating),
				GetSortColumnLeft(Index + 6) + 4, RowY + 8,
				RacerDBScreenTextColor);
		}
	}

	const SDL_FRect ScrollbarTrack{ static_cast<float>(RacerDBScreenScrollbarLeft),
		static_cast<float>(RacerDBScreenRowTop), static_cast<float>(RacerDBScreenScrollbarWidth),
		static_cast<float>(RacerDBScreenScrollbarHeight) };
	SDL_SetRenderDrawColor(Context->MainRenderer, RacerDBScreenScrollbarTrackColor.r,
		RacerDBScreenScrollbarTrackColor.g, RacerDBScreenScrollbarTrackColor.b, RacerDBScreenScrollbarTrackColor.a);
	SDL_RenderFillRect(Context->MainRenderer, &ScrollbarTrack);
	const SDL_Color ThumbColor = Screen->IsScrollbarHovered() || Screen->IsScrollbarDragging()
		? RacerDBScreenScrollbarHoverColor : RacerDBScreenScrollbarThumbColor;
	const SDL_FRect ScrollbarThumb{ ScrollbarTrack.x, Screen->GetScrollbarThumbY(), ScrollbarTrack.w,
		Screen->GetScrollbarThumbHeight() };
	SDL_SetRenderDrawColor(Context->MainRenderer, ThumbColor.r, ThumbColor.g, ThumbColor.b, ThumbColor.a);
	SDL_RenderFillRect(Context->MainRenderer, &ScrollbarThumb);

	const std::size_t FirstVisible = Screen->GetRacerCount() == 0 ? 0 : Screen->GetScrollOffset() + 1;
	const std::size_t LastVisible = std::min(Screen->GetScrollOffset() + Screens::RacerDBScreen::VisibleRacerCount,
		Screen->GetRacerCount());
	RenderText(Context->Store.GetFont(FontAssetDebugFont),
		Screen->GetRacerCount() == 0 ? "No runners in the database" :
		"Showing " + std::to_string(FirstVisible) + "-" + std::to_string(LastVisible) + " of " +
		std::to_string(Screen->GetRacerCount()),
		RacerDBScreenListLeft + 16, 691, RacerDBScreenMutedTextColor);
	DrawButton(*this, Context, Screen->ExitButton);

	Race::Racer* Runner = Screen->GetSelectedRacer();
	if (Runner == nullptr)
	{
		RenderText(Context->Store.GetFont(FontAssetInfoFont), "No racers are available.",
			RacerDBScreenDetailsLeft + 24, 132, RacerDBScreenTextColor);
	}
	else
	{
		RenderText(Context->Store.GetFont(FontAssetInfoFont), "SELECTED RACER",
			RacerDBScreenDetailsLeft + 24, 112, RacerDBScreenAccentColor);
		DrawButton(*this, Context, Screen->HistoryViewButton, Screen->IsShowingHistory(), true);
		RenderText(Context->Store.GetFont(FontAssetInfoFont), Runner->Name,
			RacerDBScreenDetailsLeft + 24, 147, RacerDBScreenTextColor);
		RenderText(Context->Store.GetFont(FontAssetDebugFont),
			"ID " + std::to_string(Runner->GUID.GUID),
			RacerDBScreenDetailsLeft + 24, 181, RacerDBScreenMutedTextColor);

		if (Screen->IsShowingHistory())
		{
			RenderText(Context->Store.GetFont(FontAssetDebugFont),
				"Rank #" + std::to_string(Screen->GetRankFor(Runner)) + " | " +
				FormatCompactCount(Runner->CareerRecord.PositionPoints) + " pts | " +
				std::to_string(Runner->CareerRecord.History.size()) + " recent results",
				RacerDBScreenDetailsLeft + 24, 198, RacerDBScreenMutedTextColor);
			RenderText(Context->Store.GetFont(FontAssetInfoFont), "RECENT RACES",
				RacerDBScreenDetailsLeft + 24, 230, RacerDBScreenAccentColor);

			const auto& History = Runner->CareerRecord.History;
			if (History.empty())
				RenderText(Context->Store.GetFont(FontAssetDebugFont), "No race history recorded yet.",
					RacerDBScreenDetailsLeft + 24, 260, RacerDBScreenMutedTextColor);
			else
			{
				const std::size_t VisibleCount = std::min<std::size_t>(History.size(), RacerDBScreenHistoryRows);
				for (std::size_t Index = 0; Index < VisibleCount; ++Index)
				{
					const Game::Race::RacerCareerHistoryEntry& Entry = History[History.size() - 1 - Index];
					const int RowY = 258 + static_cast<int>(Index) * RacerDBScreenHistoryRowStep;
					RenderText(Context->Store.GetFont(FontAssetDebugFont), FormatHistoryResult(Entry),
						RacerDBScreenDetailsLeft + 24, RowY, RacerDBScreenTextColor);
					RenderText(Context->Store.GetFont(FontAssetDebugFont),
						Entry.EventName.empty() ? "Career race" : FormatRacerName(Entry.EventName),
						RacerDBScreenDetailsLeft + 24, RowY + 18, RacerDBScreenMutedTextColor);
				}
			}
		}
		else
		{
			RenderText(Context->Store.GetFont(FontAssetDebugFont),
				"Rank #" + std::to_string(Screen->GetRankFor(Runner)) + " | " +
				FormatCompactCount(Runner->CareerRecord.PositionPoints) + " pts | XP " +
				FormatCompactCount(Runner->Skills.Xp),
				RacerDBScreenDetailsLeft + 24, 198, RacerDBScreenMutedTextColor);
			RenderText(Context->Store.GetFont(FontAssetDebugFont),
				"" + std::to_string(Runner->CareerRecord.Wins) + "W " +
				std::to_string(Runner->CareerRecord.Losses) + "L / " +
				std::to_string(Runner->CareerRecord.RacesEntered) + " races   Best " +
				FormatBestTime(Runner->CareerRecord.BestTimeMs),
				RacerDBScreenDetailsLeft + 24, 215, RacerDBScreenMutedTextColor);
			RenderText(Context->Store.GetFont(FontAssetInfoFont), "SCOUTING REPORT - ESTIMATES",
				RacerDBScreenDetailsLeft + 24, RacerDBScreenScoutingReportTop, RacerDBScreenAccentColor);

			const std::uint64_t TargetFingerprint = Game::Appraisal::HashRacer(*Runner);
			const auto Stats = GetRacerRatings(*Runner);
			for (std::size_t Index = 0; Index < Stats.size(); ++Index)
			{
				const double AppraisedRating = Screen->AppraiseRating(Stats[Index].BaseRating,
					TargetFingerprint, Index + 1);
				DrawRatingRow(*this, Context, Stats[Index].Label, FormatRating(AppraisedRating),
					AppraisedRating / RacerDBScreenPercentageScale, RacerDBScreenDetailsLeft + 24,
					RacerDBScreenSkillRowTop + static_cast<int>(Index) * RacerDBScreenSkillRowStep,
					RacerDBScreenDetailsWidth - 48);
				if (Screen->IsRatingRowHovered(Index))
					RenderText(Context->Store.GetFont(FontAssetDebugFont),
						std::string("DEBUG - TRUE BASE: ") + Stats[Index].Label + " = " +
						FormatBaseRating(Stats[Index].BaseRating),
						RacerDBScreenDetailsLeft + 24, 624, RacerDBScreenAccentColor);
			}

			RenderText(Context->Store.GetFont(FontAssetInfoFont), "PROFILE DETAILS",
				RacerDBScreenDetailsLeft + 24, 541, RacerDBScreenAccentColor);
			RenderText(Context->Store.GetFont(FontAssetDebugFont),
				"Age " + std::to_string(Runner->Details.Age) + "   Height " +
				std::to_string(static_cast<int>(std::lround(Runner->Details.HeightCm))) + " cm   Weight " +
				std::to_string(static_cast<int>(std::lround(Runner->Details.WeightKg))) + " kg",
				RacerDBScreenDetailsLeft + 24, 570, RacerDBScreenTextColor);
			RenderText(Context->Store.GetFont(FontAssetDebugFont),
				"Experience " + std::to_string(Runner->Skills.Xp),
				RacerDBScreenDetailsLeft + 24, 599, RacerDBScreenTextColor);
		}
	}

	Display();
	return static_cast<unsigned int>(SDL_GetTicks() - StartTime);
}
