#include "RouletteScreen.h"
#include "..\App\App.h"
#include "..\Audio.h"
#include "SDL3\SDL.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <random>

namespace RouletteScreenDetail
{
	constexpr float Pi = 3.14159265358979323846f;
	constexpr float TwoPi = Pi * 2.0f;
	constexpr float WheelRadius = 150.0f;
	constexpr float WheelInnerRadius = 79.0f;
	constexpr float NumberCellWidth = 34.0f;
	constexpr float NumberCellHeight = 42.0f;
	constexpr float NumberCellGap = 3.0f;
	constexpr float NumberGridX = 514.0f;
	constexpr float NumberGridY = 202.0f;
	constexpr std::array<unsigned int, 18> RedNumbers{
		1, 3, 5, 7, 9, 12, 14, 16, 18, 19, 21, 23, 25, 27, 30, 32, 34, 36
	};
	constexpr std::array<unsigned int, 37> WheelOrder{
		0, 32, 15, 19, 4, 21, 2, 25, 17, 34, 6, 27, 13, 36, 11, 30, 8, 23, 10,
		5, 24, 16, 33, 1, 20, 14, 31, 9, 22, 18, 29, 7, 28, 12, 35, 3, 26
	};
	constexpr SDL_Color CasinoText{ 244, 231, 199, 255 };
	constexpr SDL_Color CasinoMuted{ 178, 162, 132, 255 };
	constexpr SDL_Color CasinoGold{ 224, 180, 90, 255 };
	constexpr SDL_Color RouletteGreen{ 35, 112, 67, 255 };
	constexpr SDL_Color RouletteRed{ 159, 49, 48, 255 };
	constexpr SDL_Color RouletteBlack{ 34, 36, 34, 255 };

	int GetRouletteColor(unsigned int Number)
	{
		if (Number == 0)
			return 0;
		return std::find(RedNumbers.begin(), RedNumbers.end(), Number) != RedNumbers.end() ? 1 : 2;
	}

	SDL_Color GetRouletteColorFill(unsigned int Number)
	{
		const int Color = GetRouletteColor(Number);
		return Color == 0 ? RouletteGreen : Color == 1 ? RouletteRed : RouletteBlack;
	}

	const char* GetColorName(int Color)
	{
		return Color == 0 ? "GREEN" : Color == 1 ? "RED" : "BLACK";
	}

	SDL_FRect GetNumberBounds(unsigned int Number)
	{
		if (Number == 0)
			return { 468.0f, NumberGridY, 38.0f, NumberCellHeight * 3.0f + NumberCellGap * 2.0f };

		const unsigned int Column = (Number - 1) / 3;
		const unsigned int Row = 2 - (Number - 1) % 3;
		return { NumberGridX + static_cast<float>(Column) * (NumberCellWidth + NumberCellGap),
			NumberGridY + static_cast<float>(Row) * (NumberCellHeight + NumberCellGap),
			NumberCellWidth, NumberCellHeight };
	}

	void DrawRoulettePanel(SDL_Renderer* Renderer, const SDL_FRect& Bounds, SDL_Color Fill, SDL_Color Border)
	{
		SDL_SetRenderDrawColor(Renderer, Fill.r, Fill.g, Fill.b, Fill.a);
		SDL_RenderFillRect(Renderer, &Bounds);
		SDL_SetRenderDrawColor(Renderer, Border.r, Border.g, Border.b, Border.a);
		SDL_RenderRect(Renderer, &Bounds);
	}

