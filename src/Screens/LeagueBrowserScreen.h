#ifndef LEAGUEBROWSERSCREEN_H
#define LEAGUEBROWSERSCREEN_H
#include "..\Career.h"
#include "..\Render\Renderer.h"
#include "..\Screens.h"
#include "..\Util.h"
#include <vector>

namespace Game
{
	namespace Render
	{
		class LeagueBrowserRenderer;
	}

	namespace Screens
	{
		enum class LeagueBrowserTab
		{
			Standings,
			Fixtures,
			Playoffs
		};

		enum class LeagueBrowserLocationFilter
		{
			All,
			City,
			Region,
			Country
		};

		struct LeagueBrowserLocationContext
		{
			std::uint64_t CityId = 0;
			std::uint64_t RegionId = 0;
			std::uint64_t CountryId = 0;
			LeagueBrowserLocationFilter Filter = LeagueBrowserLocationFilter::All;
		};

		class LeagueBrowserScreen final : public AppScreen
		{
			friend class Game::Render::LeagueBrowserRenderer;

			std::vector<GUI::Button> LeagueButtons;
			std::vector<GUI::Button> LocationFilterButtons;
			std::vector<std::size_t> VisibleLeagueIndices;
			LeagueBrowserLocationContext LocationContext;
			LeagueBrowserLocationFilter LocationFilter = LeagueBrowserLocationFilter::All;
			std::size_t SelectedLeagueIndex = 0;
			std::size_t LeagueScrollOffset = 0;
			std::size_t FixtureScrollOffset = 0;
			std::size_t BracketScrollOffset = 0;
			float LeagueScrollRemainder = 0.0f;
			float ContentScrollRemainder = 0.0f;
			LeagueBrowserTab SelectedTab = LeagueBrowserTab::Standings;
			std::string StatusMessage;

			void RefreshLeagueButtons();
			std::size_t GetMaximumLeagueScrollOffset() const;
			std::size_t GetMaximumFixtureScrollOffset() const;
			std::size_t GetMaximumBracketScrollOffset() const;

		public:
			static constexpr std::size_t VisibleLeagueCount = 12;
			static constexpr std::size_t VisibleFixtureCount = 18;

			GUI::Button StandingsButton;
			GUI::Button FixturesButton;
			GUI::Button PlayoffsButton;
			GUI::Button JoinLeagueButton;
			GUI::Button BackButton;

			LeagueBrowserScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO,
				App::AppData& _Data, LeagueBrowserLocationContext _LocationContext = {});

			AppScreen* Update() override;
			Career::CareerData* GetCareerData() const;
			const Career::CareerLeague* GetSelectedLeague() const;
			bool CanJoinSelectedLeague() const;
			LeagueBrowserTab GetSelectedTab() const;
			std::size_t GetSelectedLeagueIndex() const;
			std::size_t GetLeagueScrollOffset() const;
			std::size_t GetFixtureScrollOffset() const;
			std::size_t GetBracketScrollOffset() const;
			LeagueBrowserLocationFilter GetLocationFilter() const;
			const std::vector<std::size_t>& GetVisibleLeagueIndices() const;
		};
	}

	namespace Render
	{
		class LeagueBrowserRenderer final : public BaseRenderer
		{
		public:
			Screens::LeagueBrowserScreen* Screen = nullptr;

			LeagueBrowserRenderer(AppRenderContext* Context);
			unsigned int Render();
		};
	}
}

#endif
