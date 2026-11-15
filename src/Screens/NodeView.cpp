#include "NodeView.h"
#include "..\Audio.h"
#include "..\App\App.h"
#include "..\Render\RaceCurveVisualization.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iterator>
#include <sstream>

namespace
{
	constexpr unsigned int NodeViewPlotSampleCount = 96;
	constexpr std::size_t NodeViewVisibleNodeCount = 10;
	constexpr int NodeViewPlotLeft = 100;
	constexpr int NodeViewPlotTop = 145;
	constexpr int NodeViewPlotWidth = 560;
	constexpr int NodeViewPlotHeight = 270;
	constexpr int NodeViewStatsLeft = 70;
	constexpr int NodeViewStatsTop = 91;
	constexpr int NodeViewStatsColumnSpacing = 190;
	constexpr int NodeViewCurvePlotLeft = 700;
	constexpr int NodeViewCurvePlotTop = 178;
	constexpr int NodeViewCurvePlotWidth = 260;
	constexpr int NodeViewCurvePlotHeight = 120;
	constexpr int NodeViewCurveTitleTop = 145;
	constexpr int NodeViewCurveLegendTop = 307;
	constexpr float NodeViewEventMarkerSize = 8.0f;
	constexpr int NodeViewNodeListTop = 493;
	constexpr int NodeViewNodeRowHeight = 17;
	constexpr SDL_Color NodeViewBackgroundColor{ 0, 0, 0, 255 };
	constexpr SDL_Color NodeViewPlotBackgroundColor{ 24, 24, 24, 255 };
	constexpr SDL_Color NodeViewGridColor{ 70, 70, 70, 255 };
	constexpr SDL_Color NodeViewBorderColor{ 150, 150, 150, 255 };
	constexpr SDL_Color NodeViewTextColor{ 255, 255, 255, 255 };
	constexpr SDL_Color NodeViewPaceColor{ 255, 112, 64, 255 };
	constexpr SDL_Color NodeViewEnduranceColor{ 64, 192, 255, 255 };
	constexpr SDL_Color NodeViewButtonFillColor{ 255, 0, 0, 255 };
	constexpr SDL_Color NodeViewButtonBorderColor{ 255, 255, 255, 255 };

	SDL_Color GetEventColor(Game::Race::RaceEventType Type)
	{
		using namespace Game::Race;

		switch (Type)
		{
		case RaceEventType::Gassing:
			return { 255, 170, 0, 255 };
		case RaceEventType::Stumble:
			return { 255, 88, 88, 255 };
		case RaceEventType::SecondWind:
			return { 80, 190, 255, 255 };
		case RaceEventType::LostBelief:
			return { 180, 110, 255, 255 };
		case RaceEventType::BelievesInSelf:
			return { 90, 220, 130, 255 };
		case RaceEventType::Injury:
			return { 255, 80, 210, 255 };
		case RaceEventType::PullingUp:
			return { 255, 64, 64, 255 };
		}
		return NodeViewTextColor;
	}

	std::string GetEventName(Game::Race::RaceEventType Type)
	{
		using namespace Game::Race;

		switch (Type)
		{
		case RaceEventType::Gassing:
			return "Gassing";
		case RaceEventType::Stumble:
			return "Stumble";
		case RaceEventType::SecondWind:
			return "Second wind";
		case RaceEventType::LostBelief:
			return "Lost belief";
		case RaceEventType::BelievesInSelf:
			return "Believes in self";
		case RaceEventType::Injury:
			return "Injury";
		case RaceEventType::PullingUp:
			return "Pulling up";
		}
		return "Unknown event";
	}

	std::string GetNodeName(const Game::Race::RaceTimelineNode& Node)
	{
		using namespace Game::Race;

		switch (Node.Type)
		{
		case RaceTimelineNodeType::InitialDelay:
			return "Start delay";
		case RaceTimelineNodeType::AccelerationToTopSpeed:
			return "Acceleration";
		case RaceTimelineNodeType::RandomEvent:
		case RaceTimelineNodeType::Gassing:
			return GetEventName(Node.Event.Type);
		case RaceTimelineNodeType::Finish:
			return "Finish";
		}
		return "Unknown node";
	}

