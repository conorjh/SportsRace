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

		struct RaceCurveVisualizationStyle
		{
			SDL_Color TitleColor;
			SDL_Color PlotBackgroundColor;
			SDL_Color PlotBorderColor;
			SDL_Color GridColor;
			SDL_Color AccelerationColor;
			SDL_Color EnduranceColor;
		};

		constexpr RaceCurveVisualizationStyle DefaultRaceCurveVisualizationStyle{
			{ 255, 255, 255, 255 }, { 24, 24, 24, 255 }, { 150, 150, 150, 255 },
			{ 70, 70, 70, 255 }, RaceAccelerationCurveColor, RaceEnduranceCurveColor
		};

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
			// Approximate the generated curve with connected samples for reuse in both profile views.
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
			int TitleLeft, int TitleTop, int LegendTop,
			const RaceCurveVisualizationStyle& Style = DefaultRaceCurveVisualizationStyle)
		{
			Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont),
				"Spawned curves (normalized)", TitleLeft, TitleTop, Style.TitleColor);

			SDL_SetRenderDrawColor(Context->MainRenderer, Style.PlotBackgroundColor.r, Style.PlotBackgroundColor.g,
				Style.PlotBackgroundColor.b, Style.PlotBackgroundColor.a);
			SDL_RenderFillRect(Context->MainRenderer, &PlotBounds);
			SDL_SetRenderDrawColor(Context->MainRenderer, Style.PlotBorderColor.r, Style.PlotBorderColor.g,
				Style.PlotBorderColor.b, Style.PlotBorderColor.a);
			SDL_RenderRect(Context->MainRenderer, &PlotBounds);
			SDL_SetRenderDrawColor(Context->MainRenderer, Style.GridColor.r, Style.GridColor.g,
				Style.GridColor.b, Style.GridColor.a);
			for (unsigned int Division = 1; Division < 4; ++Division)
			{
				const float Fraction = static_cast<float>(Division) / 4.0f;
				const float GridX = PlotBounds.x + PlotBounds.w * Fraction;
				const float GridY = PlotBounds.y + PlotBounds.h * Fraction;
				SDL_RenderLine(Context->MainRenderer, GridX, PlotBounds.y, GridX, PlotBounds.y + PlotBounds.h);
				SDL_RenderLine(Context->MainRenderer, PlotBounds.x, GridY, PlotBounds.x + PlotBounds.w, GridY);
			}

			// Share a normalized 0-1 scale so the acceleration and endurance shapes remain comparable.
			DrawRaceCurve(Context->MainRenderer, Runner.AccelerationCurve, PlotBounds, Style.AccelerationColor);
			DrawRaceCurve(Context->MainRenderer, Runner.EnduranceCurve, PlotBounds, Style.EnduranceColor);
			Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), "Acceleration",
				TitleLeft, LegendTop, Style.AccelerationColor);
			Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), "Endurance",
				TitleLeft + 150, LegendTop, Style.EnduranceColor);
		}
	}
}

#endif