	void DrawCasinoButton(Game::Render::BaseRenderer& Renderer, Game::Render::AppRenderContext* Context,
		const std::string& Text, const SDL_FRect& Bounds, SDL_FPoint Mouse, bool Selected, bool Enabled = true)
	{
		const bool Hovered = Enabled && Mouse.x >= Bounds.x && Mouse.x <= Bounds.x + Bounds.w &&
			Mouse.y >= Bounds.y && Mouse.y <= Bounds.y + Bounds.h;
		const SDL_Color Fill = !Enabled ? SDL_Color{ 55, 58, 52, 255 } : Selected
			? SDL_Color{ 105, 78, 42, 255 } : Hovered ? SDL_Color{ 88, 82, 47, 255 }
				: SDL_Color{ 48, 68, 51, 255 };
		DrawRoulettePanel(Context->MainRenderer, Bounds, Fill, Selected ? CasinoGold : SDL_Color{ 112, 133, 100, 255 });
		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), Text,
			static_cast<int>(Bounds.x) + 10, static_cast<int>(Bounds.y + (Bounds.h - 16.0f) / 2.0f),
			Enabled ? CasinoText : CasinoMuted);
	}

	void DrawNumberButton(Game::Render::BaseRenderer& Renderer, Game::Render::AppRenderContext* Context,
		unsigned int Number, SDL_FPoint Mouse, bool Selected, bool Winning, bool Enabled)
	{
		const SDL_FRect Bounds = GetNumberBounds(Number);
		const bool Hovered = Enabled && Mouse.x >= Bounds.x && Mouse.x <= Bounds.x + Bounds.w &&
			Mouse.y >= Bounds.y && Mouse.y <= Bounds.y + Bounds.h;
		SDL_Color Fill = GetRouletteColorFill(Number);
		if (!Enabled)
			Fill = { static_cast<Uint8>(Fill.r / 2), static_cast<Uint8>(Fill.g / 2),
				static_cast<Uint8>(Fill.b / 2), 255 };
		else if (Hovered)
			Fill = { static_cast<Uint8>(std::min(255, Fill.r + 28)),
				static_cast<Uint8>(std::min(255, Fill.g + 28)),
				static_cast<Uint8>(std::min(255, Fill.b + 28)), 255 };
		const SDL_Color Border = Winning ? SDL_Color{ 246, 220, 126, 255 } :
			Selected ? CasinoGold : SDL_Color{ 82, 112, 86, 255 };
		DrawRoulettePanel(Context->MainRenderer, Bounds, Fill, Border);
		const std::string Label = std::to_string(Number);
		const int TextWidth = Number == 0 ? 8 : 15;
		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), Label,
			static_cast<int>(Bounds.x + (Bounds.w - static_cast<float>(TextWidth)) / 2.0f),
			static_cast<int>(Bounds.y + (Bounds.h - 16.0f) / 2.0f), CasinoText);
	}

	void DrawFilledCircle(SDL_Renderer* Renderer, float CenterX, float CenterY, int Radius, SDL_Color Color)
	{
		SDL_SetRenderDrawColor(Renderer, Color.r, Color.g, Color.b, Color.a);
		for (int OffsetY = -Radius; OffsetY <= Radius; ++OffsetY)
		{
			const float Remaining = static_cast<float>(Radius * Radius - OffsetY * OffsetY);
			const float HalfWidth = std::sqrt(std::max(0.0f, Remaining));
			SDL_RenderLine(Renderer, CenterX - HalfWidth, CenterY + static_cast<float>(OffsetY),
				CenterX + HalfWidth, CenterY + static_cast<float>(OffsetY));
		}
	}

	void DrawCircleOutline(SDL_Renderer* Renderer, float CenterX, float CenterY, float Radius, SDL_Color Color)
	{
		SDL_SetRenderDrawColor(Renderer, Color.r, Color.g, Color.b, Color.a);
		constexpr unsigned int SegmentCount = 96;
		for (unsigned int Index = 0; Index < SegmentCount; ++Index)
		{
			const float StartAngle = TwoPi * static_cast<float>(Index) / SegmentCount;
			const float EndAngle = TwoPi * static_cast<float>(Index + 1) / SegmentCount;
			SDL_RenderLine(Renderer, CenterX + std::cos(StartAngle) * Radius,
				CenterY + std::sin(StartAngle) * Radius,
				CenterX + std::cos(EndAngle) * Radius, CenterY + std::sin(EndAngle) * Radius);
		}
	}

	void DrawRingSegment(SDL_Renderer* Renderer, float CenterX, float CenterY, float InnerRadius,
		float OuterRadius, float StartAngle, float EndAngle, SDL_Color Color)
	{
		const SDL_FColor VertexColor{ static_cast<float>(Color.r) / 255.0f,
			static_cast<float>(Color.g) / 255.0f, static_cast<float>(Color.b) / 255.0f,
			static_cast<float>(Color.a) / 255.0f };
		const SDL_Vertex Vertices[] = {
			{ { CenterX + std::cos(StartAngle) * InnerRadius, CenterY + std::sin(StartAngle) * InnerRadius },
				VertexColor, { 0.0f, 0.0f } },
			{ { CenterX + std::cos(StartAngle) * OuterRadius, CenterY + std::sin(StartAngle) * OuterRadius },
				VertexColor, { 0.0f, 0.0f } },
			{ { CenterX + std::cos(EndAngle) * OuterRadius, CenterY + std::sin(EndAngle) * OuterRadius },
				VertexColor, { 0.0f, 0.0f } },
			{ { CenterX + std::cos(EndAngle) * InnerRadius, CenterY + std::sin(EndAngle) * InnerRadius },
				VertexColor, { 0.0f, 0.0f } }
		};
		constexpr int Indices[]{ 0, 1, 2, 0, 2, 3 };
		SDL_RenderGeometry(Renderer, nullptr, Vertices, 4, Indices, 6);
	}

	void DrawRouletteWheel(SDL_Renderer* Renderer, float CenterX, float CenterY, float Rotation)
	{
		DrawFilledCircle(Renderer, CenterX, CenterY, static_cast<int>(WheelRadius), SDL_Color{ 70, 47, 31, 255 });
		DrawCircleOutline(Renderer, CenterX, CenterY, WheelRadius, CasinoGold);
		const float SegmentAngle = TwoPi / static_cast<float>(WheelOrder.size());
		for (std::size_t Index = 0; Index < WheelOrder.size(); ++Index)
		{
			const float StartAngle = -Pi / 2.0f + Rotation + static_cast<float>(Index) * SegmentAngle + 0.004f;
			const float EndAngle = -Pi / 2.0f + Rotation + static_cast<float>(Index + 1) * SegmentAngle - 0.004f;
			DrawRingSegment(Renderer, CenterX, CenterY, WheelInnerRadius, 137.0f,
				StartAngle, EndAngle, GetRouletteColorFill(WheelOrder[Index]));
		}
		DrawCircleOutline(Renderer, CenterX, CenterY, 138.0f, SDL_Color{ 239, 205, 132, 255 });
		DrawCircleOutline(Renderer, CenterX, CenterY, WheelInnerRadius, CasinoGold);
		DrawFilledCircle(Renderer, CenterX, CenterY, static_cast<int>(WheelInnerRadius - 2.0f),
			SDL_Color{ 28, 75, 50, 255 });
		DrawCircleOutline(Renderer, CenterX, CenterY, WheelInnerRadius - 2.0f, SDL_Color{ 109, 145, 97, 255 });

		const SDL_FRect Pointer{ CenterX - 7.0f, CenterY - WheelRadius - 14.0f, 14.0f, 18.0f };
		SDL_SetRenderDrawColor(Renderer, CasinoGold.r, CasinoGold.g, CasinoGold.b, CasinoGold.a);
		SDL_RenderFillRect(Renderer, &Pointer);
		const SDL_FRect Ball{ CenterX - 5.0f, CenterY - 127.0f, 10.0f, 10.0f };
		SDL_SetRenderDrawColor(Renderer, 250, 244, 223, 255);
		SDL_RenderFillRect(Renderer, &Ball);
}
}
using namespace RouletteScreenDetail;

