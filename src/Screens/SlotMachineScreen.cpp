#include "SlotMachineScreen.h"
#include "..\App\App.h"
#include "..\Audio.h"
#include "SDL3\SDL.h"
#include <algorithm>
#include <array>
#include <random>

namespace SlotMachineScreenDetail
{
	constexpr std::array<const char*, 6> SlotSymbolNames{
		"HORSE", "STAR", "CROWN", "7", "COIN", "DIAMOND"
	};
	constexpr std::array<SDL_Color, 6> SlotSymbolColors{{
		{ 225, 160, 76, 255 }, { 113, 193, 221, 255 }, { 238, 195, 83, 255 },
		{ 231, 82, 72, 255 }, { 242, 216, 112, 255 }, { 151, 196, 231, 255 }
	}};
	constexpr SDL_Color CasinoText{ 244, 231, 199, 255 };
	constexpr SDL_Color CasinoMuted{ 178, 162, 132, 255 };
	constexpr SDL_Color CasinoGold{ 224, 180, 90, 255 };

	void DrawPanel(SDL_Renderer* Renderer, const SDL_FRect& Bounds, SDL_Color Fill, SDL_Color Border)
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
		const SDL_Color Fill = !Enabled ? SDL_Color{ 55, 52, 48, 255 } : Selected
			? SDL_Color{ 133, 87, 43, 255 } : Hovered ? SDL_Color{ 123, 67, 45, 255 }
				: SDL_Color{ 78, 45, 40, 255 };
		DrawPanel(Context->MainRenderer, Bounds, Fill, Selected ? CasinoGold : SDL_Color{ 155, 116, 67, 255 });
		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), Text,
			static_cast<int>(Bounds.x) + 10, static_cast<int>(Bounds.y + (Bounds.h - 16.0f) / 2.0f),
			Enabled ? CasinoText : CasinoMuted);
	}

	void DrawSlotSymbol(Game::Render::BaseRenderer& Renderer, Game::Render::AppRenderContext* Context,
		unsigned int Symbol, const SDL_FRect& Bounds, bool IsPayline)
	{
		const SDL_Color Fill = IsPayline ? SDL_Color{ 35, 29, 28, 255 } : SDL_Color{ 27, 24, 25, 255 };
		DrawPanel(Context->MainRenderer, Bounds, Fill, IsPayline ? CasinoGold : SDL_Color{ 86, 73, 62, 255 });
		const char* Name = SlotSymbolNames[Symbol % SlotSymbolNames.size()];
		const int TextWidth = static_cast<int>(std::char_traits<char>::length(Name)) * 8;
		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), Name,
			static_cast<int>(Bounds.x + (Bounds.w - static_cast<float>(TextWidth)) / 2.0f),
			static_cast<int>(Bounds.y + (Bounds.h - 16.0f) / 2.0f),
			IsPayline ? SlotSymbolColors[Symbol % SlotSymbolColors.size()] : CasinoMuted);
	}
}
using namespace SlotMachineScreenDetail;

Game::Screens::SlotMachineScreen::SlotMachineScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO,
	App::AppData& _Data, const Career::CityPlot& _Plot)
	: BuildingScreen(_Machine, _IO, _Data, AppScreenType::SlotMachineScreen, _Plot),
	Random(std::random_device{}())
{
	BackButton.Text = "Back to bookie";
}

void Game::Screens::SlotMachineScreen::UpdateActivity()
{
	if (IsSpinning)
	{
		if (SDL_GetTicks() - SpinStartedAt >= SpinDuration)
		{
			IsSpinning = false;
			ResultMessage = PendingMessage;
		}
		return;
	}

	if (Clicked(248.0f, 590.0f, 94.0f, 48.0f))
		BetStake = 25;
	else if (Clicked(350.0f, 590.0f, 94.0f, 48.0f))
		BetStake = 50;
	else if (Clicked(452.0f, 590.0f, 94.0f, 48.0f))
		BetStake = 100;
	else if (Clicked(650.0f, 585.0f, 208.0f, 58.0f))
		Spin();
}

