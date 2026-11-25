#include "OptionsScreen.h"
#include "..\Audio.h"
#include "..\App\App.h"
#include "SDL3\SDL.h"
#include <array>

namespace OptionsScreenDetail
{
	struct WindowResolution
	{
		unsigned int Width;
		unsigned int Height;
	};

	constexpr std::array<WindowResolution, 5> OptionsResolutions{{
		{ 800, 600 }, { 1024, 768 }, { 1280, 720 }, { 1600, 900 }, { 1920, 1080 }
	}};

	std::size_t FindResolutionIndex(unsigned int Width, unsigned int Height)
	{
		for (std::size_t Index = 0; Index < OptionsResolutions.size(); ++Index)
			if (OptionsResolutions[Index].Width == Width && OptionsResolutions[Index].Height == Height)
				return Index;
		return OptionsResolutions.size();
	}

	void DrawOptionsButton(Game::Render::BaseRenderer& Renderer, Game::Render::AppRenderContext* Context,
		Game::GUI::Button& Button)
	{
		SDL_FRect Bounds;
		SDL_RectToFRect(&Button.Rect, &Bounds);
		const SDL_Color FillColor = Button.IsMouseOver() ? SDL_Color{ 134, 54, 45, 255 } :
			SDL_Color{ 104, 42, 36, 255 };
		SDL_SetRenderDrawColor(Context->MainRenderer, FillColor.r, FillColor.g, FillColor.b, FillColor.a);
		SDL_RenderFillRect(Context->MainRenderer, &Bounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, 170, 96, 72, 255);
		SDL_RenderRect(Context->MainRenderer, &Bounds);
		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), Button.Text,
			Button.x + 14, Button.y + (Button.h - 16) / 2, SDL_Color{ 239, 231, 214, 255 });
	}
}
using namespace OptionsScreenDetail;

Game::Screens::OptionsScreen::OptionsScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO,
	App::AppData& _Data)
	: AppScreen(_Machine, _IO, _Data),
	PreviousResolutionButton(_IO, "<", 252, 284, 64, 48),
	NextResolutionButton(_IO, ">", 708, 284, 64, 48),
	FullscreenButton(_IO, "Toggle fullscreen", 402, 382, 220, 52),
	BackButton(_IO, "Back", 824, 694, 160, 48)
{
	Type = AppScreenType::OptionsScreen;
	UpdateLayout();
	FullscreenButton.Text = GetFullscreenLabel();
}

void Game::Screens::OptionsScreen::Entry()
{
}

void Game::Screens::OptionsScreen::Exit()
{
}

Game::Screens::AppScreen* Game::Screens::OptionsScreen::Update()
{
	UpdateLayout();
	FullscreenButton.Text = GetFullscreenLabel();
	if (BackButton.HasMouseClicked() || IO.Esc)
	{
		IO.Player.Play(Audio::BuiltInSounds::Click);
		ScreenStack.Pop();
		return ScreenStack.Top();
	}

	PreviousResolutionButton.Update();
	NextResolutionButton.Update();
	FullscreenButton.Update();
	BackButton.Update();

	if (PreviousResolutionButton.HasMouseClicked())
		CycleResolution(-1);
	if (NextResolutionButton.HasMouseClicked())
		CycleResolution(1);
	if (FullscreenButton.HasMouseClicked())
		ToggleFullscreen();
	FullscreenButton.Text = GetFullscreenLabel();

	return this;
}

void Game::Screens::OptionsScreen::UpdateLayout()
{
	const unsigned int ScreenWidth = Data.RenderContext == nullptr ? 1024 : Data.RenderContext->ScreenWidth;
	const unsigned int ScreenHeight = Data.RenderContext == nullptr ? 768 : Data.RenderContext->ScreenHeight;
	const int PanelLeft = (static_cast<int>(ScreenWidth) - 616) / 2;
	const int PanelTop = (static_cast<int>(ScreenHeight) - 390) / 2;
	PreviousResolutionButton.SetBounds(PanelLeft + 48, PanelTop + 124, 64, 48);
	NextResolutionButton.SetBounds(PanelLeft + 504, PanelTop + 124, 64, 48);
	FullscreenButton.SetBounds(PanelLeft + 198, PanelTop + 222, 220, 52);
	BackButton.SetBounds(static_cast<int>(ScreenWidth) - 184, static_cast<int>(ScreenHeight) - 74, 160, 48);
}

