#include "BuildingScreen.h"
#include "BookiesScreen.h"
#include "BreedingFarmScreen.h"
#include "GymScreen.h"
#include "LicenseOfficeScreen.h"
#include "RaceTrackScreen.h"
#include "StableScreen.h"
#include "..\App\App.h"
#include "..\Audio.h"
#include "SDL3\SDL.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace BuildingScreenDetail
{
	constexpr SDL_Color BuildingBackgroundColor{ 23, 22, 20, 255 };
	constexpr SDL_Color BuildingPanelColor{ 36, 34, 30, 255 };
	constexpr SDL_Color BuildingBorderColor{ 91, 82, 68, 255 };
	constexpr SDL_Color BuildingTextColor{ 239, 231, 214, 255 };
	constexpr SDL_Color BuildingMutedColor{ 174, 166, 147, 255 };
	constexpr SDL_Color BuildingAccentColor{ 178, 61, 49, 255 };
	constexpr std::array<unsigned int, 4> BuildingLicensePrices{ 500, 1000, 2500, 4000 };
	constexpr std::array<const char*, 4> BuildingLicenseNames{
		"Racing", "Breeding", "Bookie Operator", "Track Operator"
	};
	constexpr std::array<const char*, 4> BuildingLicenseDescriptions{
		"Enter future licensed-league races", "Breed owned runners", "Operate a bookmaking business",
		"Own and operate a race track"
	};

	bool ContainsPoint(const Game::App::AppIO& IO, float X, float Y, float Width, float Height)
	{
		return IO.MousePosition.x >= X && IO.MousePosition.x <= X + Width &&
			IO.MousePosition.y >= Y && IO.MousePosition.y <= Y + Height;
	}

	Game::GUI::RelativeLayout GetLayout(const Game::Render::AppRenderContext* Context)
	{
		return Game::GUI::RelativeLayout(static_cast<float>(Context->ScreenWidth),
			static_cast<float>(Context->ScreenHeight));
	}

	SDL_FRect MapBounds(const Game::Render::AppRenderContext* Context, const SDL_FRect& Bounds)
	{
		const Game::GUI::RelativeLayout Layout = GetLayout(Context);
		return { static_cast<float>(Layout.MapX(Bounds.x)), static_cast<float>(Layout.MapY(Bounds.y)),
			Bounds.w, Bounds.h };
	}

	void DrawResponsiveText(Game::Render::BaseRenderer& Renderer,
		Game::Render::AppRenderContext* Context, TTF_Font* Font, const std::string& Text,
		int X, int Y, SDL_Color Color)
	{
		const Game::GUI::RelativeLayout Layout = GetLayout(Context);
		Renderer.RenderText(Font, Text, Layout.MapX(static_cast<float>(X)), Layout.MapY(static_cast<float>(Y)), Color);
	}

	void DrawPanel(Game::Render::AppRenderContext* Context, const SDL_FRect& Bounds, SDL_Color Fill,
		SDL_Color Border)
	{
		const SDL_FRect ResponsiveBounds = MapBounds(Context, Bounds);
		SDL_Renderer* Renderer = Context->MainRenderer;
		SDL_SetRenderDrawColor(Renderer, Fill.r, Fill.g, Fill.b, Fill.a);
		SDL_RenderFillRect(Renderer, &ResponsiveBounds);
		SDL_SetRenderDrawColor(Renderer, Border.r, Border.g, Border.b, Border.a);
		SDL_RenderRect(Renderer, &ResponsiveBounds);
	}

	void DrawActionButton(Game::Render::BaseRenderer& Renderer, Game::Render::AppRenderContext* Context,
		const std::string& Text, int X, int Y, int Width, int Height, bool Selected = false, bool Enabled = true,
		bool MapPosition = true)
	{
		const SDL_Color Fill = !Enabled ? SDL_Color{ 60, 58, 52, 255 } : Selected
			? SDL_Color{ 134, 54, 45, 255 } : SDL_Color{ 84, 55, 43, 255 };
		const Game::GUI::RelativeLayout Layout = GetLayout(Context);
		const SDL_FRect ReferenceBounds{ static_cast<float>(X), static_cast<float>(Y),
			static_cast<float>(Width), static_cast<float>(Height) };
		const SDL_FRect Bounds = MapPosition ? MapBounds(Context, ReferenceBounds) : ReferenceBounds;
		SDL_SetRenderDrawColor(Context->MainRenderer, Fill.r, Fill.g, Fill.b, Fill.a);
		SDL_RenderFillRect(Context->MainRenderer, &Bounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, 162, 116, 79, 255);
		SDL_RenderRect(Context->MainRenderer, &Bounds);
		const int TextX = MapPosition ? Layout.MapX(static_cast<float>(X + 10)) : X + 10;
		const int TextY = MapPosition ? Layout.MapY(static_cast<float>(Y + (Height - 16) / 2)) :
			Y + (Height - 16) / 2;
		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), Text, TextX, TextY,
			Enabled ? BuildingTextColor : BuildingMutedColor);
	}

	std::string GetDirectionName(unsigned int Direction)
	{
		constexpr std::array<const char*, 4> Names{ "UP", "RIGHT", "DOWN", "LEFT" };
		return Names[Direction % Names.size()];
	}

	std::string GetPlotSceneLabel(Game::Career::CityPlotType Type)
	{
		switch (Type)
		{
		case Game::Career::CityPlotType::Gym:
			return "TRAINING FLOOR";
		case Game::Career::CityPlotType::RaceTrack:
			return "RACE OFFICE";
		case Game::Career::CityPlotType::Bookies:
			return "BETTING COUNTER";
		case Game::Career::CityPlotType::LicenseOffice:
			return "LICENSING DESK";
		case Game::Career::CityPlotType::BreedingFarm:
			return "BREEDING DESK";
		case Game::Career::CityPlotType::Stable:
			return "STABLE SPACE";
		default:
			return "CITY BUILDING";
		}
	}

	bool MessageIncludesOwnedRacerName(const Game::App::AppData& Data, const std::string& Message)
	{
		if (Data.Profile != nullptr && !Data.Profile->MainFella.Name.empty() &&
			Message.find(Data.Profile->MainFella.Name) != std::string::npos)
			return true;
		if (Data.Career == nullptr)
			return false;
		return std::any_of(Data.Career->OwnedRacers.begin(), Data.Career->OwnedRacers.end(),
			[&Data, &Message](const Game::Race::Racer* Runner)
			{
				return Data.IsPlayerOwnedRacer(Runner) && !Runner->Name.empty() &&
					Message.find(Runner->Name) != std::string::npos;
			});
	}

	void DrawCareerRacerRow(Game::Render::BaseRenderer& Renderer, Game::Render::AppRenderContext* Context,
		const Game::Race::Racer& Racer, int X, int Y, int Width, bool Selected, const std::string& Prefix = "")
	{
		const SDL_FRect Bounds{ static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Width), 38.0f };
		DrawPanel(Context, Bounds, Selected ? SDL_Color{ 79, 48, 39, 255 } : BuildingPanelColor,
			Selected ? BuildingAccentColor : BuildingBorderColor);
		const std::string Text = Prefix + Racer.Name + "   W " + std::to_string(Racer.CareerRecord.Wins) +
			" / L " + std::to_string(Racer.CareerRecord.Losses);
		DrawResponsiveText(Renderer, Context, Context->Store.GetFont(Game::Assets::FontAssetInfoFont), Text,
			X + 9, Y + 10, Game::Render::PlayerOwnedRacerHighlightColor);
	}
}
using namespace BuildingScreenDetail;