void Game::Screens::SlotMachineScreen::Spin()
{
	if (Data.Profile == nullptr || Data.Profile->Financials.Cash < static_cast<int>(BetStake))
	{
		ResultMessage = "Not enough cash for that stake.";
		LastSpinWon = false;
		return;
	}

	Data.Profile->Financials.Cash -= static_cast<int>(BetStake);
	std::uniform_int_distribution<unsigned int> SymbolDistribution(0, 5);
	for (unsigned int& Symbol : Symbols)
		Symbol = SymbolDistribution(Random);

	const bool ThreeOfAKind = Symbols[0] == Symbols[1] && Symbols[1] == Symbols[2];
	const bool Pair = Symbols[0] == Symbols[1] || Symbols[0] == Symbols[2] || Symbols[1] == Symbols[2];
	PendingPayout = ThreeOfAKind ? BetStake * 10 : Pair ? BetStake * 2 : 0;
	Data.Profile->Financials.Cash += static_cast<int>(PendingPayout);
	LastSpinWon = PendingPayout > 0;
	if (ThreeOfAKind)
		PendingMessage = "JACKPOT! Three of a kind returns $" + std::to_string(PendingPayout) + ".";
	else if (Pair)
		PendingMessage = "Nice hit! A pair returns $" + std::to_string(PendingPayout) + ".";
	else
		PendingMessage = "No match this spin. Try again.";

	SpinStartedAt = SDL_GetTicks();
	IsSpinning = true;
	ResultMessage = "Reels in motion. Good luck!";
	IO.Player.Play(Audio::BuiltInSounds::Click);
}

Game::Render::SlotMachineScreenRenderer::SlotMachineScreenRenderer(AppRenderContext* Context)
	: BaseRenderer(Context)
{
}

