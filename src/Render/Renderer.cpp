#include "AppRenderer.h"
#include "..\Util.h"

#include "SDL3_image/SDL_image.h"
#include "spdlog/spdlog.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <array>
#include <cmath>
#include <iostream>

namespace RendererDetail
{
	constexpr double MillisecondsPerSecond = 1000.0;
	constexpr int RacerNameClickPadding = 2;

	bool IsRacerNameBoundary(char Character)
	{
		return !((Character >= 'a' && Character <= 'z') || (Character >= 'A' && Character <= 'Z') ||
			(Character >= '0' && Character <= '9') || Character == '_');
	}

	void RegisterRacerNameClickTargets(TTF_Font* Font, const std::string& Text, int X, int Y,
		Game::Render::AppRenderContext* Context)
	{
		if (Context == nullptr || !Context->RacerInfoLinksEnabled || Font == nullptr)
			return;

		std::vector<bool> ClaimedCharacters(Text.size(), false);
		for (const Game::Render::AppRenderContext::RacerInfoCandidate& Candidate :
			Context->RacerInfoCandidates)
		{
			if (Candidate.Runner == nullptr || Candidate.Name.empty() || Candidate.Name.size() > Text.size())
				continue;

			std::size_t SearchPosition = 0;
			while ((SearchPosition = Text.find(Candidate.Name, SearchPosition)) != std::string::npos)
			{
				const std::size_t MatchEnd = SearchPosition + Candidate.Name.size();
				const bool HasLeftBoundary = SearchPosition == 0 || IsRacerNameBoundary(Text[SearchPosition - 1]);
				const bool HasRightBoundary = MatchEnd == Text.size() || IsRacerNameBoundary(Text[MatchEnd]);
				const bool OverlapsExistingTarget = std::any_of(ClaimedCharacters.begin() + SearchPosition,
					ClaimedCharacters.begin() + MatchEnd, [](bool Claimed) { return Claimed; });
				if (HasLeftBoundary && HasRightBoundary && !OverlapsExistingTarget)
				{
					int PrefixWidth = 0;
					int PrefixHeight = 0;
					int NameWidth = 0;
					int NameHeight = 0;
					const bool PrefixMeasured = SearchPosition == 0 || TTF_GetStringSize(Font, Text.c_str(),
						SearchPosition, &PrefixWidth, &PrefixHeight);
					if (PrefixMeasured && TTF_GetStringSize(Font, Candidate.Name.c_str(),
						Candidate.Name.size(), &NameWidth, &NameHeight) && NameWidth > 0 && NameHeight > 0)
					{
						Context->RacerInfoClickTargets.push_back({
							{ X + PrefixWidth - RacerNameClickPadding, Y - RacerNameClickPadding,
								NameWidth + RacerNameClickPadding * 2, NameHeight + RacerNameClickPadding * 2 },
							Candidate.Runner
						});
						std::fill(ClaimedCharacters.begin() + SearchPosition,
							ClaimedCharacters.begin() + MatchEnd, true);
					}
				}
				SearchPosition = MatchEnd;
			}
		}
	}
}
using namespace RendererDetail;