std::string Game::Screens::BuildingScreen::FormatOdds(double Odds)
{
	std::ostringstream Text;
	Text << std::fixed << std::setprecision(2) << Odds;
	return Text.str();
}

std::string Game::Screens::BuildingScreen::FormatTrackLength(unsigned int TrackLength)
{
	if (TrackLength == 0)
		return "No track scheduled";
	return std::to_string(TrackLength / 50) + " m";
}

unsigned int Game::Screens::BuildingScreen::LicensePrice(std::size_t Index)
{
	return BuildingLicensePrices[Index];
}

const char* Game::Screens::BuildingScreen::LicenseName(std::size_t Index)
{
	return BuildingLicenseNames[Index];
}

const char* Game::Screens::BuildingScreen::LicenseDescription(std::size_t Index)
{
	return BuildingLicenseDescriptions[Index];
}

Game::Screens::BuildingScreen::BuildingScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO,
	App::AppData& _Data, AppScreenType _Type, const Career::CityPlot& _Plot)
	: AppScreen(_Machine, _IO, _Data),
	Orchestrator(_Data.Career, _Data.Profile),
	Plot(_Plot),
	BackButton(_IO, "Back to city", 824, 694, 160, 48)
{
	Type = _Type;
	if (Data.Career != nullptr)
	{
		const unsigned int TrackLength = Plot.Type == Career::CityPlotType::RaceTrack ? Plot.TrackLength : 0;
		Orchestrator.PrepareUpcomingRace(SDL_GetTicks(), TrackLength);
	}
}