void Game::Screens::OptionsScreen::CycleResolution(int Direction)
{
	const std::size_t CurrentIndex = FindResolutionIndex(Data.WindowedWidth, Data.WindowedHeight);
	const std::size_t StartingIndex = CurrentIndex == OptionsResolutions.size()
		? (Direction > 0 ? OptionsResolutions.size() - 1 : 0) : CurrentIndex;
	const std::size_t NextIndex = Direction > 0
		? (StartingIndex + 1) % OptionsResolutions.size()
		: (StartingIndex + OptionsResolutions.size() - 1) % OptionsResolutions.size();
	const WindowResolution& Resolution = OptionsResolutions[NextIndex];
	if (!Data.SetWindowedResolution(Resolution.Width, Resolution.Height))
	{
		StatusMessage = "Could not apply the requested window resolution.";
		return;
	}
	StatusMessage.clear();
}

void Game::Screens::OptionsScreen::ToggleFullscreen()
{
	if (!Data.SetFullscreen(!Data.Fullscreen))
	{
		StatusMessage = "Could not change the display mode.";
		return;
	}
	StatusMessage.clear();
}

std::string Game::Screens::OptionsScreen::GetResolutionLabel() const
{
	return std::to_string(Data.WindowedWidth) + " x " + std::to_string(Data.WindowedHeight);
}

std::string Game::Screens::OptionsScreen::GetFullscreenLabel() const
{
	return Data.Fullscreen ? "Fullscreen: on" : "Fullscreen: off";
}

const std::string& Game::Screens::OptionsScreen::GetStatusMessage() const
{
	return StatusMessage;
}

Game::Render::OptionsScreenRenderer::OptionsScreenRenderer(Game::Render::AppRenderContext* Context)
	: BaseRenderer(Context),
	Screen(nullptr)
{
}

unsigned int Game::Render::OptionsScreenRenderer::Render()
{
	const auto StartTime = SDL_GetTicks();
	const int PanelLeft = (static_cast<int>(Context->ScreenWidth) - 616) / 2;
	const int PanelTop = (static_cast<int>(Context->ScreenHeight) - 390) / 2;
	SDL_SetRenderDrawColor(Context->MainRenderer, 23, 22, 20, 255);
	SDL_RenderClear(Context->MainRenderer);
	RenderText(Context->Store.GetFont(Game::Assets::FontAssetMainFont), "OPTIONS",
		static_cast<int>(Context->ScreenWidth * 0.5f) - 50, PanelTop - 84, SDL_Color{ 178, 61, 49, 255 });

	const SDL_FRect Panel{ static_cast<float>(PanelLeft), static_cast<float>(PanelTop), 616.0f, 390.0f };
	SDL_SetRenderDrawColor(Context->MainRenderer, 34, 33, 30, 255);
	SDL_RenderFillRect(Context->MainRenderer, &Panel);
	SDL_SetRenderDrawColor(Context->MainRenderer, 75, 70, 60, 255);
	SDL_RenderRect(Context->MainRenderer, &Panel);

	RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), "WINDOWED RESOLUTION",
		PanelLeft + 48, PanelTop + 64, SDL_Color{ 235, 229, 215, 255 });
	DrawOptionsButton(*this, Context, Screen->PreviousResolutionButton);
	DrawOptionsButton(*this, Context, Screen->NextResolutionButton);
	RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), Screen->GetResolutionLabel(),
		PanelLeft + 208, PanelTop + 138, SDL_Color{ 235, 229, 215, 255 });

	RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont), "DISPLAY MODE",
		PanelLeft + 48, PanelTop + 206, SDL_Color{ 235, 229, 215, 255 });
	DrawOptionsButton(*this, Context, Screen->FullscreenButton);
	RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont),
		"Fullscreen uses the desktop mode; resolution controls the windowed size.",
		PanelLeft + 48, PanelTop + 298, SDL_Color{ 164, 155, 138, 255 });
	RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont),
		"Resize the window freely. The interface scales to fill each desktop aspect ratio.",
		PanelLeft + 48, PanelTop + 322, SDL_Color{ 164, 155, 138, 255 });
	if (!Screen->GetStatusMessage().empty())
		RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), Screen->GetStatusMessage(),
			PanelLeft + 48, PanelTop + 360, SDL_Color{ 220, 115, 93, 255 });

	DrawOptionsButton(*this, Context, Screen->BackButton);
	Display();
	return static_cast<unsigned int>(SDL_GetTicks() - StartTime);
}
