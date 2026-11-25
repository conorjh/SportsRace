#include "RacerScreen.h"
#include "..\Audio.h"
#include "..\App\App.h"
#include "..\Render\RaceCurveVisualization.h"
#include "WorldMapScreen.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

namespace RacerScreenDetail
{
	constexpr int RacerScreenWidth = 1024;
	constexpr int RacerScreenHeight = 768;
	constexpr int RacerScreenMargin = 40;
	constexpr int RacerScreenColumnGap = 24;
	constexpr int RacerScreenPanelPadding = 18;
	constexpr float RacerScreenPanelAccentWidth = 4.0f;
	constexpr int RacerScreenPanelTop = 94;
	constexpr int RacerScreenAttributesPanelHeight = 350;
	constexpr int RacerScreenConditionsPanelTop = 458;
	constexpr int RacerScreenConditionsPanelHeight = 170;
	constexpr int RacerScreenProfilePanelHeight = 218;
	constexpr int RacerScreenDetailsPanelTop = 326;
	constexpr int RacerScreenDetailsPanelHeight = 164;
	constexpr int RacerScreenCurvesPanelTop = 496;
	constexpr int RacerScreenCurvesPanelHeight = 134;
	constexpr int RacerScreenCurvePlotTop = 526;
	constexpr int RacerScreenCurvePlotHeight = 54;
	constexpr int RacerScreenCurveTitleTop = 508;
	constexpr int RacerScreenCurveLegendTop = 589;
	constexpr int RacerScreenHeaderTop = 26;
	constexpr int RacerScreenSkillTitleTop = 112;
	constexpr int RacerScreenMentalHeadingTop = 143;
	constexpr int RacerScreenReflexTop = 164;
	constexpr int RacerScreenComposureTop = 203;
	constexpr int RacerScreenPhysicalHeadingTop = 246;
	constexpr int RacerScreenAccelerationTop = 267;
	constexpr int RacerScreenEnduranceTop = 306;
	constexpr int RacerScreenSpeedTop = 345;
	constexpr int RacerScreenConditionTitleTop = 476;
	constexpr int RacerScreenFatigueTop = 503;
	constexpr int RacerScreenConfidenceTop = 542;
	constexpr int RacerScreenInjuryTop = 581;
	constexpr int RacerScreenMaximumSkillRating = 100;
	constexpr double RacerScreenPercentageScale = 100.0;
	constexpr int RacerScreenBarLabelWidth = 108;
	constexpr int RacerScreenBarValueWidth = 42;
	constexpr int RacerScreenBarTopOffset = 18;
	constexpr int RacerScreenBarHeight = 7;
	constexpr int RacerScreenActionButtonWidth = 160;
	constexpr int RacerScreenActionButtonHeight = 48;
	constexpr int RacerScreenActionButtonGap = 18;
	constexpr int RacerScreenActionButtonRightMargin = 40;
	constexpr int RacerScreenActionButtonBottomMargin = 40;
	constexpr int RacerScreenActionButtonY =
		RacerScreenHeight - RacerScreenActionButtonBottomMargin - RacerScreenActionButtonHeight;
	constexpr int RacerScreenActionButtonX = RacerScreenWidth - RacerScreenActionButtonRightMargin -
		RacerScreenActionButtonWidth;
	constexpr int RacerScreenRegenButtonX = RacerScreenActionButtonX - RacerScreenActionButtonGap -
		RacerScreenActionButtonWidth;
	constexpr int RacerScreenButtonTextLeftPadding = 16;
	constexpr int RacerScreenButtonTextHeight = 16;
	constexpr int RacerScreenSpriteSheetWidth = 613;
	constexpr unsigned int RacerScreenSpriteFrameCount = 4;
	constexpr unsigned int RacerScreenSpriteFrameDurationMs = 250;
	constexpr int RacerScreenSpriteWidth = RacerScreenSpriteSheetWidth / RacerScreenSpriteFrameCount;
	constexpr int RacerScreenSpriteHeight = 186;
	constexpr int RacerScreenSpriteTop = 112;
	constexpr int RacerScreenPortraitTextTop = 112;
	constexpr int RacerScreenProfileNameTop = 145;
	constexpr int RacerScreenDetailTitleTop = 342;
	constexpr int RacerScreenDetailFirstRowTop = 370;
	constexpr int RacerScreenDetailSecondRowTop = 414;
	constexpr int RacerScreenDetailRecordTop = 449;
	constexpr int RacerScreenDetailRaceHistoryTop = 466;
	constexpr int RacerScreenDetailValueVerticalOffset = 17;
	constexpr SDL_Color RacerScreenBackgroundColor{ 23, 22, 20, 255 };
	constexpr SDL_Color RacerScreenPanelFillColor{ 34, 33, 30, 255 };
	constexpr SDL_Color RacerScreenPanelBorderColor{ 75, 70, 60, 255 };
	constexpr SDL_Color RacerScreenTextColor{ 235, 229, 215, 255 };
	constexpr SDL_Color RacerScreenMutedTextColor{ 164, 155, 138, 255 };
	constexpr SDL_Color RacerScreenAccentColor{ 178, 61, 49, 255 };
	constexpr SDL_Color RacerScreenBarBackgroundColor{ 60, 56, 49, 255 };
	constexpr SDL_Color RacerScreenMentalStatColor{ 190, 158, 99, 255 };
	constexpr SDL_Color RacerScreenPhysicalStatColor{ 177, 70, 57, 255 };
	constexpr SDL_Color RacerScreenFatigueColor{ 198, 145, 68, 255 };
	constexpr SDL_Color RacerScreenConfidenceColor{ 122, 145, 91, 255 };
	constexpr SDL_Color RacerScreenInjuryColor{ 172, 66, 57, 255 };
	constexpr SDL_Color RacerScreenButtonFillColor{ 104, 42, 36, 255 };
	constexpr SDL_Color RacerScreenButtonHoverFillColor{ 134, 54, 45, 255 };
	constexpr SDL_Color RacerScreenButtonBorderColor{ 170, 96, 72, 255 };
	constexpr SDL_Color RacerScreenButtonTextColor{ 239, 231, 214, 255 };
	constexpr Game::Render::RaceCurveVisualizationStyle RacerScreenCurveStyle{
		RacerScreenTextColor, RacerScreenBackgroundColor, RacerScreenPanelBorderColor, RacerScreenBarBackgroundColor,
		RacerScreenPhysicalStatColor, RacerScreenMentalStatColor
	};

