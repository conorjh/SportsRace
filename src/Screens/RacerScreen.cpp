#include "RacerScreen.h"
#include "..\Audio.h"
#include "..\App\App.h"
#include "..\Render\RaceCurveVisualization.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace
{
	constexpr int RacerScreenWidth = 1024;
	constexpr int RacerScreenHeight = 768;
	constexpr int RacerScreenMargin = 40;
	constexpr int RacerScreenColumnGap = 24;
	constexpr int RacerScreenPanelPadding = 18;
	constexpr float RacerScreenPanelAccentWidth = 4.0f;
	constexpr int RacerScreenPanelTop = 94;
	constexpr int RacerScreenPanelWidth =
		(RacerScreenWidth - RacerScreenMargin * 2 - RacerScreenColumnGap) / 2;
	constexpr int RacerScreenLeftColumnX = RacerScreenMargin;
	constexpr int RacerScreenRightColumnX = RacerScreenLeftColumnX + RacerScreenPanelWidth + RacerScreenColumnGap;
	constexpr int RacerScreenAttributesPanelHeight = 350;
	constexpr int RacerScreenConditionsPanelTop = 458;
	constexpr int RacerScreenConditionsPanelHeight = 170;
	constexpr int RacerScreenProfilePanelHeight = 218;
	constexpr int RacerScreenDetailsPanelTop = 326;
	constexpr int RacerScreenDetailsPanelHeight = 134;
	constexpr int RacerScreenCurvesPanelTop = 476;
	constexpr int RacerScreenCurvesPanelHeight = 154;
	constexpr int RacerScreenCurvePlotTop = 506;
	constexpr int RacerScreenCurvePlotHeight = 72;
	constexpr int RacerScreenCurveTitleTop = 488;
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
	constexpr int RacerScreenDetailSecondColumnOffset = 155;
	constexpr int RacerScreenDetailThirdColumnOffset = 310;
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
		Game::GUI::Button& Button)
	{
		SDL_FRect ButtonBounds;
		SDL_RectToFRect(&Button.Rect, &ButtonBounds);
		const bool IsHovered = Button.IsMouseOver();
		const SDL_Color FillColor = IsHovered ? RacerScreenButtonHoverFillColor : RacerScreenButtonFillColor;

		SDL_SetRenderDrawColor(Context->MainRenderer, FillColor.r, FillColor.g, FillColor.b, FillColor.a);
		SDL_RenderFillRect(Context->MainRenderer, &ButtonBounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, RacerScreenButtonBorderColor.r, RacerScreenButtonBorderColor.g,
			RacerScreenButtonBorderColor.b, RacerScreenButtonBorderColor.a);
		SDL_RenderRect(Context->MainRenderer, &ButtonBounds);

		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), Button.Text,
			Button.x + RacerScreenButtonTextLeftPadding,
			Button.y + (Button.h - RacerScreenButtonTextHeight) / 2, RacerScreenButtonTextColor);
	}
}