	std::string FormatNodeDetails(const Game::Race::RaceTimelineNode& Node)
	{
		using namespace Game::Race;

		std::ostringstream Details;
		Details << std::fixed << std::setprecision(1) << Node.StartTimeMs / 1000.0 << "s  ";
		Details << GetNodeName(Node) << "  ";

		if (Node.Type == RaceTimelineNodeType::InitialDelay)
		{
			Details << "duration " << (Node.EndTimeMs - Node.StartTimeMs) / 1000.0 << "s";
		}
		else if (Node.Type == RaceTimelineNodeType::AccelerationToTopSpeed)
		{
			Details << "pace " << Node.StartPace << " -> " << Node.EndPace
				<< "  duration " << (Node.EndTimeMs - Node.StartTimeMs) / 1000.0 << "s";
		}
		else if (Node.Type == RaceTimelineNodeType::Gassing)
		{
			Details << "pace " << Node.StartPace << " -> " << Node.EndPace
				<< "  duration " << Node.Event.DurationMs / 1000.0 << "s";
		}
		else if (Node.Type == RaceTimelineNodeType::RandomEvent)
		{
			if (Node.Event.Type == RaceEventType::PullingUp)
			{
				Details << "pace " << Node.StartPace << " -> " << Node.EndPace << "  after "
					<< Node.Event.Magnitude / 1000.0 << "s gassing";
			}
			else
			{
				Details << "pace " << Node.StartPace << "  duration "
					<< Node.Event.DurationMs / 1000.0 << "s  value " << Node.Event.Magnitude;
			}
		}
		else if (Node.Type == RaceTimelineNodeType::Finish)
		{
			Details << "pace " << Node.StartPace;
		}

		return Details.str();
	}