	struct RacerScreenLayout
	{
		Game::GUI::RelativeLayout Coordinates;
		int Margin;
		int ColumnGap;
		int PanelWidth;
		int LeftColumnX;
		int RightColumnX;
	};

	RacerScreenLayout GetRacerScreenLayout(int Width, int Height)
	{
		const Game::GUI::RelativeLayout Coordinates(static_cast<float>(Width), static_cast<float>(Height));
		const int Margin = std::max(24, Coordinates.MapX(RacerScreenMargin));
		const int ColumnGap = std::max(18, Coordinates.MapX(RacerScreenColumnGap));
		const int PanelWidth = (Width - Margin * 2 - ColumnGap) / 2;
		const int LeftColumnX = Margin;
		return { Coordinates, Margin, ColumnGap, PanelWidth, LeftColumnX,
			LeftColumnX + PanelWidth + ColumnGap };
	}

	void DrawRacerScreenPanel(SDL_Renderer* Renderer, const SDL_FRect& Bounds, SDL_Color AccentColor)
	{
		SDL_SetRenderDrawColor(Renderer, RacerScreenPanelFillColor.r, RacerScreenPanelFillColor.g,
			RacerScreenPanelFillColor.b, RacerScreenPanelFillColor.a);
		SDL_RenderFillRect(Renderer, &Bounds);
		SDL_SetRenderDrawColor(Renderer, RacerScreenPanelBorderColor.r, RacerScreenPanelBorderColor.g,
			RacerScreenPanelBorderColor.b, RacerScreenPanelBorderColor.a);
		SDL_RenderRect(Renderer, &Bounds);

		const SDL_FRect AccentBounds{ Bounds.x, Bounds.y, RacerScreenPanelAccentWidth, Bounds.h };
		SDL_SetRenderDrawColor(Renderer, AccentColor.r, AccentColor.g, AccentColor.b, AccentColor.a);
		SDL_RenderFillRect(Renderer, &AccentBounds);
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

	void DrawRacerScreenBar(Game::Render::BaseRenderer& Renderer, Game::Render::AppRenderContext* Context,
		const std::string& Label, const std::string& Value, double NormalizedValue, int X, int Y, int Width,
		SDL_Color FillColor)
	{
		using namespace Game::Assets;

		const int BarWidth = Width - RacerScreenBarLabelWidth - RacerScreenBarValueWidth;
		const int BarX = X + RacerScreenBarLabelWidth;
		const int BarY = Y + RacerScreenBarTopOffset;
		Renderer.RenderText(Context->Store.GetFont(FontAssetInfoFont), Label, X, Y, RacerScreenTextColor);
		Renderer.RenderText(Context->Store.GetFont(FontAssetInfoFont), Value,
			X + Width - RacerScreenBarValueWidth, Y, RacerScreenTextColor);

		const SDL_FRect BarBounds{ static_cast<float>(BarX), static_cast<float>(BarY),
			static_cast<float>(BarWidth), static_cast<float>(RacerScreenBarHeight) };
		SDL_SetRenderDrawColor(Context->MainRenderer, RacerScreenBarBackgroundColor.r, RacerScreenBarBackgroundColor.g,
			RacerScreenBarBackgroundColor.b, RacerScreenBarBackgroundColor.a);
		SDL_RenderFillRect(Context->MainRenderer, &BarBounds);

		const double Fraction = std::clamp(NormalizedValue, 0.0, 1.0);
		if (Fraction > 0.0)
		{
			const SDL_FRect FillBounds{ BarBounds.x, BarBounds.y,
				static_cast<float>(BarWidth * Fraction), BarBounds.h };
			SDL_SetRenderDrawColor(Context->MainRenderer, FillColor.r, FillColor.g, FillColor.b, FillColor.a);
			SDL_RenderFillRect(Context->MainRenderer, &FillBounds);
		}
	}

	std::string FormatRating(double Rating)
	{
		return std::to_string(static_cast<int>(std::lround(std::clamp(Rating, 0.0,
			static_cast<double>(RacerScreenMaximumSkillRating)))));
	}

	std::string FormatPercentage(double Value)
	{
		return std::to_string(static_cast<int>(std::lround(std::clamp(Value, 0.0, 1.0) *
			RacerScreenPercentageScale))) + "%";
	}

	void DrawRacerScreenDetail(Game::Render::BaseRenderer& Renderer, Game::Render::AppRenderContext* Context,
		const std::string& Label, const std::string& Value, int X, int Y)
	{
		using namespace Game::Assets;

		Renderer.RenderText(Context->Store.GetFont(FontAssetDebugFont), Label, X, Y, RacerScreenMutedTextColor);
		Renderer.RenderText(Context->Store.GetFont(FontAssetInfoFont), Value,
			X, Y + RacerScreenDetailValueVerticalOffset, RacerScreenTextColor);
	}

	void DrawRacerScreenButton(Game::Render::BaseRenderer& Renderer, Game::Render::AppRenderContext* Context,
		Game::GUI::Button& Button, bool Enabled = true)
	{
		SDL_FRect ButtonBounds;
		SDL_RectToFRect(&Button.Rect, &ButtonBounds);
		const bool IsHovered = Enabled && Button.IsMouseOver();
		const SDL_Color FillColor = !Enabled ? SDL_Color{ 63, 61, 55, 255 } :
			IsHovered ? RacerScreenButtonHoverFillColor : RacerScreenButtonFillColor;

		SDL_SetRenderDrawColor(Context->MainRenderer, FillColor.r, FillColor.g, FillColor.b, FillColor.a);
		SDL_RenderFillRect(Context->MainRenderer, &ButtonBounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, RacerScreenButtonBorderColor.r, RacerScreenButtonBorderColor.g,
			RacerScreenButtonBorderColor.b, RacerScreenButtonBorderColor.a);
		SDL_RenderRect(Context->MainRenderer, &ButtonBounds);

		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), Button.Text,
			Button.x + RacerScreenButtonTextLeftPadding,
			Button.y + (Button.h - RacerScreenButtonTextHeight) / 2,
			Enabled ? RacerScreenButtonTextColor : RacerScreenMutedTextColor);
	}
}
using namespace RacerScreenDetail;