bool Game::Assets::AssetStore::RegisterAll(Game::Render::BaseRenderer& Renderer)
{
	using namespace Game::Assets;

	RegisterFont(FontAssetBigFont, TTF_OpenFont("media/menu_font.ttf", 96));
	RegisterFont(FontAssetMainFont, TTF_OpenFont("media/PIXELLARI.ttf", 16));
	RegisterFont(FontAssetInfoFont, TTF_OpenFont("media/OpenSans.ttf", 14));
	RegisterFont(FontAssetDebugFont, TTF_OpenFont("media/OpenSans.ttf", 12));
	RegisterFont(FontAssetWinningFont, TTF_OpenFont("media/menu_font.ttf", 16));
	RegisterFont(FontAssetMainMenuFont, TTF_OpenFont("media/menu_font.ttf", 96));

	RegisterImage(ImageAssetMainMenuBackground, Renderer.LoadImageResource("media/screen.png"));
	RegisterImage(ImageAssetRaceBackground, Renderer.LoadImageResource("media/screen.png"));
	RegisterImage(ImageAssetRaceIcon, Renderer.LoadImageResource("media/race_icon.png"));
	RegisterImage(ImageAssetRacerIcon, Renderer.LoadImageResource("media/racer_icon.png"));
	RegisterImage(ImageAssetTrainingIcon, Renderer.LoadImageResource("media/training_icon.png"));
	RegisterImage(ImageAssetRankingIcon, Renderer.LoadImageResource("media/ranking_icon.png"));
	RegisterImage(ImageAssetHead, Renderer.LoadImageResource("media/reido.png"));
	RegisterImage(ImageAssetTrack, Renderer.LoadImageResource("media/track.png"));
	RegisterImage(ImageAssetStadium, Renderer.LoadImageResource("media/stadium.png"));
	RegisterImage(ImageAssetStartingBlock, Renderer.LoadImageResource("media/startingblock.png"));
	RegisterImage(ImageAssetClouds, Renderer.LoadImageResource("media/clouds.png"));
	RegisterImage(ImageAssetMountains, Renderer.LoadImageResource("media/mountains.png"));
	RegisterImage(ImageAssetFellaRun, Renderer.LoadImageResource("media/fellarun.png"));
	RegisterImage(ImageAssetFellaStumble, Renderer.LoadImageResource("media/fellastumble.png"));
	RegisterImage(ImageAssetFellaWait, Renderer.LoadImageResource("media/fellawait.png"));
	RegisterImage(ImageAssetScreen, Renderer.LoadImageResource("media/screen.png"));
	RegisterImage(ImageAssetWorldMap, Renderer.LoadImageResource("media/city/sportsrace-borough-map.png"));
	for (unsigned int Index = 0; Index < 12; ++Index)
	{
		std::string Filename = "media/city/cityTiles_";
		Filename += Index < 10 ? "00" : "0";
		Filename += std::to_string(Index) + ".png";
		RegisterImage(static_cast<ImageAssetId>(ImageAssetCityTile000 + Index), Renderer.LoadImageResource(Filename));
	}

	for (unsigned int Index = 0; Index < 12; ++Index)
	{
		std::string Filename = "media/city/isometric/city/cityTiles_";
		Filename += Index < 10 ? "00" : "0";
		Filename += std::to_string(Index) + ".png";
		RegisterImage(static_cast<ImageAssetId>(ImageAssetIsometricCity000 + Index),
			Renderer.LoadImageResource(Filename));
	}

	const std::array<std::string, 12> IsometricBuildingFiles{
		"buildingTiles_001.png", "buildingTiles_002.png", "buildingTiles_003.png",
		"buildingTiles_004.png", "buildingTiles_009.png", "buildingTiles_010.png",
		"buildingTiles_011.png", "buildingTiles_014.png", "buildingTiles_017.png",
		"buildingTiles_018.png", "buildingTiles_019.png", "buildingTiles_020.png"
	};
	for (std::size_t Index = 0; Index < IsometricBuildingFiles.size(); ++Index)
		RegisterImage(static_cast<ImageAssetId>(ImageAssetIsometricBuilding000 + Index),
			Renderer.LoadImageResource("media/city/isometric/buildings/" + IsometricBuildingFiles[Index]));

	const std::array<std::string, 5> IsometricVehicleFiles{
		"carBlack2_000.png", "carBlue2_000.png", "carGreen2_000.png",
		"carRed2_000.png", "carSilver2_000.png"
	};
	for (std::size_t Index = 0; Index < IsometricVehicleFiles.size(); ++Index)
		RegisterImage(static_cast<ImageAssetId>(ImageAssetIsometricVehicleBlack + Index),
			Renderer.LoadImageResource("media/city/isometric/vehicles/" + IsometricVehicleFiles[Index]));

	return true;
}

void Game::Assets::AssetStore::RegisterFont(Game::Assets::FontAssetId FontId, TTF_Font* LoadedFont)
{
	Fonts.insert(std::make_pair(FontId, LoadedFont));
}

void Game::Assets::AssetStore::RegisterImage(Game::Assets::ImageAssetId ImageId, Game::Render::Image* LoadedImage)
{
	Images.insert(std::make_pair(ImageId, LoadedImage));
}

TTF_Font* Game::Assets::AssetStore::GetFont(Game::Assets::FontAssetId Id)
{
	return Fonts[Id];
}

Game::Render::Image* Game::Assets::AssetStore::GetImage(Game::Assets::ImageAssetId Id)
{
	return Images[Id];
}

Game::Render::BaseRenderer::BaseRenderer(AppRenderContext* Context)
	:LastFrameEnd()
	, HasLastFrameEnd(false)
	, Context(Context)
{
}

