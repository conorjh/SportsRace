#ifndef NODEVIEW_H
#define NODEVIEW_H
#include "..\Render\Renderer.h"
#include "..\Screens.h"
#include "..\Race.h"
#include "..\Util.h"
#include <vector>

namespace Game
{
	namespace Screens
	{
		class NodeViewScreen : public AppScreen
		{
			const Race::Race* CompletedRace;
			std::vector<Race::Racer*> Entrants;
			std::size_t SelectedRacerIndex;
			std::size_t FirstNodeIndex;

		public:
			GUI::Button PreviousRacerButton;
			GUI::Button NextRacerButton;
			GUI::Button PreviousNodePageButton;
			GUI::Button NextNodePageButton;
			GUI::Button MainMenuButton;

			NodeViewScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data,
				const Race::Race& RaceToView);

			AppScreen* Update() override;
			const Race::Race& GetCompletedRace() const;
			const Race::Racer* GetSelectedRacer() const;
			std::size_t GetSelectedRacerIndex() const;
			std::size_t GetRacerCount() const;
			std::size_t GetFirstNodeIndex() const;
		};
	}

	namespace Render
	{
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