unsigned int Game::Render::SlotMachineScreenRenderer::Render()
{
	const unsigned int StartTime = SDL_GetTicks();
	SDL_SetRenderDrawColor(Context->MainRenderer, 22, 16, 22, 255);
	SDL_RenderClear(Context->MainRenderer);
	if (Screen == nullptr)
	{
		Display();
		return SDL_GetTicks() - StartTime;
	}

	RenderText(Context->Store.GetFont(Game::Assets::FontAssetMainFont), "LUCKY TRACK SLOTS", 52, 32, CasinoGold);
	RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont),
		"Six symbols. Three reels. One very lucky spin.", 54, 60, CasinoMuted);

	const SDL_FRect CashPanel{ 790.0f, 28.0f, 186.0f, 48.0f };
	DrawPanel(Context->MainRenderer, CashPanel, SDL_Color{ 39, 31, 26, 255 }, CasinoGold);
	const int Cash = Screen->Data.Profile == nullptr ? 0 : Screen->Data.Profile->Financials.Cash;
	RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), "CASH  $" + std::to_string(Cash),
		808, 44, CasinoText);

	const SDL_FRect Machine{ 150.0f, 116.0f, 724.0f, 442.0f };
	DrawPanel(Context->MainRenderer, Machine, SDL_Color{ 67, 34, 37, 255 }, CasinoGold);
	for (unsigned int Index = 0; Index < 16; ++Index)
	{
		const bool Lit = Screen->IsSpinning || ((Index + SDL_GetTicks() / 120) % 2 == 0);
		const SDL_Color Light = Lit ? (Index % 2 == 0 ? SDL_Color{ 247, 203, 103, 255 }
			: SDL_Color{ 229, 104, 67, 255 }) : SDL_Color{ 100, 64, 47, 255 };
		const SDL_FRect TopLight{ 170.0f + static_cast<float>(Index) * 44.0f, 119.0f, 10.0f, 8.0f };
		const SDL_FRect BottomLight{ 170.0f + static_cast<float>(Index) * 44.0f, 548.0f, 10.0f, 8.0f };
		SDL_SetRenderDrawColor(Context->MainRenderer, Light.r, Light.g, Light.b, Light.a);
		SDL_RenderFillRect(Context->MainRenderer, &TopLight);
		SDL_RenderFillRect(Context->MainRenderer, &BottomLight);
	}

	const SDL_FRect Marquee{ 192.0f, 140.0f, 640.0f, 66.0f };
	DrawPanel(Context->MainRenderer, Marquee, SDL_Color{ 115, 43, 38, 255 }, CasinoGold);
	RenderText(Context->Store.GetFont(Game::Assets::FontAssetMainFont), "THE WINNER'S CIRCLE", 402, 163,
		SDL_Color{ 255, 222, 125, 255 });

	const SDL_FRect ReelFrame{ 232.0f, 218.0f, 560.0f, 214.0f };
	DrawPanel(Context->MainRenderer, ReelFrame, SDL_Color{ 31, 25, 26, 255 }, SDL_Color{ 155, 116, 67, 255 });
	const unsigned int Elapsed = SDL_GetTicks() - Screen->SpinStartedAt;
	for (std::size_t Index = 0; Index < Screen->Symbols.size(); ++Index)
	{
		const SDL_FRect Reel{ 262.0f + static_cast<float>(Index) * 168.0f, 230.0f, 150.0f, 190.0f };
		DrawPanel(Context->MainRenderer, Reel, SDL_Color{ 45, 37, 34, 255 }, SDL_Color{ 117, 92, 63, 255 });
		const bool ReelStopped = !Screen->IsSpinning || Elapsed >= 880 + static_cast<unsigned int>(Index) * 460;
		const unsigned int CurrentSymbol = ReelStopped ? Screen->Symbols[Index]
			: static_cast<unsigned int>((Elapsed / (48 + static_cast<unsigned int>(Index) * 8) + Index * 2) %
				SlotSymbolNames.size());
		for (unsigned int Row = 0; Row < 3; ++Row)
		{
			const int Offset = static_cast<int>(Row) - 1;
			const unsigned int Symbol = static_cast<unsigned int>((CurrentSymbol + SlotSymbolNames.size() + Offset) %
				SlotSymbolNames.size());
			const SDL_FRect SymbolBounds{ Reel.x + 8.0f, Reel.y + 10.0f + static_cast<float>(Row) * 56.0f,
				Reel.w - 16.0f, 50.0f };
			DrawSlotSymbol(*this, Context, Symbol, SymbolBounds, Row == 1);
		}
	}

	SDL_SetRenderDrawColor(Context->MainRenderer, CasinoGold.r, CasinoGold.g, CasinoGold.b, CasinoGold.a);
	SDL_RenderLine(Context->MainRenderer, 253.0f, 326.0f, 770.0f, 326.0f);
	SDL_RenderLine(Context->MainRenderer, 832.0f, 255.0f, 832.0f, 353.0f);
	const SDL_FRect LeverKnob{ 822.0f, Screen->IsSpinning ? 345.0f : 267.0f, 20.0f, 20.0f };
	DrawPanel(Context->MainRenderer, LeverKnob, SDL_Color{ 191, 62, 50, 255 }, CasinoGold);

	const SDL_Color ResultFill = Screen->IsSpinning ? SDL_Color{ 44, 37, 30, 255 } :
		Screen->LastSpinWon ? SDL_Color{ 49, 73, 47, 255 } : SDL_Color{ 44, 37, 30, 255 };
	const SDL_FRect ResultPanel{ 214.0f, 450.0f, 596.0f, 52.0f };
	DrawPanel(Context->MainRenderer, ResultPanel, ResultFill,
		Screen->LastSpinWon && !Screen->IsSpinning ? SDL_Color{ 115, 180, 103, 255 } : CasinoGold);
	RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), Screen->ResultMessage,
		229, 468, Screen->LastSpinWon && !Screen->IsSpinning ? SDL_Color{ 201, 242, 170, 255 } : CasinoText);
	RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont),
		"PAIR RETURNS 2X     THREE OF A KIND RETURNS 10X", 326, 521, CasinoMuted);

	RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), "WAGER", 248, 565, CasinoMuted);
	DrawCasinoButton(*this, Context, "$25", SDL_FRect{ 248.0f, 590.0f, 94.0f, 48.0f },
		Screen->IO.MousePosition, Screen->BetStake == 25, !Screen->IsSpinning);
	DrawCasinoButton(*this, Context, "$50", SDL_FRect{ 350.0f, 590.0f, 94.0f, 48.0f },
		Screen->IO.MousePosition, Screen->BetStake == 50, !Screen->IsSpinning);
	DrawCasinoButton(*this, Context, "$100", SDL_FRect{ 452.0f, 590.0f, 94.0f, 48.0f },
		Screen->IO.MousePosition, Screen->BetStake == 100, !Screen->IsSpinning);
	DrawCasinoButton(*this, Context, Screen->IsSpinning ? "SPINNING..." : "SPIN THE REELS",
		SDL_FRect{ 650.0f, 585.0f, 208.0f, 58.0f }, Screen->IO.MousePosition, false, !Screen->IsSpinning);

	RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont),
		"Winnings are returned to your career cash immediately.", 54, 665, CasinoMuted);
	DrawCasinoButton(*this, Context, Screen->BackButton.Text,
		SDL_FRect{ static_cast<float>(Screen->BackButton.x), static_cast<float>(Screen->BackButton.y),
			static_cast<float>(Screen->BackButton.w), static_cast<float>(Screen->BackButton.h) },
		Screen->IO.MousePosition, false);

	Display();
	return SDL_GetTicks() - StartTime;
}