	void DrawNodeViewButton(Game::Render::BaseRenderer& Renderer, Game::Render::AppRenderContext* Context,
		const Game::GUI::Button& Button)
	{
		SDL_FRect Bounds;
		SDL_RectToFRect(&Button.Rect, &Bounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, NodeViewButtonFillColor.r, NodeViewButtonFillColor.g,
			NodeViewButtonFillColor.b, NodeViewButtonFillColor.a);
		SDL_RenderFillRect(Context->MainRenderer, &Bounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, NodeViewButtonBorderColor.r, NodeViewButtonBorderColor.g,
			NodeViewButtonBorderColor.b, NodeViewButtonBorderColor.a);
		SDL_RenderRect(Context->MainRenderer, &Bounds);
		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), Button.Text,
			Button.x + 5, Button.y + 7, Button.Color);
	}

	void DrawEventLegend(Game::Render::NodeViewRenderer& Renderer, Game::Render::AppRenderContext* Context)
	{
		using namespace Game::Race;

		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), "Events:",
			NodeViewPlotLeft, 438, NodeViewTextColor);
		const RaceEventType EventTypes[] =
		{
			RaceEventType::Gassing,
			RaceEventType::Stumble,
			RaceEventType::LostBelief,
			RaceEventType::BelievesInSelf,
			RaceEventType::SecondWind,
			RaceEventType::Injury,
			RaceEventType::PullingUp
		};
		const int LegendStartX = NodeViewPlotLeft + 60;
		const int LegendItemWidth = 128;
		for (std::size_t Index = 0; Index < std::size(EventTypes); ++Index)
		{
			const int X = LegendStartX + static_cast<int>(Index) * LegendItemWidth;
			const SDL_Color Color = GetEventColor(EventTypes[Index]);
			SDL_FRect Swatch{ static_cast<float>(X), 441.0f, 10.0f, 10.0f };
			SDL_SetRenderDrawColor(Context->MainRenderer, Color.r, Color.g, Color.b, Color.a);
			SDL_RenderFillRect(Context->MainRenderer, &Swatch);
			Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), GetEventName(EventTypes[Index]),
				X + 15, 438, NodeViewTextColor);
		}
	}

	void DrawRacerStats(Game::Render::NodeViewRenderer& Renderer, Game::Render::AppRenderContext* Context,
		const Game::Race::Racer& Runner)
	{
		auto Rating = [](double Value)
		{
			return std::to_string(static_cast<int>(std::lround(Value)));
		};

		const std::string Stats[] =
		{
			"Reflex: " + Rating(Runner.Skills.Reflex),
			"Composure: " + Rating(Runner.Skills.Composure),
			"Acceleration: " + Rating(Runner.Skills.Acceleration),
			"Endurance: " + Rating(Runner.Skills.Endurance),
			"Speed: " + Rating(Runner.Skills.Speed)
		};

		for (std::size_t Index = 0; Index < std::size(Stats); ++Index)
		{
			Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), Stats[Index],
				NodeViewStatsLeft + static_cast<int>(Index) * NodeViewStatsColumnSpacing,
				NodeViewStatsTop, NodeViewTextColor);
		}
	}

	void DrawTimelineGraph(Game::Render::NodeViewRenderer& Renderer, Game::Render::AppRenderContext* Context,
		const Game::Race::Race& CompletedRace, const Game::Race::Racer& Runner,
		const Game::Race::RacerRaceTimeline& Timeline)
	{
		using namespace Game::Race;

		const SDL_FRect PlotBounds{ static_cast<float>(NodeViewPlotLeft), static_cast<float>(NodeViewPlotTop),
			static_cast<float>(NodeViewPlotWidth), static_cast<float>(NodeViewPlotHeight) };
		const double DurationMs = std::max(1.0, Timeline.Finished ? Timeline.FinishTimeMs :
			Timeline.PulledUp ? Timeline.PullUpTimeMs : CompletedRace.GetTimelineDurationMs());
		double MaximumPace = 1.0;
		for (const RaceTimelineNode& Node : Timeline.Nodes)
			MaximumPace = std::max(MaximumPace, std::max(Node.StartPace, Node.EndPace));
		MaximumPace *= 1.1;

		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont),
			"0", NodeViewPlotLeft - 20, NodeViewPlotTop + NodeViewPlotHeight - 8, NodeViewTextColor);
		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont),
			std::to_string(static_cast<unsigned int>(std::lround(MaximumPace))), NodeViewPlotLeft - 35,
			NodeViewPlotTop - 5, NodeViewTextColor);
		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), "100%",
			NodeViewPlotLeft + NodeViewPlotWidth + 5, NodeViewPlotTop - 5, NodeViewTextColor);
		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), "0%",
			NodeViewPlotLeft + NodeViewPlotWidth + 5, NodeViewPlotTop + NodeViewPlotHeight - 8, NodeViewTextColor);

		SDL_SetRenderDrawColor(Context->MainRenderer, NodeViewPlotBackgroundColor.r, NodeViewPlotBackgroundColor.g,
			NodeViewPlotBackgroundColor.b, NodeViewPlotBackgroundColor.a);
		SDL_RenderFillRect(Context->MainRenderer, &PlotBounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, NodeViewBorderColor.r, NodeViewBorderColor.g,
			NodeViewBorderColor.b, NodeViewBorderColor.a);
		SDL_RenderRect(Context->MainRenderer, &PlotBounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, NodeViewGridColor.r, NodeViewGridColor.g,
			NodeViewGridColor.b, NodeViewGridColor.a);
		for (unsigned int Division = 1; Division < 4; ++Division)
		{
			const float Fraction = static_cast<float>(Division) / 4.0f;
			const float GridX = PlotBounds.x + PlotBounds.w * Fraction;
			const float GridY = PlotBounds.y + PlotBounds.h * Fraction;
			SDL_RenderLine(Context->MainRenderer, GridX, PlotBounds.y, GridX, PlotBounds.y + PlotBounds.h);
			SDL_RenderLine(Context->MainRenderer, PlotBounds.x, GridY, PlotBounds.x + PlotBounds.w, GridY);
		}

		bool HasPreviousSample = false;
		float PreviousX = 0.0f;
		float PreviousPaceY = 0.0f;
		float PreviousEnduranceY = 0.0f;
		for (unsigned int Sample = 0; Sample <= NodeViewPlotSampleCount; ++Sample)
		{
			const double Fraction = static_cast<double>(Sample) / NodeViewPlotSampleCount;
			const double TimeMs = DurationMs * Fraction;
			RaceTimelineState State;
			if (!CompletedRace.GetTimelineStateAt(&Runner, TimeMs, State))
				continue;

			const float X = PlotBounds.x + static_cast<float>(Fraction) * PlotBounds.w;
			const float PaceY = PlotBounds.y + static_cast<float>(1.0 - std::clamp(State.Pace / MaximumPace, 0.0, 1.0)) * PlotBounds.h;
			const float EnduranceY = PlotBounds.y + static_cast<float>(1.0 - std::clamp(State.Endurance, 0.0, 1.0)) * PlotBounds.h;
			if (HasPreviousSample)
			{
				SDL_SetRenderDrawColor(Context->MainRenderer, NodeViewPaceColor.r, NodeViewPaceColor.g,
					NodeViewPaceColor.b, NodeViewPaceColor.a);
				SDL_RenderLine(Context->MainRenderer, PreviousX, PreviousPaceY, X, PaceY);
				SDL_SetRenderDrawColor(Context->MainRenderer, NodeViewEnduranceColor.r, NodeViewEnduranceColor.g,
					NodeViewEnduranceColor.b, NodeViewEnduranceColor.a);
				SDL_RenderLine(Context->MainRenderer, PreviousX, PreviousEnduranceY, X, EnduranceY);
			}

			PreviousX = X;
			PreviousPaceY = PaceY;
			PreviousEnduranceY = EnduranceY;
			HasPreviousSample = true;
		}

		for (const RaceTimelineNode& Node : Timeline.Nodes)
		{
			if (Node.Type != RaceTimelineNodeType::RandomEvent && Node.Type != RaceTimelineNodeType::Gassing)
				continue;

			const double EventTimeMs = std::clamp(Node.StartTimeMs, 0.0, DurationMs);
			RaceTimelineState EventState;
			if (!CompletedRace.GetTimelineStateAt(&Runner, EventTimeMs, EventState))
				continue;

			const float EventX = PlotBounds.x + static_cast<float>(EventTimeMs / DurationMs) * PlotBounds.w;
			const float EventY = PlotBounds.y + static_cast<float>(1.0 -
				std::clamp(EventState.Pace / MaximumPace, 0.0, 1.0)) * PlotBounds.h;
			const SDL_Color Color = GetEventColor(Node.Event.Type);
			SDL_FRect MarkerBounds{ EventX - NodeViewEventMarkerSize / 2.0f,
				EventY - NodeViewEventMarkerSize / 2.0f, NodeViewEventMarkerSize, NodeViewEventMarkerSize };
			SDL_SetRenderDrawColor(Context->MainRenderer, Color.r, Color.g, Color.b, Color.a);
			SDL_RenderFillRect(Context->MainRenderer, &MarkerBounds);
		}

		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), "Pace",
			NodeViewPlotLeft + 10, NodeViewPlotTop + 8, NodeViewPaceColor);
		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), "Endurance",
			NodeViewPlotLeft + 65, NodeViewPlotTop + 8, NodeViewEnduranceColor);
		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), "0s",
			NodeViewPlotLeft, NodeViewPlotTop + NodeViewPlotHeight + 4, NodeViewTextColor);
		std::ostringstream EndTimeLabel;
		EndTimeLabel << std::fixed << std::setprecision(1) << DurationMs / 1000.0 << "s";
		Renderer.RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), EndTimeLabel.str(),
			NodeViewPlotLeft + NodeViewPlotWidth - 35, NodeViewPlotTop + NodeViewPlotHeight + 4, NodeViewTextColor);
	}
}

