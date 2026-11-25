#ifndef RACERDBSCREEN_H
#define RACERDBSCREEN_H
#include "..\Render\Renderer.h"
#include "..\Screens.h"
#include "..\Race.h"
#include "..\Util.h"
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace Game
{
	namespace Screens
	{
		enum class RacerDBSortColumn
		{
			Rank,
			Name,
			PositionPoints,
			Wins,
			Losses,
			Xp,
			Reflex,
			Composure,
			Acceleration,
			Endurance,
			Speed,
			Potential,
			Trainability,
			Luck
		};

		class RacerDBScreen : public AppScreen
		{
			Race::RacerDB& Database;
			std::vector<Race::Racer*> Racers;
			std::vector<GUI::Button> RacerButtons;
			std::vector<GUI::Button> SortButtons;
			std::unordered_map<const Race::Racer*, std::size_t> RacerRanks;
			Race::Racer* SelectedRacer = nullptr;
			std::size_t ScrollOffset = 0;
			RacerDBSortColumn SortColumn = RacerDBSortColumn::Rank;
			bool SortDescending = false;
			bool ShowingHistory = false;
			float WheelScrollRemainder = 0.0f;
			int ScrollbarDragOffset = 0;
			bool ScrollbarDragging = false;
			bool WasLeftMouseDown = false;

			void UpdateLayout();
			void RefreshRacers();
			void RefreshVisibleRows();
			void SortRacers();
			void SetScrollOffset(std::size_t Offset);
			void UpdateScrollbar();
			std::size_t GetMaxScrollOffset() const;

		public:
			static constexpr std::size_t VisibleRacerCount = 12;

			GUI::Button ExitButton;
			GUI::Button HistoryViewButton;

			RacerDBScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data,
				Race::RacerDB& _Database);

			void Entry() override;
			AppScreen* Update() override;

			std::vector<GUI::Button>& GetRacerButtons();
			std::vector<GUI::Button>& GetSortButtons();
			Race::Racer* GetRacerAt(std::size_t Index) const;
			Race::Racer* GetSelectedRacer() const;
			bool IsShowingHistory() const;
			std::size_t GetRankFor(const Race::Racer* Runner) const;
			RacerDBSortColumn GetSortColumn() const;
			bool IsSortDescending() const;
			void SortByColumn(RacerDBSortColumn Column);
			std::size_t GetScrollOffset() const;
			float GetScrollbarThumbY() const;
			float GetScrollbarThumbHeight() const;
			bool IsScrollbarHovered() const;
			bool IsScrollbarDragging() const;
			bool IsRatingRowHovered(std::size_t Index) const;
			double AppraiseRating(double BaseRating, std::uint64_t TargetFingerprint,
				std::uint64_t StatIdentity) const;
			std::size_t GetRacerCount() const;
		};
	}

	namespace Render
	{
		class RacerDBScreenRenderer : public Render::BaseRenderer
		{
		public:
			Screens::RacerDBScreen* Screen;

			RacerDBScreenRenderer(Game::Render::AppRenderContext* Context);
			unsigned int Render();
		};
	}
}

#endif