Game::Screens::RacerScreen::RacerScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* _RacerToDisplay, RacerScreenStateInitType _InitType) :
	AppScreen(_Machine, _IO, _Data),
	RegenButton(_IO, "Regenerate", RacerScreenRegenButtonX, RacerScreenActionButtonY,
		RacerScreenActionButtonWidth, RacerScreenActionButtonHeight),
	ApplyButton(_IO, "Apply", RacerScreenActionButtonX, RacerScreenActionButtonY,
		RacerScreenActionButtonWidth, RacerScreenActionButtonHeight),
	ExitButton(_IO, "Back", RacerScreenActionButtonX, RacerScreenActionButtonY,
		RacerScreenActionButtonWidth, RacerScreenActionButtonHeight),
	LocationButton(_IO, "Locate", RacerScreenRegenButtonX, RacerScreenActionButtonY,
		RacerScreenActionButtonWidth, RacerScreenActionButtonHeight),
	InitType(_InitType),
	RacerToDisplay(_RacerToDisplay)
{
	Type = AppScreenType::RacerScreen;
	UpdateLayout();
	if (InitType == RacerScreenStateInitType::RacerCreation)
		Data.Profile->MainFella.Name = Race::RacerNameMaker().Make();
}

Game::Screens::RacerScreen::~RacerScreen()
{
}

const Game::Career::CareerRacerState* Game::Screens::RacerScreen::GetCareerRacerState() const
{
	if (Data.Career == nullptr || RacerToDisplay == nullptr)
		return nullptr;
	const auto State = std::find_if(Data.Career->RacerStates.begin(), Data.Career->RacerStates.end(),
		[this](const Game::Career::CareerRacerState& Candidate)
		{
			return Candidate.Runner == RacerToDisplay;
		});
	return State == Data.Career->RacerStates.end() ? nullptr : &*State;
}

std::uint64_t Game::Screens::RacerScreen::GetBasedInCityId() const
{
	if (Data.Career == nullptr)
		return 0;
	if (const Game::Career::CareerRacerState* State = GetCareerRacerState())
		return State->BasedInCityId;
	if (IsPlayerOwnedRacer(RacerToDisplay))
		return Data.Career->PlayerStableCityId != 0 ? Data.Career->PlayerStableCityId : Data.Career->HomeCityId;
	return 0;
}