Game::Screens::NodeViewScreen::NodeViewScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO,
	App::AppData& _Data, const Race::Race& RaceToView)
	: AppScreen(_Machine, _IO, _Data),
	CompletedRace(&RaceToView),
	Entrants(RaceToView.GetEntrants()),
	SelectedRacerIndex(0),
	FirstNodeIndex(0),
	PreviousRacerButton(_IO, "< Racer", 70, 62, 90, 36),
	NextRacerButton(_IO, "Racer >", 864, 62, 90, 36),
	PreviousNodePageButton(_IO, "Prev nodes", 70, 700, 110, 42),
	NextNodePageButton(_IO, "Next nodes", 190, 700, 110, 42),
	MainMenuButton(_IO, "Main Menu", 844, 700, 150, 42)
{
	Type = AppScreenType::NodeView;
}

Game::Screens::AppScreen* Game::Screens::NodeViewScreen::Update()
{
	using namespace Game::Audio;

	PreviousRacerButton.Update();
	NextRacerButton.Update();
	PreviousNodePageButton.Update();
	NextNodePageButton.Update();
	MainMenuButton.Update();

	if (!Entrants.empty() && PreviousRacerButton.HasMouseClicked())
	{
		SelectedRacerIndex = SelectedRacerIndex == 0 ? Entrants.size() - 1 : SelectedRacerIndex - 1;
		FirstNodeIndex = 0;
	}
	if (!Entrants.empty() && NextRacerButton.HasMouseClicked())
	{
		SelectedRacerIndex = (SelectedRacerIndex + 1) % Entrants.size();
		FirstNodeIndex = 0;
	}

	const Race::Racer* SelectedRacer = GetSelectedRacer();
	const Race::RacerRaceTimeline* Timeline = CompletedRace->GetTimelineFor(SelectedRacer);
	const std::size_t NodeCount = Timeline == nullptr ? 0 : Timeline->Nodes.size();
	const std::size_t LastPageStart = NodeCount > NodeViewVisibleNodeCount
		? NodeCount - NodeViewVisibleNodeCount : 0;
	if (PreviousNodePageButton.HasMouseClicked())
	{
		FirstNodeIndex = FirstNodeIndex > NodeViewVisibleNodeCount
			? FirstNodeIndex - NodeViewVisibleNodeCount : 0;
	}
	if (NextNodePageButton.HasMouseClicked())
		FirstNodeIndex = std::min(FirstNodeIndex + NodeViewVisibleNodeCount, LastPageStart);
	FirstNodeIndex = std::min(FirstNodeIndex, LastPageStart);

	if (IO.Esc || MainMenuButton.HasMouseClicked())
	{
		if (IO.Esc)
			IO.Player.Play(BuiltInSounds::Click);
		ScreenStack.Pop();
		ScreenStack.Pop();
		return ScreenStack.Top();
	}

	return this;
}