Game::Render::Image* Game::Render::BaseRenderer::LoadImageResource(const std::string& ResourceName)
{
	using namespace	Game::Render;

	spdlog::trace("Loading image resource: {}", ResourceName);
	const HMODULE Module = GetModuleHandleW(nullptr);
	// RC preserves the quotes around string identifiers, so include them in the lookup.
	const std::string ResourceIdentifier = "\"" + ResourceName + "\"";
	const HRSRC Resource = FindResourceA(Module, ResourceIdentifier.c_str(), MAKEINTRESOURCEA(10));
	if (Resource == nullptr)
	{
		spdlog::error("Failed to find embedded image resource '{}'.", ResourceName);
		return nullptr;
	}

	const DWORD ResourceSize = SizeofResource(Module, Resource);
	const HGLOBAL ResourceData = LoadResource(Module, Resource);
	if (ResourceSize == 0 || ResourceData == nullptr)
	{
		spdlog::error("Failed to load embedded image resource '{}'.", ResourceName);
		return nullptr;
	}

	const void* ResourceBytes = LockResource(ResourceData);
	if (ResourceBytes == nullptr)
	{
		spdlog::error("Failed to access embedded image resource '{}'.", ResourceName);
		return nullptr;
	}

	SDL_IOStream* ImageStream = SDL_IOFromConstMem(ResourceBytes, static_cast<std::size_t>(ResourceSize));
	if (ImageStream == nullptr)
	{
		spdlog::error("Failed to open embedded image resource '{}': {}", ResourceName, SDL_GetError());
		return nullptr;
	}

	Image* Img = new Image{};
	if ((Img->Surface = IMG_Load_IO(ImageStream, true)) == nullptr)
	{
		spdlog::error("Failed to decode embedded image '{}': {}", ResourceName, SDL_GetError());
		delete Img;
		return nullptr;
	}

	if ((Img->Texture = SDL_CreateTextureFromSurface(Context->MainRenderer, Img->Surface)) == nullptr)
	{
		spdlog::error("Failed to create texture for embedded image '{}': {}", ResourceName, SDL_GetError());
		SDL_DestroySurface(Img->Surface);
		delete Img;
		return nullptr;
	}
	return Img;
}

bool Game::Render::BaseRenderer::LoadFontFile(std::string File, unsigned int Size, TTF_Font* Font)
{
	if ((Font = TTF_OpenFont(File.c_str(), Size)) == nullptr)
	{
		spdlog::error("Failed to load font " + File + ": " + SDL_GetError());
		return false;
	}
	return true;
}

void Game::Render::BaseRenderer::RenderTextSingleLine(TTF_Font* Font, std::string Text, int x, int y, SDL_Color Color)
{
	SDL_Surface* surfaceMessage = TTF_RenderText_Blended(Font, Text.c_str(), Text.size(), Color);
	if (surfaceMessage == nullptr)
	{
		spdlog::error("Failed to render text: {}", SDL_GetError());
		return;
	}
	SDL_Texture* Message = SDL_CreateTextureFromSurface(Context->MainRenderer, surfaceMessage);
	if (Message == nullptr)
	{
		spdlog::error("Failed to create a texture for rendered text: {}", SDL_GetError());
		SDL_DestroySurface(surfaceMessage);
		return;
	}
	
	SDL_FRect Message_rect; //create a rect
	Message_rect.x = x;  //controls the rect's x coordinate 
	Message_rect.y = y; // controls the rect's y coordinte
	SDL_FPoint size; SDL_GetTextureSize(Message, &size.x, &size.y);
	Message_rect.w = size.x; // controls the width of the rect
	Message_rect.h = size.y; // controls the height of the rect
	RegisterRacerNameClickTargets(Font, Text, x, y, Context);
	SDL_RenderTexture(Context->MainRenderer, Message, NULL, &Message_rect);

	// Don't forget to free your surface and texture
	SDL_DestroySurface(surfaceMessage);
	SDL_DestroyTexture(Message);
}