bool Game::Screens::BuildingScreen::Clicked(float X, float Y, float Width, float Height) const
{
	return (IO.MouseButtonsPressed & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) != 0 &&
		ContainsPoint(IO, MapX(X), MapY(Y), Width, Height);
}

int Game::Screens::BuildingScreen::MapX(float X) const
{
	const float ScreenWidth = Data.RenderContext == nullptr ? 1024.0f :
		static_cast<float>(Data.RenderContext->ScreenWidth);
	const float ScreenHeight = Data.RenderContext == nullptr ? 768.0f :
		static_cast<float>(Data.RenderContext->ScreenHeight);
	return GUI::RelativeLayout(ScreenWidth, ScreenHeight).MapX(X);
}

int Game::Screens::BuildingScreen::MapY(float Y) const
{
	const float ScreenWidth = Data.RenderContext == nullptr ? 1024.0f :
		static_cast<float>(Data.RenderContext->ScreenWidth);
	const float ScreenHeight = Data.RenderContext == nullptr ? 768.0f :
		static_cast<float>(Data.RenderContext->ScreenHeight);
	return GUI::RelativeLayout(ScreenWidth, ScreenHeight).MapY(Y);
}

Game::Screens::AppScreen* Game::Screens::BuildingScreen::Update()
{
	const int ScreenWidth = Data.RenderContext == nullptr ? 1024 :
		static_cast<int>(Data.RenderContext->ScreenWidth);
	const int ScreenHeight = Data.RenderContext == nullptr ? 768 :
		static_cast<int>(Data.RenderContext->ScreenHeight);
	const GUI::RelativeLayout Layout(static_cast<float>(ScreenWidth), static_cast<float>(ScreenHeight));
	BackButton.SetBounds(ScreenWidth - Layout.MapX(40.0f) - 160,
		ScreenHeight - Layout.MapY(26.0f) - 48, 160, 48);
	BackButton.Update();
	if (BackButton.HasMouseClicked() || IO.Esc)
	{
		IO.Player.Play(Audio::BuiltInSounds::Click);
		ScreenStack.Pop();
		return ScreenStack.Top();
	}
	if (Data.Career == nullptr)
	{
		StatusMessage = "Career data is unavailable.";
		return this;
	}

	UpdateActivity();
	return this;
}
Game::Render::BuildingScreenRenderer::BuildingScreenRenderer(AppRenderContext* Context)
	: BaseRenderer(Context)
{
}