std::string Game::Screens::RacerScreen::GetBasedInCityName() const
{
	if (Data.Career == nullptr)
		return "Not in a career";
	const std::uint64_t CityId = GetBasedInCityId();
	const auto City = std::find_if(Data.Career->Cities.begin(), Data.Career->Cities.end(), [CityId](
		const Game::Career::CareerWorldCity& Candidate)
		{
			return Candidate.Id == CityId;
		});
	if (City == Data.Career->Cities.end())
		return "Unknown city";
	const auto Region = std::find_if(Data.Career->Regions.begin(), Data.Career->Regions.end(), [City](
		const Game::Career::CareerWorldRegion& Candidate)
		{
			return Candidate.Id == City->RegionId;
		});
	return Region == Data.Career->Regions.end() ? City->Name : City->Name + " / " + Region->Name;
}

std::string Game::Screens::RacerScreen::GetCareerStatusLabel() const
{
	if (Data.Career == nullptr)
		return "Not in a career";
	const Game::Career::CareerRacerState* State = GetCareerRacerState();
	std::uint64_t LeagueId = State == nullptr ? 0 : State->LeagueId;
	if (LeagueId == 0 && Data.Career->PlayerLeagueRacer == RacerToDisplay)
		LeagueId = Data.Career->PlayerLeagueId;
	const auto League = std::find_if(Data.Career->Leagues.begin(), Data.Career->Leagues.end(), [LeagueId](
		const Game::Career::CareerLeague& Candidate)
		{
			return LeagueId != 0 && Candidate.Id == LeagueId;
		});
	if (League != Data.Career->Leagues.end() && !League->IsCup && !League->IsAmateurCup &&
		League->Division != Game::Career::CareerLeagueDivision::Amateur)
		return "PRO - " + Game::Career::CareerCompetitionLevelToString(League->Level) + " " +
			Game::Career::CareerLeagueDivisionToString(League->Division);
	if (State != nullptr && State->Activity == Game::Career::CareerRacerActivity::Retired)
		return "Retired";
	return State != nullptr && State->HasRacingLicense ? "Licensed amateur" : "Amateur";
}