void Game::Render::BaseRenderer::RenderText(TTF_Font* Font, std::string Text, int x, int y, SDL_Color Color)
{
	// SDL renders one line at a time, so draw each newline-delimited segment with a vertical offset.
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
	if (Context->ShowDebugMenu)
	{
		const SDL_FRect Backdrop{ 0.0f, 0.0f, static_cast<float>(Context->ScreenWidth),
			static_cast<float>(Context->ScreenHeight) };
		SDL_SetRenderDrawColor(Context->MainRenderer, 0, 0, 0, 170);
		SDL_RenderFillRect(Context->MainRenderer, &Backdrop);

		const SDL_FRect Panel{ 272.0f, 230.0f, 480.0f, 310.0f };
		SDL_SetRenderDrawColor(Context->MainRenderer, 30, 28, 25, 248);
		SDL_RenderFillRect(Context->MainRenderer, &Panel);
		SDL_SetRenderDrawColor(Context->MainRenderer, 170, 96, 72, 255);
		SDL_RenderRect(Context->MainRenderer, &Panel);

		RenderText(Context->Store.GetFont(Game::Assets::FontAssetMainFont), "CAREER DEBUG MENU",
			312, 264, SDL_Color{ 218, 112, 82, 255 });
		RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont),
			Context->DebugMenuStatus.empty() ? "Press 1 or click to spawn five AI racers." : Context->DebugMenuStatus,
			312, 310, SDL_Color{ 220, 214, 201, 255 });

		SDL_FRect SpawnButtonBounds;
		SDL_RectToFRect(&Game::App::DebugMenuSpawnRacersButtonBounds, &SpawnButtonBounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, 104, 42, 36, 255);
		SDL_RenderFillRect(Context->MainRenderer, &SpawnButtonBounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, 170, 96, 72, 255);
		SDL_RenderRect(Context->MainRenderer, &SpawnButtonBounds);
		RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont),
			"SPAWN " + std::to_string(Game::App::DebugMenuSpawnRacerCount) + " AI RACERS",
			370, 378, SDL_Color{ 239, 231, 214, 255 });
		SDL_FRect NodeViewerButtonBounds;
		SDL_RectToFRect(&Game::App::DebugMenuNodeViewerButtonBounds, &NodeViewerButtonBounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, 104, 42, 36, 255);
		SDL_RenderFillRect(Context->MainRenderer, &NodeViewerButtonBounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, 170, 96, 72, 255);
		SDL_RenderRect(Context->MainRenderer, &NodeViewerButtonBounds);
		RenderText(Context->Store.GetFont(Game::Assets::FontAssetInfoFont),
			std::string("NODE VIEWER: ") + (Context->NodeViewerEnabled ? "ENABLED" : "DISABLED"),
			370, 445, SDL_Color{ 239, 231, 214, 255 });
		RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), "Press F3 to close.",
			312, 500, SDL_Color{ 164, 155, 138, 255 });
	}

	SDL_RenderPresent(Context->MainRenderer);

	const auto FrameEnd = std::chrono::steady_clock::now();
	if (HasLastFrameEnd)
	{
		const double FrameTimeMs = std::chrono::duration<double, std::milli>(FrameEnd - LastFrameEnd).count();
		FPS.AddFrame(FrameTimeMs);
	}

	LastFrameEnd = FrameEnd;
	HasLastFrameEnd = true;
}

void Game::Render::BaseRenderer::RenderImage(SDL_Texture* Texture, SDL_Rect* SourceQuad, SDL_Rect* RenderQuad)
{
	SDL_FRect SourceQuadF, RenderQuadF;
	// A null source rectangle selects the full texture; a supplied rectangle supports sprite-sheet frames.
	if (SourceQuad == nullptr)
	{
		SourceQuadF.x = SourceQuadF.y = 0;
		SDL_GetTextureSize(Texture, &SourceQuadF.w, &SourceQuadF.h);
	}
	else
		SDL_RectToFRect(SourceQuad, &SourceQuadF);
	SDL_RectToFRect(RenderQuad, &RenderQuadF);
	SDL_RenderTexture(Context->MainRenderer, Texture, &SourceQuadF, &RenderQuadF);
}

void Game::Render::FPSCounter::AddFrame(double TimeTakenMs)
{
	if (!std::isfinite(TimeTakenMs) || TimeTakenMs <= 0.0)
		return;

	// Evict older samples so the FPS estimate reflects the most recent second of frame time.
	while (!FrameTimes.empty() && TotalFrameTime + TimeTakenMs > MillisecondsPerSecond)
	{
		TotalFrameTime -= FrameTimes.front();
		FrameTimes.erase(FrameTimes.begin());
	}

	FrameTimes.push_back(TimeTakenMs);
	TotalFrameTime += TimeTakenMs;
}

void Game::Render::FPSCounter::Clear()
{
	FrameTimes.clear();
	TotalFrameTime = 0.0;
}

unsigned int Game::Render::FPSCounter::GetFrameTime()
{
	if (FrameTimes.empty())
		return 0;

	return static_cast<unsigned int>(std::lround(TotalFrameTime / static_cast<double>(FrameTimes.size())));
}

unsigned int Game::Render::FPSCounter::GetFPS()
{
	if (FrameTimes.empty() || TotalFrameTime <= 0.0)
		return 0;

	const double FramesPerSecond = static_cast<double>(FrameTimes.size()) * MillisecondsPerSecond / TotalFrameTime;
	return static_cast<unsigned int>(std::lround(FramesPerSecond));
}