Game::Screens::RouletteScreen::RouletteScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO,
	App::AppData& _Data, const Career::CityPlot& _Plot)
	: BuildingScreen(_Machine, _IO, _Data, AppScreenType::RouletteScreen, _Plot),
	Random(std::random_device{}())
{
	BackButton.Text = "Back to bookie";
}

void Game::Screens::RouletteScreen::UpdateActivity()
{
	if (IsSpinning)
	{
		const float Progress = std::min(static_cast<float>(SDL_GetTicks() - SpinStartedAt) /
			static_cast<float>(SpinDuration), 1.0f);
		const float Deceleration = 1.0f - std::pow(1.0f - Progress, 4.0f);
		WheelRotation = SpinStartRotation + SpinRotationDelta * Deceleration;
		if (Progress >= 1.0f)
		{
			IsSpinning = false;
			HasResult = true;
			WheelRotation = std::fmod(SpinStartRotation + SpinRotationDelta, TwoPi);
			ResultMessage = PendingMessage;
		}
		return;
	}

	if (Clicked(514.0f, 352.0f, 208.0f, 46.0f))
	{
		SelectedNumber = -1;
		SelectedColor = 1;
		return;
	}
	if (Clicked(736.0f, 352.0f, 219.0f, 46.0f))
	{
		SelectedNumber = -1;
		SelectedColor = 2;
		return;
	}
	if (Clicked(514.0f, 420.0f, 88.0f, 48.0f))
		BetStake = 25;
	else if (Clicked(612.0f, 420.0f, 88.0f, 48.0f))
		BetStake = 50;
	else if (Clicked(710.0f, 420.0f, 88.0f, 48.0f))
		BetStake = 100;
	else if (Clicked(810.0f, 420.0f, 145.0f, 48.0f))
	{
		Spin();
		return;
	}
	else
	{
		for (unsigned int Number = 0; Number <= 36; ++Number)
		{
			const SDL_FRect Bounds = GetNumberBounds(Number);
			if (Clicked(Bounds.x, Bounds.y, Bounds.w, Bounds.h))
			{
				SelectedNumber = static_cast<int>(Number);
				return;
			}
		}
	}
}