Game::Screens::AppScreen* Game::Screens::RacerScreen::Update()
{
	using namespace Game::Race;
	using namespace Game::Audio;

	UpdateLayout();
	const int Width = Data.RenderContext == nullptr ? RacerScreenWidth :
		static_cast<int>(Data.RenderContext->ScreenWidth);
	const int Height = Data.RenderContext == nullptr ? RacerScreenHeight :
		static_cast<int>(Data.RenderContext->ScreenHeight);
	const RacerScreenLayout Layout = GetRacerScreenLayout(Width, Height);
	if (InitType == RacerScreenStateInitType::RacerCreation)
	{
		RegenButton.Update();
		if (RegenButton.HasMouseClicked())
		{
			IO.Player.Play(BuiltInSounds::Click);

			Racer Generated(RacerNameMaker().Make());
			Generated.GUID = Data.Profile->MainFella.GUID;
			Data.Profile->MainFella = Generated;
		}
		ApplyButton.Update();
	}
	else
	{
		if (GetBasedInCityId() != 0)
			LocationButton.Update();
		ExitButton.Update();
		if (LocationButton.HasMouseClicked() && Data.Career != nullptr && RacerToDisplay != nullptr)
		{
			const Career::CareerRacerState* State = GetCareerRacerState();
			std::uint64_t TargetCityId = GetBasedInCityId();
			int TargetPlotIndex = -1;
			const auto GetPlotsForCity = [this](std::uint64_t CityId) ->
				const std::vector<Career::CityPlot>*
			{
				if (Data.Career == nullptr)
					return nullptr;
				if (CityId == Data.Career->HomeCityId)
					return &Data.Career->CityPlots;
				const auto City = std::find_if(Data.Career->Cities.begin(), Data.Career->Cities.end(),
					[CityId](const Career::CareerWorldCity& Candidate) { return Candidate.Id == CityId; });
				return City == Data.Career->Cities.end() ? nullptr : &City->Plots;
			};
			const auto FindEquivalentPlot = [this, &GetPlotsForCity](std::uint64_t CityId,
				std::size_t SourcePlotIndex, const Career::CityPlot& SourcePlot)
			{
				if (Data.Career != nullptr && CityId == Data.Career->HomeCityId &&
					SourcePlotIndex < Data.Career->CityPlots.size())
					return static_cast<int>(SourcePlotIndex);
				const std::vector<Career::CityPlot>* Plots = GetPlotsForCity(CityId);
				if (Plots == nullptr)
					return -1;
			const auto IsMatchingPlot = [&SourcePlot](const Career::CityPlot& Candidate)
			{
				return Candidate.Type == SourcePlot.Type &&
					(Candidate.Type != Career::CityPlotType::RaceTrack ||
					Candidate.TrackLength == SourcePlot.TrackLength);
			};
			std::size_t MatchingOrdinal = static_cast<std::size_t>(std::count_if(
				Data.Career->CityPlots.begin(), Data.Career->CityPlots.begin() + SourcePlotIndex, IsMatchingPlot));
			for (std::size_t Index = 0; Index < Plots->size(); ++Index)
				if (IsMatchingPlot((*Plots)[Index]))
				{
					if (MatchingOrdinal == 0)
						return static_cast<int>(Index);
					--MatchingOrdinal;
				}
			return -1;
			};

			if (State != nullptr && (State->Activity == Career::CareerRacerActivity::InBuilding ||
				State->Activity == Career::CareerRacerActivity::Training ||
				State->Activity == Career::CareerRacerActivity::Travelling ||
				State->Activity == Career::CareerRacerActivity::InRace))
			{
				const std::size_t SourcePlotIndex = State->Activity == Career::CareerRacerActivity::Travelling
					? State->DestinationPlotIndex : State->LocationPlotIndex;
				if (SourcePlotIndex < Data.Career->CityPlots.size())
				{
					const Career::CityPlot& SourcePlot = Data.Career->CityPlots[SourcePlotIndex];
					if (SourcePlot.Type != Career::CityPlotType::Scenery &&
						SourcePlot.Type != Career::CityPlotType::VacantLot)
					{
						if (State->Activity == Career::CareerRacerActivity::InRace)
							TargetCityId = Data.Career->HomeCityId;
						TargetPlotIndex = FindEquivalentPlot(TargetCityId, SourcePlotIndex, SourcePlot);
					}
				}
			}
			else if (State == nullptr && Data.Career->PlayerLeagueRacer == RacerToDisplay &&
				(Data.Career->PendingPlayerLeagueRace.Active || Data.Career->PendingPlayerLeagueRace.Preparing))
			{
				TargetCityId = Data.Career->HomeCityId;
				if (Data.Career->PendingPlayerLeagueRace.TrackPlotIndex < Data.Career->CityPlots.size())
					TargetPlotIndex = static_cast<int>(Data.Career->PendingPlayerLeagueRace.TrackPlotIndex);
			}

			if (TargetCityId != 0)
			{
				auto* MapScreen = new WorldMapScreen(ScreenStack, IO, Data);
				ScreenStack.Push(MapScreen);
				MapScreen->FocusOnLocation(TargetCityId, TargetPlotIndex);
				return ScreenStack.Top();
			}
		}
	}
	if (InitType == RacerScreenStateInitType::ViewOnly && RacerToDisplay != nullptr &&
		(IO.MouseButtonsPressed & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) != 0 &&
		IO.MousePosition.x >= Layout.RightColumnX + RacerScreenPanelPadding &&
		IO.MousePosition.x < Layout.RightColumnX + Layout.PanelWidth - RacerScreenPanelPadding &&
		IO.MousePosition.y >= Layout.Coordinates.MapY(RacerScreenDetailRaceHistoryTop) &&
		IO.MousePosition.y < Layout.Coordinates.MapY(RacerScreenDetailsPanelTop +
			RacerScreenDetailsPanelHeight))
	{
		const Game::Race::RacerCareerHistoryEntry* LastRace = RacerToDisplay->CareerRecord.History.empty()
			? nullptr : &RacerToDisplay->CareerRecord.History.back();
		if (LastRace != nullptr && LastRace->HistoricalRaceId != 0 &&
			OpenCareerHistoricalRace(ScreenStack, IO, Data, LastRace->HistoricalRaceId,
				Data.NodeViewerEnabled && IO.ShiftClickPressed))
			return ScreenStack.Top();
	}
	if ((InitType == RacerScreenStateInitType::RacerCreation && ApplyButton.HasMouseClicked()) ||
		(InitType == RacerScreenStateInitType::ViewOnly && ExitButton.HasMouseClicked()) || IO.Esc)
	{
		IO.Player.Play(BuiltInSounds::Click);

		//go back to last screen
		ScreenStack.Pop();
		return ScreenStack.Top();
	}

	return this;
}

void Game::Screens::RacerScreen::UpdateLayout()
{
	const int Width = Data.RenderContext == nullptr ? RacerScreenWidth :
		static_cast<int>(Data.RenderContext->ScreenWidth);
	const int Height = Data.RenderContext == nullptr ? RacerScreenHeight :
		static_cast<int>(Data.RenderContext->ScreenHeight);
	const RacerScreenLayout Layout = GetRacerScreenLayout(Width, Height);
	const int ButtonY = Height - std::max(24, Layout.Coordinates.MapY(RacerScreenActionButtonBottomMargin)) -
		RacerScreenActionButtonHeight;
	const int ApplyButtonX = Width - Layout.Margin - RacerScreenActionButtonWidth;
	RegenButton.SetBounds(ApplyButtonX - RacerScreenActionButtonGap - RacerScreenActionButtonWidth,
		ButtonY, RacerScreenActionButtonWidth, RacerScreenActionButtonHeight);
	LocationButton.SetBounds(ApplyButtonX - RacerScreenActionButtonGap - RacerScreenActionButtonWidth,
		ButtonY, RacerScreenActionButtonWidth, RacerScreenActionButtonHeight);
	ApplyButton.SetBounds(ApplyButtonX, ButtonY, RacerScreenActionButtonWidth, RacerScreenActionButtonHeight);
	ExitButton.SetBounds(ApplyButtonX, ButtonY, RacerScreenActionButtonWidth, RacerScreenActionButtonHeight);
}


Game::Render::RacerScreenRenderer::RacerScreenRenderer(Game::Render::AppRenderContext* Context, Game::Career::CareerProfile* RacerProfile)
	: BaseRenderer(Context),
	RacerProfile(RacerProfile),
	Screen()
{

}

