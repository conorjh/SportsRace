#ifndef NODEVIEWSCREEN_H
#define NODEVIEWSCREEN_H
#include "..\Render\Renderer.h"
#include "..\Screens.h"
#include "..\Race.h"
#include "..\Util.h"
#include <memory>
#include <vector>

namespace Game
{
	namespace Career
	{
		struct CareerRaceHistoryReplay;
	}

	namespace Screens
	{
		// Read-only inspection of a completed race's racer timelines, events, and curve data.
		class NodeViewScreen : public AppScreen
		{
			std::shared_ptr<Career::CareerRaceHistoryReplay> HistoricalReplayOwner;
			const Race::Race* CompletedRace;
			std::vector<Race::Racer*> Entrants;
			std::size_t SelectedRacerIndex;
			std::size_t FirstNodeIndex;
			bool PopRaceScreenOnExit = true;

		public:
			GUI::Button PreviousRacerButton;
			GUI::Button NextRacerButton;
			GUI::Button PreviousNodePageButton;
			GUI::Button NextNodePageButton;
			GUI::Button MainMenuButton;

			NodeViewScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data,
				const Race::Race& RaceToView,
				std::shared_ptr<Career::CareerRaceHistoryReplay> _HistoricalReplayOwner = nullptr,
				bool _PopRaceScreenOnExit = true);

			AppScreen* Update() override;
			const Race::Race& GetCompletedRace() const;
			Race::Racer* GetSelectedRacer();
			const Race::Racer* GetSelectedRacer() const;
			std::size_t GetSelectedRacerIndex() const;
			std::size_t GetRacerCount() const;
			std::size_t GetFirstNodeIndex() const;
		};
	}

	namespace Render
	{
		// Renders the selected entrant's pace/endurance graph and timeline details.
		class NodeViewRenderer : public BaseRenderer
		{
		public:
			Screens::NodeViewScreen* Screen;

			NodeViewRenderer(AppRenderContext* Context);
			unsigned int Render();
		};
	}
}
#endif