void Game::Screens::RouletteScreen::Spin()
{
	if (Data.Profile == nullptr || Data.Profile->Financials.Cash < static_cast<int>(BetStake))
	{
		ResultMessage = "Not enough cash for that stake.";
		LastSpinWon = false;
		return;
	}

	Data.Profile->Financials.Cash -= static_cast<int>(BetStake);
	std::uniform_int_distribution<unsigned int> NumberDistribution(0, 36);
	RouletteNumber = NumberDistribution(Random);
	RouletteColor = GetRouletteColor(RouletteNumber);
	unsigned int Payout = 0;
	if (SelectedNumber >= 0 && static_cast<unsigned int>(SelectedNumber) == RouletteNumber)
		Payout = BetStake * 36;
	else if (SelectedNumber < 0 && RouletteNumber != 0 && RouletteColor == SelectedColor)
		Payout = BetStake * 2;
	Data.Profile->Financials.Cash += static_cast<int>(Payout);
	LastSpinWon = Payout > 0;

	if (SelectedNumber >= 0)
		PendingMessage = LastSpinWon ? "NUMBER HIT! #" + std::to_string(RouletteNumber) +
			" returns $" + std::to_string(Payout) + "." : "No hit on #" + std::to_string(SelectedNumber) +
			". Result: " + std::to_string(RouletteNumber) + " " + GetColorName(RouletteColor) + ".";
	else
		PendingMessage = LastSpinWon ? std::string(GetColorName(RouletteColor)) + " " +
			std::to_string(RouletteNumber) + " - WIN! $" + std::to_string(Payout) + " returned."
			: "No payout. Result: " + std::string(GetColorName(RouletteColor)) + " " +
				std::to_string(RouletteNumber) + ".";

	const auto WinningPocket = std::find(WheelOrder.begin(), WheelOrder.end(), RouletteNumber);
	const float TargetRotation = -(static_cast<float>(std::distance(WheelOrder.begin(), WinningPocket)) + 0.5f) *
		(TwoPi / static_cast<float>(WheelOrder.size()));
	SpinStartRotation = WheelRotation;
	float RotationDelta = std::fmod(TargetRotation - SpinStartRotation, TwoPi);
	if (RotationDelta < 0.0f)
		RotationDelta += TwoPi;
	SpinRotationDelta = RotationDelta + TwoPi * 6.0f;
	SpinStartedAt = SDL_GetTicks();
	HasResult = false;
	IsSpinning = true;
	ResultMessage = "The wheel is spinning...";
	IO.Player.Play(Audio::BuiltInSounds::Click);
}

