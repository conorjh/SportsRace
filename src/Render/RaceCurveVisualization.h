#ifndef RACECURVEVISUALIZATION_H
#define RACECURVEVISUALIZATION_H

#include "Renderer.h"
#include "..\Race.h"

namespace Game
{
	namespace Render
	{
		constexpr unsigned int RaceCurveVisualizationSampleCount = 48;
		constexpr SDL_Color RaceAccelerationCurveColor{ 255, 112, 64, 255 };
		constexpr SDL_Color RaceEnduranceCurveColor{ 64, 192, 255, 255 };

		inline void DrawRaceCurve(SDL_Renderer* Renderer, const Game::Race::RaceCurve& Curve,
			const SDL_FRect& PlotBounds, SDL_Color Color)
		{
			SDL_SetRenderDrawColor(Renderer, Color.r, Color.g, Color.b, Color.a);

			const auto PlotY = [&PlotBounds](double Value)
			{
				return PlotBounds.y + static_cast<float>((1.0 - Value) * PlotBounds.h);
			};

			float PreviousX = PlotBounds.x;
			float PreviousY = PlotY(Curve.Evaluate(0.0));
			for (unsigned int Sample = 1; Sample <= RaceCurveVisualizationSampleCount; ++Sample)
			{
				const double Time = static_cast<double>(Sample) / RaceCurveVisualizationSampleCount;
				const float X = PlotBounds.x + static_cast<float>(Time) * PlotBounds.w;
				const float Y = PlotY(Curve.Evaluate(Time));
				SDL_RenderLine(Renderer, PreviousX, PreviousY, X, Y);
				PreviousX = X;
				PreviousY = Y;
			}
		}

		inline void DrawRacerCurveVisualization(BaseRenderer& Renderer, AppRenderContext* Context,
			const Game::Race::Racer& Runner, const SDL_FRect& PlotBounds,
			int TitleLeft, int TitleTop, int LegendTop)
		{
			Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont),
				"Spawned curves (normalized)", TitleLeft, TitleTop, { 255, 255, 255, 255 });

			SDL_SetRenderDrawColor(Context->MainRenderer, 24, 24, 24, 255);
			SDL_RenderFillRect(Context->MainRenderer, &PlotBounds);
			SDL_SetRenderDrawColor(Context->MainRenderer, 150, 150, 150, 255);
			SDL_RenderRect(Context->MainRenderer, &PlotBounds);
			for (unsigned int Division = 1; Division < 4; ++Division)
			{
				const float Fraction = static_cast<float>(Division) / 4.0f;
				const float GridX = PlotBounds.x + PlotBounds.w * Fraction;
				const float GridY = PlotBounds.y + PlotBounds.h * Fraction;
				SDL_RenderLine(Context->MainRenderer, GridX, PlotBounds.y, GridX, PlotBounds.y + PlotBounds.h);
				SDL_RenderLine(Context->MainRenderer, PlotBounds.x, GridY, PlotBounds.x + PlotBounds.w, GridY);
			}

			DrawRaceCurve(Context->MainRenderer, Runner.AccelerationCurve, PlotBounds, RaceAccelerationCurveColor);
			DrawRaceCurve(Context->MainRenderer, Runner.EnduranceCurve, PlotBounds, RaceEnduranceCurveColor);
			Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), "Acceleration",
				TitleLeft, LegendTop, RaceAccelerationCurveColor);
			Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), "Endurance",
				TitleLeft + 150, LegendTop, RaceEnduranceCurveColor);
		}
	}
}

#endif