const Game::Race::Race& Game::Screens::NodeViewScreen::GetCompletedRace() const
{
	return *CompletedRace;
}

const Game::Race::Racer* Game::Screens::NodeViewScreen::GetSelectedRacer() const
{
	return Entrants.empty() ? nullptr : Entrants[SelectedRacerIndex];
}

std::size_t Game::Screens::NodeViewScreen::GetSelectedRacerIndex() const
{
	return SelectedRacerIndex;
}

std::size_t Game::Screens::NodeViewScreen::GetRacerCount() const
{
	return Entrants.size();
}

std::size_t Game::Screens::NodeViewScreen::GetFirstNodeIndex() const
{
	return FirstNodeIndex;
}

Game::Render::NodeViewRenderer::NodeViewRenderer(AppRenderContext* Context)
	: BaseRenderer(Context), Screen(nullptr)
{
}

unsigned int Game::Render::NodeViewRenderer::Render()
{
	const unsigned int StartTime = SDL_GetTicks();
	SDL_SetRenderDrawColor(Context->MainRenderer, NodeViewBackgroundColor.r, NodeViewBackgroundColor.g,
		NodeViewBackgroundColor.b, NodeViewBackgroundColor.a);
	SDL_RenderClear(Context->MainRenderer);
	RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), "Race Node View", 70, 20, NodeViewTextColor);

	const Race::Racer* SelectedRacer = Screen->GetSelectedRacer();
	if (SelectedRacer == nullptr)
	{
		RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), "No racer timelines are available.",
			70, 120, NodeViewTextColor);
	}
	else
	{
		RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont),
			"Racer " + std::to_string(Screen->GetSelectedRacerIndex() + 1) + " of " +
			std::to_string(Screen->GetRacerCount()) + ": " + SelectedRacer->Name,
			260, 70, NodeViewTextColor);

		const Race::RacerRaceTimeline* Timeline = Screen->GetCompletedRace().GetTimelineFor(SelectedRacer);
		if (Timeline != nullptr)
		{
			DrawRacerStats(*this, Context, *SelectedRacer);
			DrawTimelineGraph(*this, Context, Screen->GetCompletedRace(), *SelectedRacer, *Timeline);
			const SDL_FRect CurvePlotBounds{ static_cast<float>(NodeViewCurvePlotLeft),
				static_cast<float>(NodeViewCurvePlotTop), static_cast<float>(NodeViewCurvePlotWidth),
				static_cast<float>(NodeViewCurvePlotHeight) };
			DrawRacerCurveVisualization(*this, Context, *SelectedRacer, CurvePlotBounds,
				NodeViewCurvePlotLeft, NodeViewCurveTitleTop, NodeViewCurveLegendTop);
			DrawEventLegend(*this, Context);

			RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), "Node list",
				70, 463, NodeViewTextColor);
			const std::size_t FirstNodeIndex = Screen->GetFirstNodeIndex();
			const std::size_t EndNodeIndex = std::min(FirstNodeIndex + NodeViewVisibleNodeCount, Timeline->Nodes.size());
			const std::string NodeRange = Timeline->Nodes.empty()
				? "Nodes 0 of 0"
				: "Nodes " + std::to_string(FirstNodeIndex + 1) + "-" + std::to_string(EndNodeIndex) +
				" of " + std::to_string(Timeline->Nodes.size());
			RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont),
				NodeRange, 700, 463, NodeViewTextColor);

			auto Node = Timeline->Nodes.begin();
			std::advance(Node, static_cast<std::ptrdiff_t>(FirstNodeIndex));
			for (std::size_t Row = 0; Row < NodeViewVisibleNodeCount && Node != Timeline->Nodes.end(); ++Row, ++Node)
			{
				RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), FormatNodeDetails(*Node),
					70, NodeViewNodeListTop + static_cast<int>(Row) * NodeViewNodeRowHeight, NodeViewTextColor);
			}
		}
		else
		{
			RenderText(Context->Store.GetFont(Game::Assets::FontAssetDebugFont), "No timeline is available for this racer.",
				70, 120, NodeViewTextColor);
		}
	}

	DrawNodeViewButton(*this, Context, Screen->PreviousRacerButton);
	DrawNodeViewButton(*this, Context, Screen->NextRacerButton);
	DrawNodeViewButton(*this, Context, Screen->PreviousNodePageButton);
	DrawNodeViewButton(*this, Context, Screen->NextNodePageButton);
	DrawNodeViewButton(*this, Context, Screen->MainMenuButton);
	Display();
	return SDL_GetTicks() - StartTime;
}