Game::Render::RouletteScreenRenderer::RouletteScreenRenderer(AppRenderContext* Context)
	: BaseRenderer(Context)
{
}

unsigned int Game::Render::RouletteScreenRenderer::Render()
{
	const unsigned int StartTime = SDL_GetTicks();
	SDL_SetRenderDrawColor(Context->MainRenderer, 16, 35, 27, 255);
	SDL_RenderClear(Context->MainRenderer);
	if (Screen == nullptr)
	{
		Display();
		return SDL_GetTicks() - StartTime;
	}

	RenderText(Context->Store.GetFont(Game::Assets::FontAssetMainFont), "ROULETTE TABLE", 52, 32, CasinoGold);
	RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont),
		"European wheel - 37 pockets - place your bets before the spin.", 54, 60, CasinoMuted);
	const SDL_FRect CashPanel{ 790.0f, 28.0f, 186.0f, 48.0f };
	DrawRoulettePanel(Context->MainRenderer, CashPanel, SDL_Color{ 27, 48, 34, 255 }, CasinoGold);
	const int Cash = Screen->Data.Profile == nullptr ? 0 : Screen->Data.Profile->Financials.Cash;
	RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), "CASH  $" + std::to_string(Cash),
		808, 44, CasinoText);

	const SDL_FRect WheelPanel{ 42.0f, 124.0f, 370.0f, 454.0f };
	DrawRoulettePanel(Context->MainRenderer, WheelPanel, SDL_Color{ 26, 58, 40, 255 }, SDL_Color{ 88, 130, 89, 255 });
	RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), "EUROPEAN WHEEL", 134, 143, CasinoText);
	constexpr float WheelCenterX = 227.0f;
	constexpr float WheelCenterY = 341.0f;
	DrawRouletteWheel(Context->MainRenderer, WheelCenterX, WheelCenterY, Screen->WheelRotation);
	const std::string CenterNumber = Screen->IsSpinning ? "..." :
		Screen->HasResult ? std::to_string(Screen->RouletteNumber) : "READY";
	const int CenterTextWidth = static_cast<int>(CenterNumber.size()) * 9;
	RenderText(Context->Store.GetFont(Game::Assets::FontAssetMainFont), CenterNumber,
		static_cast<int>(WheelCenterX) - CenterTextWidth / 2, 329,
		Screen->RouletteColor == 0 ? RouletteGreen : Screen->RouletteColor == 1 ? SDL_Color{ 247, 185, 140, 255 }
			: CasinoText);
	if (Screen->HasResult && !Screen->IsSpinning)
		RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), GetColorName(Screen->RouletteColor),
			static_cast<int>(WheelCenterX) - 18, 355, CasinoMuted);
	RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont),
		"0 is green; single number bets pay 35:1.", 85, 526, CasinoMuted);

	const SDL_FRect TablePanel{ 434.0f, 124.0f, 548.0f, 454.0f };
	DrawRoulettePanel(Context->MainRenderer, TablePanel, SDL_Color{ 24, 51, 36, 255 }, SDL_Color{ 88, 130, 89, 255 });
	RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), "CHOOSE A NUMBER OR COLOR", 468, 143,
		CasinoText);
	for (unsigned int Number = 0; Number <= 36; ++Number)
		DrawNumberButton(*this, Context, Number, Screen->IO.MousePosition,
			Screen->SelectedNumber == static_cast<int>(Number),
			Screen->HasResult && !Screen->IsSpinning && Screen->RouletteNumber == Number, !Screen->IsSpinning);

	DrawCasinoButton(*this, Context, "RED  -  1:1", SDL_FRect{ 514.0f, 352.0f, 208.0f, 46.0f },
		Screen->IO.MousePosition, Screen->SelectedNumber < 0 && Screen->SelectedColor == 1, !Screen->IsSpinning);
	DrawCasinoButton(*this, Context, "BLACK  -  1:1", SDL_FRect{ 736.0f, 352.0f, 219.0f, 46.0f },
		Screen->IO.MousePosition, Screen->SelectedNumber < 0 && Screen->SelectedColor == 2, !Screen->IsSpinning);

	const std::string BetDescription = Screen->SelectedNumber >= 0
		? "STRAIGHT UP  #" + std::to_string(Screen->SelectedNumber) + "  -  35:1"
		: "COLOR BET  " + std::string(GetColorName(Screen->SelectedColor)) + "  -  1:1";
	RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), BetDescription, 514, 403, CasinoMuted);
	DrawCasinoButton(*this, Context, "$25", SDL_FRect{ 514.0f, 420.0f, 88.0f, 48.0f },
		Screen->IO.MousePosition, Screen->BetStake == 25, !Screen->IsSpinning);
	DrawCasinoButton(*this, Context, "$50", SDL_FRect{ 612.0f, 420.0f, 88.0f, 48.0f },
		Screen->IO.MousePosition, Screen->BetStake == 50, !Screen->IsSpinning);
	DrawCasinoButton(*this, Context, "$100", SDL_FRect{ 710.0f, 420.0f, 88.0f, 48.0f },
		Screen->IO.MousePosition, Screen->BetStake == 100, !Screen->IsSpinning);
	DrawCasinoButton(*this, Context, Screen->IsSpinning ? "SPINNING..." : "SPIN",
		SDL_FRect{ 810.0f, 420.0f, 145.0f, 48.0f }, Screen->IO.MousePosition, false, !Screen->IsSpinning);

	const SDL_Color ResultFill = Screen->IsSpinning ? SDL_Color{ 31, 57, 39, 255 } :
		Screen->LastSpinWon ? SDL_Color{ 52, 83, 47, 255 } : SDL_Color{ 31, 57, 39, 255 };
	const SDL_FRect ResultPanel{ 468.0f, 486.0f, 487.0f, 52.0f };
	DrawRoulettePanel(Context->MainRenderer, ResultPanel, ResultFill,
		Screen->LastSpinWon && !Screen->IsSpinning ? SDL_Color{ 115, 180, 103, 255 } : SDL_Color{ 88, 130, 89, 255 });
	RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), Screen->ResultMessage,
		480, 504, Screen->LastSpinWon && !Screen->IsSpinning ? SDL_Color{ 201, 242, 170, 255 } : CasinoText);
	RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont),
		"Pick a color for even money, or one number for a 35:1 payout.", 468, 551, CasinoMuted);

	DrawCasinoButton(*this, Context, Screen->BackButton.Text,
		SDL_FRect{ static_cast<float>(Screen->BackButton.x), static_cast<float>(Screen->BackButton.y),
			static_cast<float>(Screen->BackButton.w), static_cast<float>(Screen->BackButton.h) },
		Screen->IO.MousePosition, false);

	Display();
	return SDL_GetTicks() - StartTime;
}