unsigned int Game::Render::BuildingScreenRenderer::Render()
{
	const unsigned int StartTime = SDL_GetTicks();
	SDL_SetRenderDrawColor(Context->MainRenderer, BuildingBackgroundColor.r, BuildingBackgroundColor.g,
		BuildingBackgroundColor.b, BuildingBackgroundColor.a);
	SDL_RenderClear(Context->MainRenderer);
	if (Screen == nullptr)
	{
		Display();
		return SDL_GetTicks() - StartTime;
	}

	const Game::GUI::RelativeLayout Layout = GetLayout(Context);
	const auto RenderText = [this, &Layout](TTF_Font* Font, const std::string& Text, int X, int Y,
		SDL_Color Color)
	{
		BaseRenderer::RenderText(Font, Text, Layout.MapX(static_cast<float>(X)),
			Layout.MapY(static_cast<float>(Y)), Color);
	};
	const int SceneLeft = Layout.MapX(24.0f);
	const int SceneTop = Layout.MapY(78.0f);
	const int SceneRight = static_cast<int>(Context->ScreenWidth) - Layout.MapX(24.0f);
	const int SceneBottom = static_cast<int>(Context->ScreenHeight) - Layout.MapY(124.0f);
	const SDL_FRect SceneBounds{ static_cast<float>(SceneLeft), static_cast<float>(SceneTop),
		static_cast<float>(SceneRight - SceneLeft), static_cast<float>(SceneBottom - SceneTop) };
	if (Screen->Plot.SpriteIndex < 12)
	{
		const Assets::ImageAssetId TileAsset = static_cast<Assets::ImageAssetId>(Assets::ImageAssetCityTile000 +
			Screen->Plot.SpriteIndex);
		Game::Render::Image* Tile = Context->Store.GetImage(TileAsset);
		if (Tile != nullptr && Tile->Texture != nullptr)
		{
			SDL_Rect Target{ SceneLeft, SceneTop, SceneRight - SceneLeft, SceneBottom - SceneTop };
			RenderImage(Tile->Texture, nullptr, &Target);
		}
	}
	const SDL_FRect SceneOverlay{ SceneBounds.x, SceneBounds.y, SceneBounds.w, SceneBounds.h };
	SDL_SetRenderDrawColor(Context->MainRenderer, 19, 17, 15, 160);
	SDL_RenderFillRect(Context->MainRenderer, &SceneOverlay);
	SDL_SetRenderDrawColor(Context->MainRenderer, BuildingBorderColor.r, BuildingBorderColor.g,
		BuildingBorderColor.b, BuildingBorderColor.a);
	SDL_RenderRect(Context->MainRenderer, &SceneBounds);
	RenderText(Context->Store.GetFont(Assets::FontAssetMainFont), Screen->Plot.Name, 38, 22, BuildingTextColor);
	const std::string SceneLabel = Screen->Plot.Type == Career::CityPlotType::Stable &&
		static_cast<Screens::StableScreen*>(Screen)->IsCurrentStable()
		? "YOUR STABLE" : GetPlotSceneLabel(Screen->Plot.Type);
	RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), SceneLabel, 40, 88, BuildingMutedColor);

	const int CounterLeft = Layout.MapX(38.0f);
	const int CounterTop = Layout.MapY(596.0f);
	const SDL_FRect Counter{ static_cast<float>(CounterLeft), static_cast<float>(CounterTop),
		static_cast<float>(static_cast<int>(Context->ScreenWidth) - CounterLeft * 2), 46.0f };
	SDL_SetRenderDrawColor(Context->MainRenderer, 66, 45, 34, 255);
	SDL_RenderFillRect(Context->MainRenderer, &Counter);
	SDL_SetRenderDrawColor(Context->MainRenderer, 148, 105, 72, 255);
	SDL_RenderRect(Context->MainRenderer, &Counter);

	const int Cash = Screen->Data.Profile == nullptr ? 0 : Screen->Data.Profile->Financials.Cash;
	RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "Cash: $" + std::to_string(Cash),
		40, 666, BuildingMutedColor);
	DrawActionButton(*this, Context, Screen->BackButton.Text, Screen->BackButton.x, Screen->BackButton.y,
		Screen->BackButton.w, Screen->BackButton.h, Screen->BackButton.IsMouseOver(), true, false);

	switch (Screen->Plot.Type)
	{
	case Career::CityPlotType::Bookies:
	{
		Screens::BookiesScreen* Screen = static_cast<Screens::BookiesScreen*>(this->Screen);
		RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "Race " +
			Screens::BuildingScreen::FormatTrackLength(Screen->Data.Career == nullptr ? 0 : Screen->Data.Career->UpcomingRaceTrackLength) +
			"  |  Bookmaker appraisal: " + std::to_string(Screen->Data.Career == nullptr ? 0 :
				Screen->Data.Career->BookmakerAccuracy) + "%", 42, 116, BuildingTextColor);
		const std::size_t RunnerCount = std::min<std::size_t>(Screen->PredictionEntrants.size(), 6);
		for (std::size_t Index = 0; Index < RunnerCount; ++Index)
		{
			const Race::Racer* Runner = Screen->PredictionEntrants[Index];
			if (Runner == nullptr)
				continue;
			const int Y = 150 + static_cast<int>(Index) * 48;
			const SDL_FRect Row{ 38.0f, static_cast<float>(Y), 612.0f, 42.0f };
			DrawPanel(Context, Row,
				Index == Screen->SelectedRaceEntrant ? SDL_Color{ 83, 49, 40, 255 } : BuildingPanelColor,
				Index == Screen->SelectedRaceEntrant ? BuildingAccentColor : BuildingBorderColor);
			const std::string Odds = Index < Screen->BookieOdds.size()
				? Screens::BuildingScreen::FormatOdds(Screen->BookieOdds[Index]) : "--";
			const std::string Probability = Index < Screen->BookieWinProbabilities.size()
				? Screens::BuildingScreen::FormatOdds(Screen->BookieWinProbabilities[Index] * 100.0) + "%" : "--";
			RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), Runner->Name + "   Win chance " +
				Probability + "   Odds " + Odds, 50, Y + 12,
				Screen->Data.IsPlayerOwnedRacer(Runner) ? Render::PlayerOwnedRacerHighlightColor : BuildingTextColor);
		}
		RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "Stake", 724, 154, BuildingMutedColor);
		DrawActionButton(*this, Context, "$25", 720, 190, 100, 44, Screen->BetStake == 25);
		DrawActionButton(*this, Context, "$100", 830, 190, 100, 44, Screen->BetStake == 100);
		DrawActionButton(*this, Context, "$250", 720, 244, 210, 44, Screen->BetStake == 250);
		DrawActionButton(*this, Context, "Place bet", 720, 310, 210, 52, false,
			!Screen->Data.Career->PendingBet.Active && !Screen->BookieOdds.empty());
		if (Screen->Data.Career->PendingBet.Active)
			RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "Wager placed: $" +
				std::to_string(Screen->Data.Career->PendingBet.Stake), 720, 374, BuildingTextColor);

		DrawPanel(Context, SDL_FRect{ 64.0f, 458.0f, 386.0f, 126.0f }, BuildingPanelColor,
			BuildingBorderColor);
		RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "SLOT MACHINE", 80, 468,
			BuildingTextColor);
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
			"Pairs pay 2x; three of a kind pays 10x.", 80, 492, BuildingMutedColor);
		DrawActionButton(*this, Context, "Enter the slot room", 78, 526, 358, 48);

		DrawPanel(Context, SDL_FRect{ 482.0f, 458.0f, 482.0f, 126.0f }, BuildingPanelColor,
			BuildingBorderColor);
		RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "ROULETTE TABLE", 500, 468,
			BuildingTextColor);
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
			"Colors pay 1:1; a single number pays 35:1.", 500, 492, BuildingMutedColor);
		DrawActionButton(*this, Context, "Take a seat", 500, 526, 444, 48);
		break;
	}
	case Career::CityPlotType::Gym:
	{
		Screens::GymScreen* Screen = static_cast<Screens::GymScreen*>(this->Screen);
		RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "Choose a runner, then click a training station.",
			42, 118, BuildingTextColor);
		const std::vector<Race::Racer*>& OwnedRacers = Screen->Data.Career->OwnedRacers;
		for (std::size_t Index = 0; Index < OwnedRacers.size() && Index < 8; ++Index)
			if (OwnedRacers[Index] != nullptr)
				DrawCareerRacerRow(*this, Context, *OwnedRacers[Index], 38, 160 + static_cast<int>(Index) * 46,
					300, Index == Screen->SelectedOwnedRacerIndex);
		DrawActionButton(*this, Context, "TREADMILL", 382, 160, 180, 60,
			Screen->ActiveGymActivity == Screens::GymScreen::GymActivity::Treadmill);
		DrawActionButton(*this, Context, "WEIGHTS", 576, 160, 180, 60,
			Screen->ActiveGymActivity == Screens::GymScreen::GymActivity::Weights);
		DrawActionButton(*this, Context, "DANCE", 770, 160, 180, 60,
			Screen->ActiveGymActivity == Screens::GymScreen::GymActivity::Dance);

		if (Screen->SelectedOwnedRacerIndex < OwnedRacers.size() && OwnedRacers[Screen->SelectedOwnedRacerIndex] != nullptr)
		{
			const Race::Racer& Runner = *OwnedRacers[Screen->SelectedOwnedRacerIndex];
			RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "Selected: " + Runner.Name +
				"   Endurance " + Screens::BuildingScreen::FormatOdds(Runner.Skills.Endurance) + "   Acceleration " +
				Screens::BuildingScreen::FormatOdds(Runner.Skills.Acceleration), 382, 248,
				Screen->Data.IsPlayerOwnedRacer(&Runner) ? Render::PlayerOwnedRacerHighlightColor : BuildingTextColor);
		}
		if (Screen->ActiveGymActivity == Screens::GymScreen::GymActivity::Treadmill ||
			Screen->ActiveGymActivity == Screens::GymScreen::GymActivity::Weights)
		{
			RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "Good reps: " +
				std::to_string(Screen->WorkoutHits) + " / " + std::to_string(Screens::GymScreen::WorkoutAttemptCount), 382, 310,
				BuildingTextColor);
			const SDL_FRect Meter{ 402.0f, 366.0f, 520.0f, 32.0f };
			const SDL_FRect ResponsiveMeter = MapBounds(Context, Meter);
			DrawPanel(Context, Meter, SDL_Color{ 53, 51, 45, 255 }, BuildingBorderColor);
			const SDL_FRect Zone{ ResponsiveMeter.x + ResponsiveMeter.w *
				(Screen->ActiveGymActivity == Screens::GymScreen::GymActivity::Treadmill
				? 0.35f : 0.46f), ResponsiveMeter.y + 2.0f,
				ResponsiveMeter.w * (Screen->ActiveGymActivity == Screens::GymScreen::GymActivity::Treadmill
				? 0.30f : 0.10f), ResponsiveMeter.h - 4.0f };
			SDL_SetRenderDrawColor(Context->MainRenderer, 74, 151, 96, 255);
			SDL_RenderFillRect(Context->MainRenderer, &Zone);
			const double Phase = std::fmod(static_cast<double>(SDL_GetTicks() - Screen->WorkoutStartedAt), 1200.0) / 1200.0;
			const SDL_FRect Marker{ ResponsiveMeter.x + ResponsiveMeter.w * static_cast<float>(Phase) - 3.0f,
				ResponsiveMeter.y - 5.0f, 6.0f, ResponsiveMeter.h + 10.0f };
			SDL_SetRenderDrawColor(Context->MainRenderer, 245, 220, 159, 255);
			SDL_RenderFillRect(Context->MainRenderer, &Marker);
			DrawActionButton(*this, Context,
				Screen->ActiveGymActivity == Screens::GymScreen::GymActivity::Treadmill ? "RUN" : "LIFT",
				622, 510, 210, 54);
		}
		else if (Screen->ActiveGymActivity == Screens::GymScreen::GymActivity::Dance)
		{
			std::string Sequence = "Sequence: ";
			for (std::size_t Index = 0; Index < Screen->DanceSequence.size(); ++Index)
				Sequence += Index < Screen->DanceProgress ? "[" + GetDirectionName(Screen->DanceSequence[Index]) + "] "
					: GetDirectionName(Screen->DanceSequence[Index]) + " ";
			RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), Sequence, 400, 350, BuildingTextColor);
			constexpr std::array<const char*, 4> Directions{ "UP", "RIGHT", "DOWN", "LEFT" };
			for (std::size_t Index = 0; Index < Directions.size(); ++Index)
				DrawActionButton(*this, Context, Directions[Index], 520 + static_cast<int>(Index) * 86, 478, 78, 52);
		}
		break;
	}
	case Career::CityPlotType::LicenseOffice:
	{
		Screens::LicenseOfficeScreen* Screen = static_cast<Screens::LicenseOfficeScreen*>(this->Screen);
		RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "Activity licenses unlock specific activities and provide no stat bonuses.",
			68, 126, BuildingTextColor);
		for (std::size_t Index = 0; Index < Screen->Data.Career->Licenses.size(); ++Index)
		{
			const int Y = 168 + static_cast<int>(Index) * 92;
			const SDL_FRect Card{ 100.0f, static_cast<float>(Y), 824.0f, 72.0f };
			DrawPanel(Context, Card, BuildingPanelColor,
				Screen->Data.Career->Licenses[Index] ? SDL_Color{ 91, 151, 101, 255 } : BuildingBorderColor);
			const std::string Status = Screen->Data.Career->Licenses[Index] ? "OWNED" : "$" +
				std::to_string(Screens::BuildingScreen::LicensePrice(Index));
			RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), std::string(Screens::BuildingScreen::LicenseName(Index)) +
				" license   -   " + Status, 124, Y + 14, BuildingTextColor);
			RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), Screens::BuildingScreen::LicenseDescription(Index),
				124, Y + 43, BuildingMutedColor);
		}
		break;
	}
	case Career::CityPlotType::BreedingFarm:
	{
		Screens::BreedingFarmScreen* Screen = static_cast<Screens::BreedingFarmScreen*>(this->Screen);
		RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont),
			"Generate and buy a prospect for $" + std::to_string(Career::CareerBreederRacerPrice) +
				"; breeding owned pairs requires a license.",
			42, 118, BuildingTextColor);
		const std::vector<Race::Racer*>& OwnedRacers = Screen->Data.Career->OwnedRacers;
		for (std::size_t Index = 0; Index < OwnedRacers.size() && Index < 8; ++Index)
			if (OwnedRacers[Index] != nullptr)
			{
				const int Y = 160 + static_cast<int>(Index) * 46;
				DrawCareerRacerRow(*this, Context, *OwnedRacers[Index], 42, Y, 430,
					Index == Screen->FirstParentIndex, "A: ");
				DrawCareerRacerRow(*this, Context, *OwnedRacers[Index], 522, Y, 430,
					Index == Screen->SecondParentIndex, "B: ");
			}
		const bool CanGenerateRacer = Screen->Data.Career->PlayerStableCityId != 0 &&
			OwnedRacers.size() < Career::CareerStableRacerCapacity && Screen->Data.Profile != nullptr &&
			Screen->Data.Profile->Financials.Cash >= Career::CareerBreederRacerPrice;
		DrawActionButton(*this, Context, Screen->GenerateRacerButton.Text, Screen->GenerateRacerButton.x,
			Screen->GenerateRacerButton.y, Screen->GenerateRacerButton.w, Screen->GenerateRacerButton.h,
			Screen->GenerateRacerButton.IsMouseOver(), CanGenerateRacer, false);
		DrawActionButton(*this, Context, "Breed pair - $1,000", 680, 596, 230, 56,
			false, Screen->Data.Career->Licenses[1] && OwnedRacers.size() >= 2 &&
				OwnedRacers.size() < Career::CareerStableRacerCapacity);
		break;
	}
	case Career::CityPlotType::RaceTrack:
	{
		Screens::RaceTrackScreen* Screen = static_cast<Screens::RaceTrackScreen*>(this->Screen);
		const std::vector<std::shared_ptr<Career::CareerWorldRace>> ActiveRaces = Screen->GetActiveWorldRaces();
		const std::size_t MaximumScrollOffset = ActiveRaces.size() > Screens::RaceTrackScreen::VisibleRaceCount
			? ActiveRaces.size() - Screens::RaceTrackScreen::VisibleRaceCount : 0;
		const std::size_t FirstRace = std::min(Screen->RaceScrollOffset, MaximumScrollOffset);
		const std::size_t EndRace = std::min(ActiveRaces.size(), FirstRace +
			Screens::RaceTrackScreen::VisibleRaceCount);
		RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "TRACK: " +
			Screens::BuildingScreen::FormatTrackLength(Screen->Plot.TrackLength) + "   |   Live races: " +
			std::to_string(ActiveRaces.size()), 62, 132, BuildingTextColor);
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
			"Click a race to watch it; scroll the list to browse all live races.", 62, 150, BuildingMutedColor);
		if (ActiveRaces.empty())
			RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont),
				"No races are in progress on this track.", 62, 190, BuildingMutedColor);
		for (std::size_t Index = FirstRace; Index < EndRace; ++Index)
		{
			const std::shared_ptr<Career::CareerWorldRace>& WorldRace = ActiveRaces[Index];
			if (WorldRace->StateMachine == nullptr || WorldRace->StateMachine->State == nullptr)
				continue;
			const int RowY = 166 + static_cast<int>(Index - FirstRace) * 50;
			const SDL_FRect RowBounds = MapBounds(Context,
				SDL_FRect{ 62.0f, static_cast<float>(RowY), 900.0f, 42.0f });
			const bool IsHovered = Screen->IO.MousePosition.x >= RowBounds.x &&
				Screen->IO.MousePosition.x <= RowBounds.x + RowBounds.w &&
				Screen->IO.MousePosition.y >= RowBounds.y &&
				Screen->IO.MousePosition.y <= RowBounds.y + RowBounds.h;
			const SDL_Color RowFill = IsHovered ? SDL_Color{ 74, 43, 35, 255 } : BuildingPanelColor;
			SDL_SetRenderDrawColor(Context->MainRenderer, RowFill.r, RowFill.g, RowFill.b, RowFill.a);
			SDL_RenderFillRect(Context->MainRenderer, &RowBounds);
			SDL_SetRenderDrawColor(Context->MainRenderer, BuildingBorderColor.r, BuildingBorderColor.g,
				BuildingBorderColor.b, BuildingBorderColor.a);
			SDL_RenderRect(Context->MainRenderer, &RowBounds);
			std::string EntrantNames;
			bool HasOwnedEntrant = false;
			for (const Race::Racer* Runner : WorldRace->Entrants)
				if (Runner != nullptr)
				{
					HasOwnedEntrant = HasOwnedEntrant || Screen->Data.IsPlayerOwnedRacer(Runner);
					if (!EntrantNames.empty())
						EntrantNames += "  |  ";
					EntrantNames += Runner->Name;
				}
			if (EntrantNames.size() > 84)
				EntrantNames = EntrantNames.substr(0, 81) + "...";
			const std::string Phase = Race::RaceStateTypeToString(WorldRace->StateMachine->State->Type);
			const std::string Elapsed = std::to_string(
				static_cast<unsigned int>(WorldRace->PlaybackElapsedMs / 1000.0)) + "s";
			RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), WorldRace->EventName + "   |   " +
				Phase + "   |   " + Elapsed, 74, RowY + 5, BuildingTextColor);
			RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), EntrantNames, 74, RowY + 23,
				HasOwnedEntrant ? Render::PlayerOwnedRacerHighlightColor : BuildingMutedColor);
		}
		if (ActiveRaces.size() > Screens::RaceTrackScreen::VisibleRaceCount)
			RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), "Showing " +
				std::to_string(FirstRace + 1) + "-" + std::to_string(EndRace) + " of " +
				std::to_string(ActiveRaces.size()) + " live races; scroll to browse. Public events need no license.",
				62, 566, BuildingMutedColor);
		else
			RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
				"Public amateur event - a racing license is not required.", 62, 566, BuildingMutedColor);
		DrawActionButton(*this, Context, "Start race", 590, 596, 300, 56,
			false, Screen->Data.Career->UpcomingRaceEntrants.size() >= 2);
		break;
	}
	case Career::CityPlotType::Stable:
	{
		Screens::StableScreen* Screen = static_cast<Screens::StableScreen*>(this->Screen);
		if (!Screen->IsCurrentStable())
		{
			if (Screen->Data.Career->PlayerStableCityId == 0)
			{
				RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont),
					"Rent space here for $" + std::to_string(Career::CareerStableLeasePrice) +
					". Your stable holds up to six racers.", 42, 118, BuildingTextColor);
				DrawActionButton(*this, Context, Screen->GetRentStableButton().Text,
					Screen->GetRentStableButton().x, Screen->GetRentStableButton().y,
					Screen->GetRentStableButton().w, Screen->GetRentStableButton().h,
					Screen->GetRentStableButton().IsMouseOver(), Screen->CanRentSpace(), false);
			}
			else
			{
				const auto LeasedCity = std::find_if(Screen->Data.Career->Cities.begin(),
					Screen->Data.Career->Cities.end(), [Screen](const Career::CareerWorldCity& City)
					{
						return City.Id == Screen->Data.Career->PlayerStableCityId;
					});
				RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont),
					"Your stable lease is in " + (LeasedCity == Screen->Data.Career->Cities.end()
						? std::string("another city") : LeasedCity->Name) + ".",
					42, 118, BuildingTextColor);
			}
			break;
		}

		RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "YOUR STABLE  |  " +
			std::to_string(Screen->Data.Career->OwnedRacers.size()) + "/" +
			std::to_string(Career::CareerStableRacerCapacity) + " spaces used",
			42, 118, BuildingTextColor);
		const std::vector<Race::Racer*>& OwnedRacers = Screen->Data.Career->OwnedRacers;
		if (OwnedRacers.empty())
			RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont),
				"Your stable is empty. Visit the breeder to buy your first racer.", 42, 160, BuildingMutedColor);
		for (std::size_t Index = 0; Index < OwnedRacers.size() && Index < 9; ++Index)
			if (OwnedRacers[Index] != nullptr)
				DrawCareerRacerRow(*this, Context, *OwnedRacers[Index], 42, 160 + static_cast<int>(Index) * 46,
					430, Index == Screen->SelectedOwnedRacerIndex);
		if (Screen->SelectedOwnedRacerIndex < OwnedRacers.size() && OwnedRacers[Screen->SelectedOwnedRacerIndex] != nullptr)
		{
			const Race::Racer& Runner = *OwnedRacers[Screen->SelectedOwnedRacerIndex];
			RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "RACER PROFILE", 520, 164, BuildingTextColor);
			RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "Reflex: " + Screens::BuildingScreen::FormatOdds(Runner.Skills.Reflex) +
				"     Composure: " + Screens::BuildingScreen::FormatOdds(Runner.Skills.Composure), 520, 206, BuildingTextColor);
			RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "Acceleration: " + Screens::BuildingScreen::FormatOdds(Runner.Skills.Acceleration) +
				"     Endurance: " + Screens::BuildingScreen::FormatOdds(Runner.Skills.Endurance), 520, 244, BuildingTextColor);
			RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "Speed: " + Screens::BuildingScreen::FormatOdds(Runner.Skills.Speed) +
				"     Potential: " + Screens::BuildingScreen::FormatOdds(Runner.Skills.Potential), 520, 282, BuildingTextColor);
			RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "Best time: " +
				(Runner.CareerRecord.BestTimeMs == 0 ? "-" : std::to_string(Runner.CareerRecord.BestTimeMs) + " ms"),
				520, 320, BuildingTextColor);
		}
		break;
	}
	default:
		break;
	}

	if (!Screen->StatusMessage.empty())
		RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), Screen->StatusMessage, 40, 646,
			MessageIncludesOwnedRacerName(Screen->Data, Screen->StatusMessage)
				? Render::PlayerOwnedRacerHighlightColor : BuildingTextColor);
	Display();
	return SDL_GetTicks() - StartTime;
}

// The Visual C++ project compiles an explicit source list, so this dedicated screen joins an existing unit.
#include "RouletteScreen.cpp"