unsigned int Game::Render::RacerScreenRenderer::Render()
{
	// Keep hidden ratings out of this player-facing profile while showing visible skills and conditions.
	using namespace Game::Assets;
	const auto StartTime = SDL_GetTicks();
	const int ScreenWidth = static_cast<int>(Context->ScreenWidth);
	const int ScreenHeight = static_cast<int>(Context->ScreenHeight);
	const RacerScreenLayout Layout = GetRacerScreenLayout(ScreenWidth, ScreenHeight);
	const Game::Race::Racer* DisplayedRunner = Screen == nullptr ? nullptr : Screen->GetRacerToDisplay();
	const Game::Race::Racer& Runner = DisplayedRunner == nullptr ? RacerProfile->MainFella : *DisplayedRunner;
	const bool IsPlayerRacer = &Runner == &RacerProfile->MainFella;

	SDL_SetRenderDrawColor(Context->MainRenderer, RacerScreenBackgroundColor.r, RacerScreenBackgroundColor.g,
		RacerScreenBackgroundColor.b, RacerScreenBackgroundColor.a);
	SDL_RenderClear(Context->MainRenderer);

	RenderText(Context->Store.GetFont(FontAssetMainFont), "RACER PROFILE",
		Layout.Margin, Layout.Coordinates.MapY(RacerScreenHeaderTop), RacerScreenAccentColor);

	const int PanelTop = Layout.Coordinates.MapY(RacerScreenPanelTop);
	const SDL_FRect AttributesPanel{ static_cast<float>(Layout.LeftColumnX), static_cast<float>(PanelTop),
		static_cast<float>(Layout.PanelWidth),
		static_cast<float>(Layout.Coordinates.MapY(RacerScreenPanelTop + RacerScreenAttributesPanelHeight) - PanelTop) };
	DrawRacerScreenPanel(Context->MainRenderer, AttributesPanel, RacerScreenMentalStatColor);
	RenderText(Context->Store.GetFont(FontAssetInfoFont), "SKILLS (0-100)",
		Layout.LeftColumnX + RacerScreenPanelPadding, Layout.Coordinates.MapY(RacerScreenSkillTitleTop),
		RacerScreenAccentColor);
	RenderText(Context->Store.GetFont(FontAssetDebugFont), "MENTAL",
		Layout.LeftColumnX + RacerScreenPanelPadding, Layout.Coordinates.MapY(RacerScreenMentalHeadingTop),
		RacerScreenMutedTextColor);

	const int SkillLeft = Layout.LeftColumnX + RacerScreenPanelPadding;
	const int SkillWidth = Layout.PanelWidth - RacerScreenPanelPadding * 2;
	DrawRacerScreenBar(*this, Context, "Reflex", FormatRating(Runner.Skills.Reflex),
		Runner.Skills.Reflex / RacerScreenMaximumSkillRating, SkillLeft,
		Layout.Coordinates.MapY(RacerScreenReflexTop),
		SkillWidth, RacerScreenMentalStatColor);
	DrawRacerScreenBar(*this, Context, "Composure", FormatRating(Runner.Skills.Composure),
		Runner.Skills.Composure / RacerScreenMaximumSkillRating, SkillLeft,
		Layout.Coordinates.MapY(RacerScreenComposureTop),
		SkillWidth, RacerScreenMentalStatColor);
	RenderText(Context->Store.GetFont(FontAssetDebugFont), "PHYSICAL",
		Layout.LeftColumnX + RacerScreenPanelPadding, Layout.Coordinates.MapY(RacerScreenPhysicalHeadingTop),
		RacerScreenMutedTextColor);
	DrawRacerScreenBar(*this, Context, "Acceleration", FormatRating(Runner.Skills.Acceleration),
		Runner.Skills.Acceleration / RacerScreenMaximumSkillRating, SkillLeft,
		Layout.Coordinates.MapY(RacerScreenAccelerationTop),
		SkillWidth, RacerScreenPhysicalStatColor);
	DrawRacerScreenBar(*this, Context, "Endurance", FormatRating(Runner.Skills.Endurance),
		Runner.Skills.Endurance / RacerScreenMaximumSkillRating, SkillLeft,
		Layout.Coordinates.MapY(RacerScreenEnduranceTop),
		SkillWidth, RacerScreenPhysicalStatColor);
	DrawRacerScreenBar(*this, Context, "Speed", FormatRating(Runner.Skills.Speed),
		Runner.Skills.Speed / RacerScreenMaximumSkillRating, SkillLeft,
		Layout.Coordinates.MapY(RacerScreenSpeedTop),
		SkillWidth, RacerScreenPhysicalStatColor);

	const int ConditionsTop = Layout.Coordinates.MapY(RacerScreenConditionsPanelTop);
	const SDL_FRect ConditionsPanel{ static_cast<float>(Layout.LeftColumnX), static_cast<float>(ConditionsTop),
		static_cast<float>(Layout.PanelWidth), static_cast<float>(Layout.Coordinates.MapY(
			RacerScreenConditionsPanelTop + RacerScreenConditionsPanelHeight) - ConditionsTop) };
	DrawRacerScreenPanel(Context->MainRenderer, ConditionsPanel, RacerScreenConfidenceColor);
	RenderText(Context->Store.GetFont(FontAssetInfoFont), "CONDITIONS",
		Layout.LeftColumnX + RacerScreenPanelPadding, Layout.Coordinates.MapY(RacerScreenConditionTitleTop),
		RacerScreenAccentColor);
	const int ConditionLeft = Layout.LeftColumnX + RacerScreenPanelPadding;
	const int ConditionWidth = Layout.PanelWidth - RacerScreenPanelPadding * 2;
	DrawRacerScreenBar(*this, Context, "Fatigue", FormatPercentage(Runner.Conditions.Fatigue),
		Runner.Conditions.Fatigue, ConditionLeft, Layout.Coordinates.MapY(RacerScreenFatigueTop),
		ConditionWidth, RacerScreenFatigueColor);
	DrawRacerScreenBar(*this, Context, "Confidence", FormatPercentage(Runner.Conditions.Confidence),
		Runner.Conditions.Confidence, ConditionLeft, Layout.Coordinates.MapY(RacerScreenConfidenceTop),
		ConditionWidth, RacerScreenConfidenceColor);
	DrawRacerScreenBar(*this, Context, "Injury", FormatPercentage(Runner.Conditions.Injury),
		Runner.Conditions.Injury, ConditionLeft, Layout.Coordinates.MapY(RacerScreenInjuryTop),
		ConditionWidth, RacerScreenInjuryColor);

	const SDL_FRect ProfilePanel{ static_cast<float>(Layout.RightColumnX), static_cast<float>(PanelTop),
		static_cast<float>(Layout.PanelWidth), static_cast<float>(Layout.Coordinates.MapY(
			RacerScreenPanelTop + RacerScreenProfilePanelHeight) - PanelTop) };
	DrawRacerScreenPanel(Context->MainRenderer, ProfilePanel, RacerScreenAccentColor);
	RenderText(Context->Store.GetFont(FontAssetInfoFont), "PROFILE",
		Layout.RightColumnX + RacerScreenPanelPadding, Layout.Coordinates.MapY(RacerScreenPortraitTextTop),
		RacerScreenAccentColor);
	RenderText(Context->Store.GetFont(FontAssetWinningFont), Runner.Name,
		Layout.RightColumnX + RacerScreenPanelPadding, Layout.Coordinates.MapY(RacerScreenProfileNameTop),
		Game::Render::PlayerOwnedRacerHighlightColor);
	RenderText(Context->Store.GetFont(FontAssetDebugFont), "BASED IN",
		Layout.RightColumnX + RacerScreenPanelPadding, Layout.Coordinates.MapY(184),
		RacerScreenMutedTextColor);
	RenderText(Context->Store.GetFont(FontAssetInfoFont), Screen->GetBasedInCityName(),
		Layout.RightColumnX + RacerScreenPanelPadding, Layout.Coordinates.MapY(201),
		RacerScreenTextColor);
	RenderText(Context->Store.GetFont(FontAssetDebugFont), Screen->GetCareerStatusLabel(),
		Layout.RightColumnX + RacerScreenPanelPadding, Layout.Coordinates.MapY(224),
		RacerScreenMutedTextColor);

	const unsigned int AnimationFrame = static_cast<unsigned int>((SDL_GetTicks() / RacerScreenSpriteFrameDurationMs) %
		RacerScreenSpriteFrameCount);
	SDL_Rect SpriteSource{ static_cast<int>(AnimationFrame) * RacerScreenSpriteWidth, 0,
		RacerScreenSpriteWidth, RacerScreenSpriteHeight };
	SDL_Rect SpriteBounds{ Layout.RightColumnX + Layout.PanelWidth - RacerScreenPanelPadding -
		RacerScreenSpriteWidth, Layout.Coordinates.MapY(RacerScreenSpriteTop),
		RacerScreenSpriteWidth, RacerScreenSpriteHeight };
	RenderImage(Context->Store.GetImage(ImageAssetFellaRun)->Texture, &SpriteSource, &SpriteBounds);

	const int DetailsTop = Layout.Coordinates.MapY(RacerScreenDetailsPanelTop);
	const SDL_FRect DetailsPanel{ static_cast<float>(Layout.RightColumnX), static_cast<float>(DetailsTop),
		static_cast<float>(Layout.PanelWidth), static_cast<float>(Layout.Coordinates.MapY(
			RacerScreenDetailsPanelTop + RacerScreenDetailsPanelHeight) - DetailsTop) };
	DrawRacerScreenPanel(Context->MainRenderer, DetailsPanel, RacerScreenPhysicalStatColor);
	RenderText(Context->Store.GetFont(FontAssetInfoFont), "PERSONAL DETAILS",
		Layout.RightColumnX + RacerScreenPanelPadding, Layout.Coordinates.MapY(RacerScreenDetailTitleTop),
		RacerScreenAccentColor);
	DrawRacerScreenDetail(*this, Context, "AGE", std::to_string(Runner.Details.Age),
		Layout.RightColumnX + RacerScreenPanelPadding, Layout.Coordinates.MapY(RacerScreenDetailFirstRowTop));
	DrawRacerScreenDetail(*this, Context, "HEIGHT", std::to_string(static_cast<int>(std::lround(Runner.Details.HeightCm))) + " cm",
		Layout.RightColumnX + Layout.PanelWidth / 3, Layout.Coordinates.MapY(RacerScreenDetailFirstRowTop));
	DrawRacerScreenDetail(*this, Context, "WEIGHT", std::to_string(static_cast<int>(std::lround(Runner.Details.WeightKg))) + " kg",
		Layout.RightColumnX + Layout.PanelWidth * 2 / 3, Layout.Coordinates.MapY(RacerScreenDetailFirstRowTop));
	const std::string CareerCash = IsPlayerRacer
		? "$" + std::to_string(RacerProfile->Financials.Cash) : "N/A";
	DrawRacerScreenDetail(*this, Context, "CAREER CASH", CareerCash,
		Layout.RightColumnX + RacerScreenPanelPadding, Layout.Coordinates.MapY(RacerScreenDetailSecondRowTop));
	RenderText(Context->Store.GetFont(FontAssetDebugFont),
		"Career " + std::to_string(Runner.CareerRecord.Wins) + "W / " +
		std::to_string(Runner.CareerRecord.Losses) + "L   Best " +
		FormatBestTime(Runner.CareerRecord.BestTimeMs),
		Layout.RightColumnX + RacerScreenPanelPadding, Layout.Coordinates.MapY(RacerScreenDetailRecordTop),
		RacerScreenMutedTextColor);
	if (Runner.CareerRecord.History.empty())
		RenderText(Context->Store.GetFont(FontAssetDebugFont), "No race history yet",
			Layout.RightColumnX + RacerScreenPanelPadding,
			Layout.Coordinates.MapY(RacerScreenDetailRaceHistoryTop),
			RacerScreenMutedTextColor);
	else
	{
		constexpr std::uint64_t MillisecondsPerWorldDay = 86400000;
		const Game::Race::RacerCareerHistoryEntry& LastRace = Runner.CareerRecord.History.back();
		const std::string Place = LastRace.Position == Game::Race::PulledUpPosition
			? "DNF" : "#" + std::to_string(LastRace.Position);
		const std::string Distance = LastRace.TrackLength == 0 ? "?m" :
			std::to_string(LastRace.TrackLength * 100 /
				static_cast<unsigned int>(Game::Race::TrackLength::Standard100m)) + "m";
		RenderText(Context->Store.GetFont(FontAssetDebugFont),
			"Last: Day " + std::to_string(LastRace.WorldTimeMs / MillisecondsPerWorldDay + 1) + " " +
			Place + " " + Distance + "  (click for results)",
			Layout.RightColumnX + RacerScreenPanelPadding,
			Layout.Coordinates.MapY(RacerScreenDetailRaceHistoryTop),
			RacerScreenMutedTextColor);
	}

	const int CurvesTop = Layout.Coordinates.MapY(RacerScreenCurvesPanelTop);
	const SDL_FRect CurvesPanel{ static_cast<float>(Layout.RightColumnX), static_cast<float>(CurvesTop),
		static_cast<float>(Layout.PanelWidth), static_cast<float>(Layout.Coordinates.MapY(
			RacerScreenCurvesPanelTop + RacerScreenCurvesPanelHeight) - CurvesTop) };
	DrawRacerScreenPanel(Context->MainRenderer, CurvesPanel, RacerScreenPhysicalStatColor);
	const int CurvePlotTop = Layout.Coordinates.MapY(RacerScreenCurvePlotTop);
	const SDL_FRect CurvePlotBounds{ static_cast<float>(Layout.RightColumnX + RacerScreenPanelPadding),
		static_cast<float>(CurvePlotTop),
		static_cast<float>(Layout.PanelWidth - RacerScreenPanelPadding * 2),
		static_cast<float>(Layout.Coordinates.MapY(RacerScreenCurvePlotTop + RacerScreenCurvePlotHeight) -
			CurvePlotTop) };
	// Show the generated, normalized acceleration and endurance curves alongside the raw skill ratings.
	DrawRacerCurveVisualization(*this, Context, Runner, CurvePlotBounds,
		Layout.RightColumnX + RacerScreenPanelPadding, Layout.Coordinates.MapY(RacerScreenCurveTitleTop),
		Layout.Coordinates.MapY(RacerScreenCurveLegendTop),
		RacerScreenCurveStyle);

	if (Screen->InitType == Screens::RacerScreenStateInitType::RacerCreation)
	{
		DrawRacerScreenButton(*this, Context, Screen->RegenButton);
		DrawRacerScreenButton(*this, Context, Screen->ApplyButton);
	}
	else
	{
		DrawRacerScreenButton(*this, Context, Screen->LocationButton,
			Screen->GetBasedInCityId() != 0);
		DrawRacerScreenButton(*this, Context, Screen->ExitButton);
	}

	Display();
	const auto EndTime = SDL_GetTicks();
	return static_cast<unsigned int>(EndTime - StartTime);
}