Game::Screens::RacerScreen::RacerScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* _RacerToDisplay, RacerScreenStateInitType _InitType) :
	AppScreen(_Machine, _IO, _Data),
	RegenButton(_IO, "Regenerate", RacerScreenRegenButtonX, RacerScreenActionButtonY,
		RacerScreenActionButtonWidth, RacerScreenActionButtonHeight),
	ApplyButton(_IO, "Apply", RacerScreenActionButtonX, RacerScreenActionButtonY,
		RacerScreenActionButtonWidth, RacerScreenActionButtonHeight),
	ExitButton(_IO, "Back", RacerScreenActionButtonX, RacerScreenActionButtonY,
		RacerScreenActionButtonWidth, RacerScreenActionButtonHeight),
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
		// Preserve the racer's identity while replacing the generated profile and curves.
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
	// Keep hidden ratings out of this player-facing profile while showing visible skills and conditions.
	using namespace Game::Assets;
	const auto StartTime = SDL_GetTicks();
	const Game::Race::Racer& Runner = RacerProfile->MainFella;

	SDL_SetRenderDrawColor(Context->MainRenderer, RacerScreenBackgroundColor.r, RacerScreenBackgroundColor.g,
		RacerScreenBackgroundColor.b, RacerScreenBackgroundColor.a);
	SDL_RenderClear(Context->MainRenderer);

	RenderText(Context->Store.GetFont(FontAssetMainFont), "RACER PROFILE",
		RacerScreenMargin, RacerScreenHeaderTop, RacerScreenAccentColor);

	const SDL_FRect AttributesPanel{ static_cast<float>(RacerScreenLeftColumnX),
		static_cast<float>(RacerScreenPanelTop), static_cast<float>(RacerScreenPanelWidth),
		static_cast<float>(RacerScreenAttributesPanelHeight) };
	DrawRacerScreenPanel(Context->MainRenderer, AttributesPanel, RacerScreenMentalStatColor);
	RenderText(Context->Store.GetFont(FontAssetInfoFont), "SKILLS (0-100)",
		RacerScreenLeftColumnX + RacerScreenPanelPadding, RacerScreenSkillTitleTop, RacerScreenAccentColor);
	RenderText(Context->Store.GetFont(FontAssetDebugFont), "MENTAL",
		RacerScreenLeftColumnX + RacerScreenPanelPadding, RacerScreenMentalHeadingTop, RacerScreenMutedTextColor);

	const int SkillLeft = RacerScreenLeftColumnX + RacerScreenPanelPadding;
	const int SkillWidth = RacerScreenPanelWidth - RacerScreenPanelPadding * 2;
	DrawRacerScreenBar(*this, Context, "Reflex", FormatRating(Runner.Skills.Reflex),
		Runner.Skills.Reflex / RacerScreenMaximumSkillRating, SkillLeft, RacerScreenReflexTop,
		SkillWidth, RacerScreenMentalStatColor);
	DrawRacerScreenBar(*this, Context, "Composure", FormatRating(Runner.Skills.Composure),
		Runner.Skills.Composure / RacerScreenMaximumSkillRating, SkillLeft, RacerScreenComposureTop,
		SkillWidth, RacerScreenMentalStatColor);
	RenderText(Context->Store.GetFont(FontAssetDebugFont), "PHYSICAL",
		RacerScreenLeftColumnX + RacerScreenPanelPadding, RacerScreenPhysicalHeadingTop, RacerScreenMutedTextColor);
	DrawRacerScreenBar(*this, Context, "Acceleration", FormatRating(Runner.Skills.Acceleration),
		Runner.Skills.Acceleration / RacerScreenMaximumSkillRating, SkillLeft, RacerScreenAccelerationTop,
		SkillWidth, RacerScreenPhysicalStatColor);
	DrawRacerScreenBar(*this, Context, "Endurance", FormatRating(Runner.Skills.Endurance),
		Runner.Skills.Endurance / RacerScreenMaximumSkillRating, SkillLeft, RacerScreenEnduranceTop,
		SkillWidth, RacerScreenPhysicalStatColor);
	DrawRacerScreenBar(*this, Context, "Speed", FormatRating(Runner.Skills.Speed),
		Runner.Skills.Speed / RacerScreenMaximumSkillRating, SkillLeft, RacerScreenSpeedTop,
		SkillWidth, RacerScreenPhysicalStatColor);

	const SDL_FRect ConditionsPanel{ static_cast<float>(RacerScreenLeftColumnX),
		static_cast<float>(RacerScreenConditionsPanelTop), static_cast<float>(RacerScreenPanelWidth),
		static_cast<float>(RacerScreenConditionsPanelHeight) };
	DrawRacerScreenPanel(Context->MainRenderer, ConditionsPanel, RacerScreenConfidenceColor);
	RenderText(Context->Store.GetFont(FontAssetInfoFont), "CONDITIONS",
		RacerScreenLeftColumnX + RacerScreenPanelPadding, RacerScreenConditionTitleTop, RacerScreenAccentColor);
	const int ConditionLeft = RacerScreenLeftColumnX + RacerScreenPanelPadding;
	const int ConditionWidth = RacerScreenPanelWidth - RacerScreenPanelPadding * 2;
	DrawRacerScreenBar(*this, Context, "Fatigue", FormatPercentage(Runner.Conditions.Fatigue),
		Runner.Conditions.Fatigue, ConditionLeft, RacerScreenFatigueTop, ConditionWidth, RacerScreenFatigueColor);
	DrawRacerScreenBar(*this, Context, "Confidence", FormatPercentage(Runner.Conditions.Confidence),
		Runner.Conditions.Confidence, ConditionLeft, RacerScreenConfidenceTop, ConditionWidth, RacerScreenConfidenceColor);
	DrawRacerScreenBar(*this, Context, "Injury", FormatPercentage(Runner.Conditions.Injury),
		Runner.Conditions.Injury, ConditionLeft, RacerScreenInjuryTop, ConditionWidth, RacerScreenInjuryColor);

	const SDL_FRect ProfilePanel{ static_cast<float>(RacerScreenRightColumnX),
		static_cast<float>(RacerScreenPanelTop), static_cast<float>(RacerScreenPanelWidth),
		static_cast<float>(RacerScreenProfilePanelHeight) };
	DrawRacerScreenPanel(Context->MainRenderer, ProfilePanel, RacerScreenAccentColor);
	RenderText(Context->Store.GetFont(FontAssetInfoFont), "PROFILE",
		RacerScreenRightColumnX + RacerScreenPanelPadding, RacerScreenPortraitTextTop, RacerScreenAccentColor);
	RenderText(Context->Store.GetFont(FontAssetWinningFont), Runner.Name,
		RacerScreenRightColumnX + RacerScreenPanelPadding, RacerScreenProfileNameTop, RacerScreenTextColor);

	const unsigned int AnimationFrame = static_cast<unsigned int>((SDL_GetTicks() / RacerScreenSpriteFrameDurationMs) %
		RacerScreenSpriteFrameCount);
	SDL_Rect SpriteSource{ static_cast<int>(AnimationFrame) * RacerScreenSpriteWidth, 0,
		RacerScreenSpriteWidth, RacerScreenSpriteHeight };
	SDL_Rect SpriteBounds{ RacerScreenRightColumnX + RacerScreenPanelWidth - RacerScreenPanelPadding -
		RacerScreenSpriteWidth, RacerScreenSpriteTop, RacerScreenSpriteWidth, RacerScreenSpriteHeight };
	RenderImage(Context->Store.GetImage(ImageAssetFellaRun)->Texture, &SpriteSource, &SpriteBounds);

	const SDL_FRect DetailsPanel{ static_cast<float>(RacerScreenRightColumnX),
		static_cast<float>(RacerScreenDetailsPanelTop), static_cast<float>(RacerScreenPanelWidth),
		static_cast<float>(RacerScreenDetailsPanelHeight) };
	DrawRacerScreenPanel(Context->MainRenderer, DetailsPanel, RacerScreenPhysicalStatColor);
	RenderText(Context->Store.GetFont(FontAssetInfoFont), "PERSONAL DETAILS",
		RacerScreenRightColumnX + RacerScreenPanelPadding, RacerScreenDetailTitleTop, RacerScreenAccentColor);
	DrawRacerScreenDetail(*this, Context, "AGE", std::to_string(Runner.Details.Age),
		RacerScreenRightColumnX + RacerScreenPanelPadding, RacerScreenDetailFirstRowTop);
	DrawRacerScreenDetail(*this, Context, "HEIGHT", std::to_string(static_cast<int>(std::lround(Runner.Details.HeightCm))) + " cm",
		RacerScreenRightColumnX + RacerScreenDetailSecondColumnOffset, RacerScreenDetailFirstRowTop);
	DrawRacerScreenDetail(*this, Context, "WEIGHT", std::to_string(static_cast<int>(std::lround(Runner.Details.WeightKg))) + " kg",
		RacerScreenRightColumnX + RacerScreenDetailThirdColumnOffset, RacerScreenDetailFirstRowTop);
	DrawRacerScreenDetail(*this, Context, "CAREER CASH", "$" + std::to_string(RacerProfile->Financials.Cash),
		RacerScreenRightColumnX + RacerScreenPanelPadding, RacerScreenDetailSecondRowTop);

	const SDL_FRect CurvesPanel{ static_cast<float>(RacerScreenRightColumnX),
		static_cast<float>(RacerScreenCurvesPanelTop), static_cast<float>(RacerScreenPanelWidth),
		static_cast<float>(RacerScreenCurvesPanelHeight) };
	DrawRacerScreenPanel(Context->MainRenderer, CurvesPanel, RacerScreenPhysicalStatColor);
	const SDL_FRect CurvePlotBounds{ static_cast<float>(RacerScreenRightColumnX + RacerScreenPanelPadding),
		static_cast<float>(RacerScreenCurvePlotTop),
		static_cast<float>(RacerScreenPanelWidth - RacerScreenPanelPadding * 2),
		static_cast<float>(RacerScreenCurvePlotHeight) };
	// Show the generated, normalized acceleration and endurance curves alongside the raw skill ratings.
	DrawRacerCurveVisualization(*this, Context, Runner, CurvePlotBounds,
		RacerScreenRightColumnX + RacerScreenPanelPadding, RacerScreenCurveTitleTop, RacerScreenCurveLegendTop,
		RacerScreenCurveStyle);

	DrawRacerScreenButton(*this, Context, Screen->RegenButton);
	if (Screen->InitType == Screens::RacerScreenStateInitType::RacerCreation)
		DrawRacerScreenButton(*this, Context, Screen->ApplyButton);

	if (Screen->InitType == Screens::RacerScreenStateInitType::ViewOnly)
		DrawRacerScreenButton(*this, Context, Screen->ExitButton);

	Display();
	const auto EndTime = SDL_GetTicks();
	return static_cast<unsigned int>(EndTime - StartTime);
}
