#include "LeagueBrowserScreen.h"
#include "..\App\App.h"
#include "..\Audio.h"
#include "RaceScreen.h"
#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <string>
#include <utility>

namespace LeagueBrowserScreenDetail
{
	constexpr int LeaguePanelLeft = 24;
	constexpr int LeaguePanelWidth = 292;
	constexpr int LeaguePanelTop = 100;
	constexpr int LeaguePanelHeight = 550;
	constexpr int LeagueLocationFilterLeft = 28;
	constexpr int LeagueLocationFilterTop = 80;
	constexpr int LeagueLocationFilterWidth = 64;
	constexpr int LeagueLocationFilterHeight = 18;
	constexpr int LeagueLocationFilterStep = 68;
	constexpr int LeagueListLeft = 36;
	constexpr int LeagueListTop = 152;
	constexpr int LeagueListRowStep = 40;
	constexpr int LeagueListRowHeight = 36;
	constexpr int LeagueListWidth = 268;
	constexpr int LeagueDetailLeft = 332;
	constexpr int LeagueDetailWidth = 668;
	constexpr int LeagueContentLeft = 350;
	constexpr int LeagueContentTop = 218;
	constexpr int LeagueContentBottom = 632;
	constexpr int LeagueFixtureRowStep = 22;
	constexpr int LeagueButtonY = 690;
	constexpr std::uint64_t LeagueWorldDayMs = 24ull * 60 * 60 * 1000;
	constexpr SDL_Color LeagueBackground{ 23, 22, 20, 255 };
	constexpr SDL_Color LeaguePanelFill{ 34, 33, 30, 255 };
	constexpr SDL_Color LeaguePanelBorder{ 75, 70, 60, 255 };
	constexpr SDL_Color LeagueText{ 235, 229, 215, 255 };
	constexpr SDL_Color LeagueMuted{ 164, 155, 138, 255 };
	constexpr SDL_Color LeagueAccent{ 178, 61, 49, 255 };
	constexpr SDL_Color LeagueSelected{ 74, 43, 35, 255 };
	constexpr SDL_Color LeagueButtonFill{ 104, 42, 36, 255 };
	constexpr SDL_Color LeagueButtonHover{ 134, 54, 45, 255 };
	constexpr SDL_Color LeagueButtonBorder{ 170, 96, 72, 255 };

	void DrawPanel(SDL_Renderer* Renderer, const SDL_FRect& Bounds, SDL_Color Accent)
	{
		SDL_SetRenderDrawColor(Renderer, LeaguePanelFill.r, LeaguePanelFill.g, LeaguePanelFill.b,
			LeaguePanelFill.a);
		SDL_RenderFillRect(Renderer, &Bounds);
		SDL_SetRenderDrawColor(Renderer, LeaguePanelBorder.r, LeaguePanelBorder.g, LeaguePanelBorder.b,
			LeaguePanelBorder.a);
		SDL_RenderRect(Renderer, &Bounds);
		const SDL_FRect AccentBar{ Bounds.x, Bounds.y, 4.0f, Bounds.h };
		SDL_SetRenderDrawColor(Renderer, Accent.r, Accent.g, Accent.b, Accent.a);
		SDL_RenderFillRect(Renderer, &AccentBar);
	}

	void DrawButton(Game::Render::BaseRenderer& Renderer, Game::Render::AppRenderContext* Context,
		const Game::GUI::Button& Button, bool Selected, bool Compact = false, bool Enabled = true)
	{
		const SDL_FRect Bounds{ static_cast<float>(Button.Rect.x), static_cast<float>(Button.Rect.y),
			static_cast<float>(Button.Rect.w), static_cast<float>(Button.Rect.h) };
		const SDL_Color Fill = !Enabled ? SDL_Color{ 63, 61, 55, 255 } : Selected ? LeagueSelected :
			Button.IsMouseOver() ? LeagueButtonHover : LeagueButtonFill;
		SDL_SetRenderDrawColor(Context->MainRenderer, Fill.r, Fill.g, Fill.b, Fill.a);
		SDL_RenderFillRect(Context->MainRenderer, &Bounds);
		SDL_SetRenderDrawColor(Context->MainRenderer, LeagueButtonBorder.r, LeagueButtonBorder.g,
			LeagueButtonBorder.b, LeagueButtonBorder.a);
		SDL_RenderRect(Context->MainRenderer, &Bounds);
		const Game::Assets::FontAssetId Font = Compact
			? Game::Assets::FontAssetDebugFont : Game::Assets::FontAssetInfoFont;
		Renderer.RenderText(Context->Store.GetFont(Font), Button.Text, Button.Rect.x + 8,
			Button.Rect.y + (Button.Rect.h - 16) / 2, LeagueText);
	}

	std::string ClipText(const std::string& Text, std::size_t MaximumCharacters)
	{
		if (Text.size() <= MaximumCharacters)
			return Text;
		if (MaximumCharacters <= 3)
			return Text.substr(0, MaximumCharacters);
		return Text.substr(0, MaximumCharacters - 3) + "...";
	}

	const Game::Career::CareerRacerState* FindRacerState(const Game::Career::CareerData& Data,
		const Game::Race::Racer* Runner)
	{
		const auto State = std::find_if(Data.RacerStates.begin(), Data.RacerStates.end(),
			[Runner](const Game::Career::CareerRacerState& Candidate)
			{
				return Candidate.Runner == Runner;
			});
		return State == Data.RacerStates.end() ? nullptr : &*State;
	}

	std::string GetFixtureState(const Game::Career::CareerFixtureState State)
	{
		switch (State)
		{
		case Game::Career::CareerFixtureState::Scheduled:
			return "Scheduled";
		case Game::Career::CareerFixtureState::InProgress:
			return "LIVE";
		case Game::Career::CareerFixtureState::Complete:
			return "Complete";
		case Game::Career::CareerFixtureState::Cancelled:
			return "Cancelled";
		}
		return "Unknown";
	}

	const Game::Race::Racer* GetCompetitor(const Game::Career::CareerData& Data, std::uint64_t CompetitorId)
	{
		if (CompetitorId == 0 || CompetitorId > std::numeric_limits<unsigned int>::max())
			return nullptr;
		if (Data.PlayerLeagueRacer != nullptr &&
			Data.PlayerLeagueRacer->GUID.GUID == CompetitorId)
			return Data.PlayerLeagueRacer;
		return Data.Racers.Get(static_cast<unsigned int>(CompetitorId));
	}

	std::string GetCompetitorName(const Game::Career::CareerData& Data,
		const std::optional<std::uint64_t>& CompetitorId)
	{
		if (!CompetitorId.has_value())
			return "TBD";
		if (*CompetitorId == 0)
			return "BYE";
		const Game::Race::Racer* Runner = GetCompetitor(Data, *CompetitorId);
		return Runner == nullptr ? "Unknown racer" : Runner->Name;
	}

	std::string GetCompetitorName(const Game::Career::CareerData& Data, std::uint64_t CompetitorId)
	{
		if (CompetitorId == 0)
			return "-";
		const Game::Race::Racer* Runner = GetCompetitor(Data, CompetitorId);
		return Runner == nullptr ? "Unknown racer" : Runner->Name;
	}

	std::string GetCompetitionLocation(const Game::Career::CareerData& Data,
		const Game::Career::CareerLeague& League)
	{
		std::string Location = Game::Career::CareerCompetitionLevelToString(League.Level);
		if (League.Level == Game::Career::CareerCompetitionLevel::City)
		{
			const auto City = std::find_if(Data.Cities.begin(), Data.Cities.end(),
				[&League](const Game::Career::CareerWorldCity& Candidate)
				{
					return Candidate.Id == League.CityId;
				});
			if (City != Data.Cities.end())
				Location += " | " + City->Name;
		}
		else if (League.Level == Game::Career::CareerCompetitionLevel::Regional)
		{
			const auto Region = std::find_if(Data.Regions.begin(), Data.Regions.end(),
				[&League](const Game::Career::CareerWorldRegion& Candidate)
				{
					return Candidate.Id == League.RegionId;
				});
			if (Region != Data.Regions.end())
				Location += " | " + Region->Name;
		}
		else if (League.Level == Game::Career::CareerCompetitionLevel::National)
		{
			const auto Country = std::find_if(Data.Countries.begin(), Data.Countries.end(),
				[&League](const Game::Career::CareerWorldCountry& Candidate)
				{
					return Candidate.Id == League.CountryId;
				});
			if (Country != Data.Countries.end())
				Location += " | " + Country->Name;
		}
		return Location;
	}

	bool IsKnockoutCompetition(const Game::Career::CareerLeague& League)
	{
		return League.IsCup || League.IsAmateurCup;
	}

	std::string GetDivisionAndState(const Game::Career::CareerData& Data,
		const Game::Career::CareerLeague& League)
	{
		const std::string Location = GetCompetitionLocation(Data, League);
		if (IsKnockoutCompetition(League))
			return Location + " Cup  |  " + Game::Career::CareerLeagueStateToString(League.State);
		return Location + "  |  " + Game::Career::CareerLeagueDivisionToString(League.Division) + "  |  " +
			Game::Career::CareerLeagueStateToString(League.State);
	}

	bool MatchesLocationFilter(const Game::Career::CareerLeague& League,
		const Game::Screens::LeagueBrowserLocationContext& Context,
		Game::Screens::LeagueBrowserLocationFilter Filter)
	{
		switch (Filter)
		{
		case Game::Screens::LeagueBrowserLocationFilter::All:
			return true;
		case Game::Screens::LeagueBrowserLocationFilter::City:
			return League.Level == Game::Career::CareerCompetitionLevel::City &&
				Context.CityId != 0 && League.CityId == Context.CityId;
		case Game::Screens::LeagueBrowserLocationFilter::Region:
			return League.Level == Game::Career::CareerCompetitionLevel::Regional &&
				Context.RegionId != 0 && League.RegionId == Context.RegionId;
		case Game::Screens::LeagueBrowserLocationFilter::Country:
			return League.Level == Game::Career::CareerCompetitionLevel::National &&
				Context.CountryId != 0 && League.CountryId == Context.CountryId;
		default:
			return false;
		}
	}

	std::string GetWorldTime(std::uint64_t WorldTimeMs)
	{
		const std::uint64_t WorldMinutes = WorldTimeMs / 60000;
		const unsigned int Hour = static_cast<unsigned int>((WorldMinutes / 60) % 24);
		const unsigned int Minute = static_cast<unsigned int>(WorldMinutes % 60);
		return (Hour < 10 ? "0" : "") + std::to_string(Hour) + ":" +
			(Minute < 10 ? "0" : "") + std::to_string(Minute);
	}
}
using namespace LeagueBrowserScreenDetail;

Game::Screens::LeagueBrowserScreen::LeagueBrowserScreen(AppScreenStateMachine& _Machine,
	App::AppIO& _IO, App::AppData& _Data, LeagueBrowserLocationContext _LocationContext)
	: AppScreen(_Machine, _IO, _Data),
	LocationContext(_LocationContext),
	LocationFilter(_LocationContext.Filter),
	StandingsButton(_IO, "Standings", 350, 112, 150, 42),
	FixturesButton(_IO, "Fixtures", 512, 112, 150, 42),
	PlayoffsButton(_IO, "Playoffs", 674, 112, 150, 42),
	JoinLeagueButton(_IO, "Join league", 835, 112, 150, 42),
	BackButton(_IO, "Back to career", 824, LeagueButtonY, 160, 48)
{
	Type = AppScreenType::LeagueBrowserScreen;
	const char* LocationFilterLabels[]{ "All", "City", "Region", "Country" };
	LocationFilterButtons.reserve(std::size(LocationFilterLabels));
	for (std::size_t Index = 0; Index < std::size(LocationFilterLabels); ++Index)
		LocationFilterButtons.emplace_back(_IO, LocationFilterLabels[Index],
			LeagueLocationFilterLeft + static_cast<int>(Index) * LeagueLocationFilterStep,
			LeagueLocationFilterTop, LeagueLocationFilterWidth, LeagueLocationFilterHeight);
	if (Data.Career != nullptr)
	{
		if (LocationContext.CityId == 0)
			LocationContext.CityId = Data.Career->HomeCityId;
		const auto City = std::find_if(Data.Career->Cities.begin(), Data.Career->Cities.end(),
			[this](const Career::CareerWorldCity& Entry) { return Entry.Id == LocationContext.CityId; });
		if (City != Data.Career->Cities.end() && LocationContext.RegionId == 0)
			LocationContext.RegionId = City->RegionId;
		const auto Region = std::find_if(Data.Career->Regions.begin(), Data.Career->Regions.end(),
			[this](const Career::CareerWorldRegion& Entry) { return Entry.Id == LocationContext.RegionId; });
		if (Region != Data.Career->Regions.end() && LocationContext.CountryId == 0)
			LocationContext.CountryId = Region->CountryId;
	}
	LeagueButtons.reserve(VisibleLeagueCount);
	for (std::size_t Index = 0; Index < VisibleLeagueCount; ++Index)
		LeagueButtons.emplace_back(_IO, "", LeagueListLeft, LeagueListTop +
			static_cast<int>(Index) * LeagueListRowStep, LeagueListWidth, LeagueListRowHeight);
	RefreshLeagueButtons();
}

Game::Screens::AppScreen* Game::Screens::LeagueBrowserScreen::Update()
{
	BackButton.Update();
	if (BackButton.HasMouseClicked() || IO.Esc)
	{
		IO.Player.Play(Audio::BuiltInSounds::Click);
		ScreenStack.Pop();
		return ScreenStack.Top();
	}
	for (std::size_t Index = 0; Index < LocationFilterButtons.size(); ++Index)
	{
		LocationFilterButtons[Index].Update();
		if (LocationFilterButtons[Index].HasMouseClicked())
		{
			LocationFilter = static_cast<LeagueBrowserLocationFilter>(Index);
			LeagueScrollOffset = 0;
			RefreshLeagueButtons();
			break;
		}
	}

	RefreshLeagueButtons();
	if (Data.Career == nullptr)
		return this;

	const SDL_Point MousePosition{ static_cast<int>(IO.MousePosition.x), static_cast<int>(IO.MousePosition.y) };
	const SDL_Rect LeagueListBounds{ LeaguePanelLeft, LeagueListTop, LeaguePanelWidth,
		static_cast<int>(VisibleLeagueCount) * LeagueListRowStep };
	if (IO.MouseWheelY != 0.0f && SDL_PointInRect(&MousePosition, &LeagueListBounds))
	{
		LeagueScrollRemainder -= IO.MouseWheelY * 3.0f;
		const int RowDelta = static_cast<int>(LeagueScrollRemainder);
		if (RowDelta != 0)
		{
			const long long NewOffset = static_cast<long long>(LeagueScrollOffset) + RowDelta;
			LeagueScrollOffset = static_cast<std::size_t>(std::clamp<long long>(NewOffset, 0,
				static_cast<long long>(GetMaximumLeagueScrollOffset())));
			LeagueScrollRemainder -= static_cast<float>(RowDelta);
		}
	}

	for (std::size_t Row = 0; Row < LeagueButtons.size(); ++Row)
	{
		const std::size_t VisibleIndex = LeagueScrollOffset + Row;
		if (VisibleIndex >= VisibleLeagueIndices.size())
			continue;
		const std::size_t LeagueIndex = VisibleLeagueIndices[VisibleIndex];
		const Career::CareerLeague& League = Data.Career->Leagues[LeagueIndex];
		LeagueButtons[Row].Text = ClipText(League.Name, 31);
		LeagueButtons[Row].Update();
		if (LeagueButtons[Row].HasMouseClicked())
		{
			SelectedLeagueIndex = LeagueIndex;
			FixtureScrollOffset = 0;
			BracketScrollOffset = 0;
			SelectedTab = IsKnockoutCompetition(League) ? LeagueBrowserTab::Playoffs : LeagueBrowserTab::Standings;
			StatusMessage.clear();
		}
	}

	StandingsButton.Update();
	FixturesButton.Update();
	PlayoffsButton.Update();
	const Career::CareerLeague* SelectedLeagueForTabs = GetSelectedLeague();
	if (SelectedLeagueForTabs != nullptr && IsKnockoutCompetition(*SelectedLeagueForTabs))
		SelectedTab = LeagueBrowserTab::Playoffs;
	else if (StandingsButton.HasMouseClicked())
		SelectedTab = LeagueBrowserTab::Standings;
	else if (FixturesButton.HasMouseClicked())
		SelectedTab = LeagueBrowserTab::Fixtures;
	else if (PlayoffsButton.HasMouseClicked())
		SelectedTab = LeagueBrowserTab::Playoffs;

	const Career::CareerLeague* SelectedLeague = GetSelectedLeague();
	const bool PlayerIsMember = SelectedLeague != nullptr &&
		Data.Career->PlayerLeagueId == SelectedLeague->Id;
	Career::CareerOrchestrator Orchestrator(Data.Career, Data.Profile);
	const bool CanRace = PlayerIsMember && Orchestrator.CanRaceNextPlayerLeagueMatch();
	const bool CanJoin = CanJoinSelectedLeague();
	const std::string SelectedLeagueName = SelectedLeague == nullptr ? std::string{} : SelectedLeague->Name;
	const bool IsPreparingRace = Data.Career->PendingPlayerLeagueRace.Preparing;
	JoinLeagueButton.Text = CanRace ? "Race next match" : CanJoin ? "Join amateur league" :
		PlayerIsMember ? (IsPreparingRace ? "Preparing race" : "League member") : "Join unavailable";
	JoinLeagueButton.Update();
	if (JoinLeagueButton.HasMouseClicked())
	{
		if (CanRace && Data.Profile != nullptr)
		{
			Race::Race RaceBuffer;
			if (Orchestrator.PreparePlayerLeagueRace(RaceBuffer))
			{
				StatusMessage.clear();
				ScreenStack.Push(new RaceScreen(ScreenStack, IO, Data, std::move(RaceBuffer),
					Data.Profile->MainFella.GUID));
				return ScreenStack.Top();
			}
			StatusMessage = "The field is travelling to the track; the race will be ready shortly.";
		}
		else if (CanJoin && SelectedLeague != nullptr)
		{
			if (Orchestrator.JoinLeague(SelectedLeague->Id))
				StatusMessage = "Joined " + SelectedLeagueName + ". The season starts when the division is full.";
			else
				StatusMessage = "Could not join this league. Please select an open amateur lobby.";
		}
		else if (PlayerIsMember)
			StatusMessage = IsPreparingRace ? "The selected racers are travelling to the track."
				: "No player fixture is ready yet.";
	}

	if (SelectedTab == LeagueBrowserTab::Fixtures && IO.MouseWheelY != 0.0f &&
		IO.MousePosition.x >= LeagueDetailLeft && IO.MousePosition.x <= LeagueDetailLeft + LeagueDetailWidth &&
		IO.MousePosition.y >= LeagueContentTop && IO.MousePosition.y <= LeagueContentBottom)
	{
		ContentScrollRemainder -= IO.MouseWheelY * 3.0f;
		const int RowDelta = static_cast<int>(ContentScrollRemainder);
		if (RowDelta != 0)
		{
			const long long NewOffset = static_cast<long long>(FixtureScrollOffset) + RowDelta;
			FixtureScrollOffset = static_cast<std::size_t>(std::clamp<long long>(NewOffset, 0,
				static_cast<long long>(GetMaximumFixtureScrollOffset())));
			ContentScrollRemainder -= static_cast<float>(RowDelta);
		}
	}
	else if (SelectedTab == LeagueBrowserTab::Playoffs && IO.MouseWheelY != 0.0f &&
		IO.MousePosition.x >= LeagueDetailLeft && IO.MousePosition.x <= LeagueDetailLeft + LeagueDetailWidth &&
		IO.MousePosition.y >= LeagueContentTop && IO.MousePosition.y <= LeagueContentBottom)
	{
		ContentScrollRemainder -= IO.MouseWheelY * 3.0f;
		const int RowDelta = static_cast<int>(ContentScrollRemainder);
		if (RowDelta != 0)
		{
			const long long NewOffset = static_cast<long long>(BracketScrollOffset) + RowDelta;
			BracketScrollOffset = static_cast<std::size_t>(std::clamp<long long>(NewOffset, 0,
				static_cast<long long>(GetMaximumBracketScrollOffset())));
			ContentScrollRemainder -= static_cast<float>(RowDelta);
		}
	}
	if ((IO.MouseButtonsPressed & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) != 0)
	{
		if (SelectedTab == LeagueBrowserTab::Fixtures && SelectedLeague != nullptr)
		{
			const std::size_t FirstFixture = FixtureScrollOffset;
			const std::size_t EndFixture = std::min(SelectedLeague->Fixtures.size(),
				FirstFixture + VisibleFixtureCount);
			for (std::size_t Index = FirstFixture; Index < EndFixture; ++Index)
			{
				const int RowY = 216 + static_cast<int>(Index - FirstFixture) * LeagueFixtureRowStep;
				const SDL_Rect RowBounds{ LeagueDetailLeft + 8, RowY - 2, LeagueDetailWidth - 16,
					LeagueFixtureRowStep };
				const Career::CareerLeagueFixture& Fixture = SelectedLeague->Fixtures[Index];
				if (Fixture.State == Career::CareerFixtureState::Complete && Fixture.HistoricalRaceId != 0 &&
					SDL_PointInRect(&MousePosition, &RowBounds) && OpenCareerHistoricalRace(ScreenStack, IO, Data,
						Fixture.HistoricalRaceId, Data.NodeViewerEnabled && IO.ShiftClickPressed))
					return ScreenStack.Top();
			}
		}
		else if (SelectedTab == LeagueBrowserTab::Playoffs && SelectedLeague != nullptr &&
			!SelectedLeague->Playoffs.Rounds.empty())
		{
			const SportRaceCore::PlayoffBracket& Bracket = SelectedLeague->Playoffs;
			const double ViewportHeight = LeagueContentBottom - LeagueContentTop;
			const std::size_t FirstRoundMatchCount = Bracket.Rounds.front().Matches.size();
			const double MatchSpacing = std::max(28.0,
				ViewportHeight / static_cast<double>(std::max<std::size_t>(1, FirstRoundMatchCount)));
			const double ScrollDistance = static_cast<double>(BracketScrollOffset) * MatchSpacing;
			const double ContentTop = LeagueContentTop + 4.0;
			const double ColumnWidth = static_cast<double>(LeagueDetailWidth - 32) /
				static_cast<double>(Bracket.Rounds.size());
			const double BoxWidth = std::max(48.0, std::min(190.0, ColumnWidth - 18.0));
			const double BoxHeight = std::max(22.0, std::min(58.0, MatchSpacing * 0.86));
			const double FirstColumnX = LeagueDetailLeft + 16.0;
			for (std::size_t RoundIndex = 0; RoundIndex < Bracket.Rounds.size(); ++RoundIndex)
				for (std::size_t MatchIndex = 0; MatchIndex < Bracket.Rounds[RoundIndex].Matches.size(); ++MatchIndex)
				{
					const double SlotSpan = std::pow(2.0, static_cast<double>(RoundIndex));
					const double CenterY = ContentTop +
						(static_cast<double>(MatchIndex) * SlotSpan + SlotSpan * 0.5) * MatchSpacing - ScrollDistance;
					const double RoundX = FirstColumnX + RoundIndex * ColumnWidth;
					const double BoxTop = CenterY - BoxHeight * 0.5;
					if (IO.MousePosition.x < RoundX || IO.MousePosition.x >= RoundX + BoxWidth ||
						IO.MousePosition.y < BoxTop || IO.MousePosition.y >= BoxTop + BoxHeight)
						continue;
					const SportRaceCore::PlayoffMatch& Match = Bracket.Rounds[RoundIndex].Matches[MatchIndex];
					if (Match.IsComplete && Match.HistoricalRaceId != 0 && OpenCareerHistoricalRace(ScreenStack, IO,
						Data, Match.HistoricalRaceId, Data.NodeViewerEnabled && IO.ShiftClickPressed))
						return ScreenStack.Top();
				}
		}
	}
	return this;
}

void Game::Screens::LeagueBrowserScreen::RefreshLeagueButtons()
{
	VisibleLeagueIndices.clear();
	if (Data.Career == nullptr)
	{
		SelectedLeagueIndex = 0;
		LeagueScrollOffset = 0;
		return;
	}
	for (std::size_t Index = 0; Index < Data.Career->Leagues.size(); ++Index)
		if (MatchesLocationFilter(Data.Career->Leagues[Index], LocationContext, LocationFilter))
			VisibleLeagueIndices.push_back(Index);
	if (VisibleLeagueIndices.empty())
	{
		SelectedLeagueIndex = 0;
		LeagueScrollOffset = 0;
		FixtureScrollOffset = 0;
		BracketScrollOffset = 0;
		return;
	}
	if (std::find(VisibleLeagueIndices.begin(), VisibleLeagueIndices.end(), SelectedLeagueIndex) ==
		VisibleLeagueIndices.end())
		SelectedLeagueIndex = VisibleLeagueIndices.front();
	LeagueScrollOffset = std::min(LeagueScrollOffset, GetMaximumLeagueScrollOffset());
}

std::size_t Game::Screens::LeagueBrowserScreen::GetMaximumLeagueScrollOffset() const
{
	if (VisibleLeagueIndices.size() <= VisibleLeagueCount)
		return 0;
	return VisibleLeagueIndices.size() - VisibleLeagueCount;
}

std::size_t Game::Screens::LeagueBrowserScreen::GetMaximumFixtureScrollOffset() const
{
	const Career::CareerLeague* League = GetSelectedLeague();
	return League != nullptr && League->Fixtures.size() > VisibleFixtureCount
		? League->Fixtures.size() - VisibleFixtureCount : 0;
}

std::size_t Game::Screens::LeagueBrowserScreen::GetMaximumBracketScrollOffset() const
{
	const Career::CareerLeague* League = GetSelectedLeague();
	if (League == nullptr || League->Playoffs.Rounds.empty() ||
		League->Playoffs.Rounds.front().Matches.empty())
		return 0;
	const std::size_t FirstRoundMatches = League->Playoffs.Rounds.front().Matches.size();
	constexpr double ViewportHeight = LeagueContentBottom - LeagueContentTop;
	constexpr double MinimumMatchSpacing = 28.0;
	const double MatchSpacing = std::max(MinimumMatchSpacing,
		ViewportHeight / static_cast<double>(FirstRoundMatches));
	const double ContentHeight = MatchSpacing * static_cast<double>(FirstRoundMatches);
	if (ContentHeight <= ViewportHeight)
		return 0;
	return static_cast<std::size_t>(std::ceil((ContentHeight - ViewportHeight) / MatchSpacing));
}

Game::Career::CareerData* Game::Screens::LeagueBrowserScreen::GetCareerData() const
{
	return Data.Career;
}

const Game::Career::CareerLeague* Game::Screens::LeagueBrowserScreen::GetSelectedLeague() const
{
	if (Data.Career == nullptr ||
		std::find(VisibleLeagueIndices.begin(), VisibleLeagueIndices.end(), SelectedLeagueIndex) ==
			VisibleLeagueIndices.end() || SelectedLeagueIndex >= Data.Career->Leagues.size())
		return nullptr;
	return &Data.Career->Leagues[SelectedLeagueIndex];
}

bool Game::Screens::LeagueBrowserScreen::CanJoinSelectedLeague() const
{
	const Career::CareerLeague* League = GetSelectedLeague();
	if (League == nullptr)
		return false;
	const Career::CareerOrchestrator Orchestrator(Data.Career, Data.Profile);
	return Orchestrator.CanJoinLeague(League->Id);
}

Game::Screens::LeagueBrowserTab Game::Screens::LeagueBrowserScreen::GetSelectedTab() const
{
	return SelectedTab;
}

std::size_t Game::Screens::LeagueBrowserScreen::GetSelectedLeagueIndex() const
{
	return SelectedLeagueIndex;
}

std::size_t Game::Screens::LeagueBrowserScreen::GetLeagueScrollOffset() const
{
	return LeagueScrollOffset;
}

std::size_t Game::Screens::LeagueBrowserScreen::GetFixtureScrollOffset() const
{
	return FixtureScrollOffset;
}

std::size_t Game::Screens::LeagueBrowserScreen::GetBracketScrollOffset() const
{
	return BracketScrollOffset;
}

Game::Screens::LeagueBrowserLocationFilter Game::Screens::LeagueBrowserScreen::GetLocationFilter() const
{
	return LocationFilter;
}

const std::vector<std::size_t>& Game::Screens::LeagueBrowserScreen::GetVisibleLeagueIndices() const
{
	return VisibleLeagueIndices;
}

Game::Render::LeagueBrowserRenderer::LeagueBrowserRenderer(AppRenderContext* Context)
	: BaseRenderer(Context)
{
}

unsigned int Game::Render::LeagueBrowserRenderer::Render()
{
	const unsigned int StartTime = SDL_GetTicks();
	SDL_SetRenderDrawColor(Context->MainRenderer, LeagueBackground.r, LeagueBackground.g,
		LeagueBackground.b, LeagueBackground.a);
	SDL_RenderClear(Context->MainRenderer);
	RenderText(Context->Store.GetFont(Assets::FontAssetMainFont), "LEAGUE BROWSER", 28, 22, LeagueAccent);

	const Career::CareerData* CareerData = Screen == nullptr ? nullptr : Screen->GetCareerData();
	if (CareerData == nullptr)
	{
		RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "Career data is unavailable.", 40, 100,
			LeagueText);
		if (Screen != nullptr)
			DrawButton(*this, Context, Screen->BackButton, false);
		Display();
		return SDL_GetTicks() - StartTime;
	}

	const std::uint64_t WorldMinutes = CareerData->WorldClock.NowMs() / 60000;
	const std::size_t LeagueCount = CareerData->Leagues.size();
	const std::uint64_t NextCupDay = CareerData->NextAmateurCupAtMs / LeagueWorldDayMs + 1;
	const std::string Status = "Showing " + std::to_string(Screen->GetVisibleLeagueIndices().size()) + " of " +
		std::to_string(LeagueCount) + " leagues  |  World day " + std::to_string(WorldMinutes / 1440 + 1) + "  " +
		GetWorldTime(CareerData->WorldClock.NowMs()) + "   |   " +
		"Next cup day " + std::to_string(NextCupDay);
	RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), Status, 28, 58, LeagueMuted);

	const SDL_FRect LeaguePanel{ static_cast<float>(LeaguePanelLeft), static_cast<float>(LeaguePanelTop),
		static_cast<float>(LeaguePanelWidth), static_cast<float>(LeaguePanelHeight) };
	const SDL_FRect DetailPanel{ static_cast<float>(LeagueDetailLeft), static_cast<float>(LeaguePanelTop),
		static_cast<float>(LeagueDetailWidth), static_cast<float>(LeaguePanelHeight) };
	DrawPanel(Context->MainRenderer, LeaguePanel, LeagueAccent);
	DrawPanel(Context->MainRenderer, DetailPanel, LeagueAccent);
	for (std::size_t Index = 0; Index < Screen->LocationFilterButtons.size(); ++Index)
		DrawButton(*this, Context, Screen->LocationFilterButtons[Index],
			Screen->GetLocationFilter() == static_cast<Screens::LeagueBrowserLocationFilter>(Index), true);
	RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "LEAGUES", LeagueListLeft, 112, LeagueAccent);

	for (std::size_t Row = 0; Row < Screen->LeagueButtons.size(); ++Row)
	{
		const std::size_t VisibleIndex = Screen->GetLeagueScrollOffset() + Row;
		const std::vector<std::size_t>& VisibleLeagueIndices = Screen->GetVisibleLeagueIndices();
		if (VisibleIndex >= VisibleLeagueIndices.size())
			continue;
		const std::size_t LeagueIndex = VisibleLeagueIndices[VisibleIndex];
		const Career::CareerLeague& League = CareerData->Leagues[LeagueIndex];
		DrawButton(*this, Context, Screen->LeagueButtons[Row],
			LeagueIndex == Screen->GetSelectedLeagueIndex(), true);
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
			GetDivisionAndState(*CareerData, League), LeagueListLeft + 8,
			LeagueListTop + static_cast<int>(Row) * LeagueListRowStep + 25, LeagueMuted);
	}
	if (Screen->GetVisibleLeagueIndices().empty())
		RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), CareerData->Leagues.empty()
			? "No leagues have been created." : "No leagues match this location filter.",
			LeagueListLeft, LeagueListTop, LeagueText);

	const Career::CareerLeague* League = Screen->GetSelectedLeague();
	if (League == nullptr)
	{
		RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "Select a league to inspect its season.",
			LeagueDetailLeft + 20, 178, LeagueText);
	}
	else
	{
		RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), League->Name +
			(IsKnockoutCompetition(*League) ? "  |  Edition " : "  |  Season ") +
			std::to_string(League->Season) + "  |  " + GetDivisionAndState(*CareerData, *League),
			LeagueDetailLeft + 18, 78, LeagueText);
		DrawButton(*this, Context, Screen->StandingsButton,
			Screen->GetSelectedTab() == Screens::LeagueBrowserTab::Standings, false,
			!IsKnockoutCompetition(*League));
		DrawButton(*this, Context, Screen->FixturesButton,
			Screen->GetSelectedTab() == Screens::LeagueBrowserTab::Fixtures, false,
			!IsKnockoutCompetition(*League));
		DrawButton(*this, Context, Screen->PlayoffsButton,
			Screen->GetSelectedTab() == Screens::LeagueBrowserTab::Playoffs);

		if (IsKnockoutCompetition(*League) && Screen->GetSelectedTab() != Screens::LeagueBrowserTab::Playoffs)
			RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont),
				"This cup is knockout-only. View its Playoffs bracket.", LeagueContentLeft,
				LeagueContentTop, LeagueText);
		else if (Screen->GetSelectedTab() == Screens::LeagueBrowserTab::Standings)
		{
			std::vector<const Career::CareerLeagueStanding*> Standings;
			Standings.reserve(League->Standings.size());
			for (const Career::CareerLeagueStanding& Standing : League->Standings)
				Standings.push_back(&Standing);
			std::stable_sort(Standings.begin(), Standings.end(), [](const auto* Left, const auto* Right)
				{
					if (Left->Points != Right->Points)
						return Left->Points > Right->Points;
					if (Left->Wins != Right->Wins)
						return Left->Wins > Right->Wins;
					if (Left->Runner == nullptr || Right->Runner == nullptr)
						return Left->Runner != nullptr;
					if (Left->Runner->CareerRecord.PositionPoints != Right->Runner->CareerRecord.PositionPoints)
						return Left->Runner->CareerRecord.PositionPoints > Right->Runner->CareerRecord.PositionPoints;
					return Left->Runner->Name < Right->Runner->Name;
				});

			RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), "POS  RACER                         STATUS               W     D     L     PTS     CASH",
				LeagueContentLeft, 190, LeagueMuted);
			const int RowTop = 216;
			for (std::size_t Index = 0; Index < Standings.size(); ++Index)
			{
				const Career::CareerLeagueStanding& Standing = *Standings[Index];
				if (Standing.Runner == nullptr)
					continue;
				const int Y = RowTop + static_cast<int>(Index) * 20;
				if (Y > LeagueContentBottom)
					break;
				const bool IsPlayer = Screen->Data.IsPlayerOwnedRacer(Standing.Runner);
				const Career::CareerRacerState* State = FindRacerState(*CareerData, Standing.Runner);
				const SDL_Color RowColor = IsPlayer ? Render::PlayerOwnedRacerHighlightColor :
					State != nullptr && State->Activity == Career::CareerRacerActivity::Retired
						? LeagueMuted : LeagueText;
				RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), std::to_string(Index + 1),
					LeagueContentLeft, Y, RowColor);
				RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), ClipText(Standing.Runner->Name, 25),
					LeagueContentLeft + 40, Y, RowColor);
				RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
					IsPlayer ? "YOU" : State == nullptr ? "-" :
					Career::CareerRacerActivityToString(State->Activity),
					LeagueContentLeft + 232, Y, RowColor);
				RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), std::to_string(Standing.Wins),
					LeagueContentLeft + 390, Y, RowColor);
				RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), std::to_string(Standing.Draws),
					LeagueContentLeft + 426, Y, RowColor);
				RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), std::to_string(Standing.Losses),
					LeagueContentLeft + 462, Y, RowColor);
				RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), std::to_string(Standing.Points),
					LeagueContentLeft + 500, Y, RowColor);
				RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
					IsPlayer ? (Screen->Data.Profile == nullptr ? "-" :
						"$" + std::to_string(Screen->Data.Profile->Financials.Cash)) :
					State == nullptr ? "-" : "$" + std::to_string(State->Cash),
					LeagueContentLeft + 552, Y, RowColor);
			}
			if (Standings.empty())
				RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "Standings will appear when the season starts.",
					LeagueContentLeft, LeagueContentTop, LeagueText);
		}
		else if (Screen->GetSelectedTab() == Screens::LeagueBrowserTab::Fixtures)
		{
			RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), "ROUND  FIXTURE                                     STATUS       RESULT / TRACK",
				LeagueContentLeft, 190, LeagueMuted);
			const std::size_t FirstFixture = Screen->GetFixtureScrollOffset();
			const std::size_t EndFixture = std::min(League->Fixtures.size(),
				FirstFixture + Screens::LeagueBrowserScreen::VisibleFixtureCount);
			for (std::size_t Index = FirstFixture; Index < EndFixture; ++Index)
			{
				const Career::CareerLeagueFixture& Fixture = League->Fixtures[Index];
				const Race::Racer* FirstRacer = GetCompetitor(*CareerData,
					Fixture.Fixture.FirstCompetitorId);
				const Race::Racer* SecondRacer = GetCompetitor(*CareerData,
					Fixture.Fixture.SecondCompetitorId);
				const std::string FirstName = GetCompetitorName(*CareerData, Fixture.Fixture.FirstCompetitorId);
				const std::string SecondName = GetCompetitorName(*CareerData, Fixture.Fixture.SecondCompetitorId);
				std::string Result = GetFixtureState(Fixture.State);
				const Race::Racer* Winner = nullptr;
				if (Fixture.State == Career::CareerFixtureState::Complete)
				{
					Winner = Fixture.WinnerId == 0 ? nullptr : GetCompetitor(*CareerData, Fixture.WinnerId);
					Result = Fixture.WinnerId == 0 ? "Draw" : "Winner: " +
						ClipText(GetCompetitorName(*CareerData, Fixture.WinnerId), 16);
				}
				else if (Fixture.State == Career::CareerFixtureState::InProgress)
					Result = Fixture.RaceId == 0 ? "Player race" : "Race #" + std::to_string(Fixture.RaceId);
				const std::string TrackName = Fixture.TrackPlotIndex < CareerData->CityPlots.size()
					? CareerData->CityPlots[Fixture.TrackPlotIndex].Name : "-";
				const int Y = 216 + static_cast<int>(Index - FirstFixture) * LeagueFixtureRowStep;
				RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
					std::to_string(Fixture.Fixture.Round + 1), LeagueContentLeft, Y, LeagueText);
				RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), ClipText(FirstName, 16),
					LeagueContentLeft + 50, Y, Screen->Data.IsPlayerOwnedRacer(FirstRacer)
						? Render::PlayerOwnedRacerHighlightColor : LeagueText);
				RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), "v",
					LeagueContentLeft + 170, Y, LeagueMuted);
				RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), ClipText(SecondName, 16),
					LeagueContentLeft + 186, Y, Screen->Data.IsPlayerOwnedRacer(SecondRacer)
						? Render::PlayerOwnedRacerHighlightColor : LeagueText);
				RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), Result,
					LeagueContentLeft + 360, Y,
					Winner != nullptr && Screen->Data.IsPlayerOwnedRacer(Winner)
						? Render::PlayerOwnedRacerHighlightColor :
						Fixture.State == Career::CareerFixtureState::InProgress ? LeagueAccent : LeagueText);
				RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), ClipText(TrackName, 18),
					LeagueContentLeft + 520, Y, LeagueMuted);
			}
			if (League->Fixtures.empty())
				RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), "Fixtures are generated when all league places are filled.",
					LeagueContentLeft, LeagueContentTop, LeagueText);
			else
				RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
					"Showing " + std::to_string(FirstFixture + 1) + "-" + std::to_string(EndFixture) +
					" of " + std::to_string(League->Fixtures.size()) + " fixtures  |  scroll to browse",
					LeagueContentLeft, 624, LeagueMuted);
		}
		else
		{
			const SportRaceCore::PlayoffBracket& Bracket = League->Playoffs;
			if (Bracket.Rounds.empty())
				RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont), IsKnockoutCompetition(*League)
					? "The next knockout edition starts on world day " + std::to_string(NextCupDay) + "."
					: "Playoffs begin when the regular season is resolved.", LeagueContentLeft,
					LeagueContentTop, LeagueText);
			else
			{
				const double ViewportHeight = LeagueContentBottom - LeagueContentTop;
				const std::size_t FirstRoundMatchCount = Bracket.Rounds.front().Matches.size();
				const double MatchSpacing = std::max(28.0,
					ViewportHeight / static_cast<double>(std::max<std::size_t>(1, FirstRoundMatchCount)));
				const double ScrollDistance = static_cast<double>(Screen->GetBracketScrollOffset()) * MatchSpacing;
				const double ContentTop = LeagueContentTop + 4.0;
				const double ContentHeight = MatchSpacing * static_cast<double>(FirstRoundMatchCount);
				const double ColumnWidth = static_cast<double>(LeagueDetailWidth - 32) /
					static_cast<double>(Bracket.Rounds.size());
				const double BoxWidth = std::max(48.0, std::min(190.0, ColumnWidth - 18.0));
				const double BoxHeight = std::max(22.0, std::min(58.0, MatchSpacing * 0.86));
				const double FirstColumnX = LeagueDetailLeft + 16.0;

				for (std::size_t RoundIndex = 0; RoundIndex + 1 < Bracket.Rounds.size(); ++RoundIndex)
				{
					const SportRaceCore::PlayoffRound& Round = Bracket.Rounds[RoundIndex];
					for (std::size_t MatchIndex = 0; MatchIndex < Round.Matches.size(); ++MatchIndex)
					{
						const double SlotSpan = std::pow(2.0, static_cast<double>(RoundIndex));
						const double CenterY = ContentTop +
							(static_cast<double>(MatchIndex) * SlotSpan + SlotSpan * 0.5) * MatchSpacing -
							ScrollDistance;
						const double NextSpan = SlotSpan * 2.0;
						const double NextCenterY = ContentTop +
							(static_cast<double>(MatchIndex / 2) * NextSpan + NextSpan * 0.5) * MatchSpacing -
							ScrollDistance;
						const double StartX = FirstColumnX + (RoundIndex + 1) * ColumnWidth - 16.0;
						const double EndX = FirstColumnX + (RoundIndex + 1) * ColumnWidth;
						const double MiddleX = (StartX + EndX) * 0.5;
						SDL_SetRenderDrawColor(Context->MainRenderer, LeaguePanelBorder.r, LeaguePanelBorder.g,
							LeaguePanelBorder.b, LeaguePanelBorder.a);
						SDL_RenderLine(Context->MainRenderer, static_cast<float>(StartX), static_cast<float>(CenterY),
							static_cast<float>(MiddleX), static_cast<float>(CenterY));
						SDL_RenderLine(Context->MainRenderer, static_cast<float>(MiddleX), static_cast<float>(CenterY),
							static_cast<float>(MiddleX), static_cast<float>(NextCenterY));
						SDL_RenderLine(Context->MainRenderer, static_cast<float>(MiddleX), static_cast<float>(NextCenterY),
							static_cast<float>(EndX), static_cast<float>(NextCenterY));
					}
				}

				for (std::size_t RoundIndex = 0; RoundIndex < Bracket.Rounds.size(); ++RoundIndex)
				{
					const SportRaceCore::PlayoffRound& Round = Bracket.Rounds[RoundIndex];
					const double RoundX = FirstColumnX + RoundIndex * ColumnWidth;
					RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
						"ROUND " + std::to_string(RoundIndex + 1), static_cast<int>(RoundX),
						LeaguePanelTop + 78, LeagueMuted);
					for (std::size_t MatchIndex = 0; MatchIndex < Round.Matches.size(); ++MatchIndex)
					{
						const SportRaceCore::PlayoffMatch& Match = Round.Matches[MatchIndex];
					const double SlotSpan = std::pow(2.0, static_cast<double>(RoundIndex));
					const double CenterY = ContentTop +
						(static_cast<double>(MatchIndex) * SlotSpan + SlotSpan * 0.5) * MatchSpacing -
						ScrollDistance;
					const SDL_FRect MatchBounds{ static_cast<float>(RoundX),
						static_cast<float>(CenterY - BoxHeight * 0.5), static_cast<float>(BoxWidth),
						static_cast<float>(BoxHeight) };
					const SDL_Color MatchFill = Match.IsComplete ? LeagueSelected : LeaguePanelFill;
					SDL_SetRenderDrawColor(Context->MainRenderer, MatchFill.r, MatchFill.g, MatchFill.b,
						MatchFill.a);
					SDL_RenderFillRect(Context->MainRenderer, &MatchBounds);
					SDL_SetRenderDrawColor(Context->MainRenderer, LeaguePanelBorder.r, LeaguePanelBorder.g,
						LeaguePanelBorder.b, LeaguePanelBorder.a);
					SDL_RenderRect(Context->MainRenderer, &MatchBounds);
					const std::string NameA = ClipText(GetCompetitorName(*CareerData, Match.CompetitorAId), 21);
					const std::string NameB = ClipText(GetCompetitorName(*CareerData, Match.CompetitorBId), 21);
					const int NameY = static_cast<int>(CenterY - BoxHeight * 0.5) +
						(BoxHeight <= 30.0 ? 1 : 7);
					const bool AIsWinner = Match.IsComplete && Match.WinnerId != 0 && Match.CompetitorAId.has_value() &&
						*Match.CompetitorAId == Match.WinnerId;
					const bool BIsWinner = Match.IsComplete && Match.WinnerId != 0 && Match.CompetitorBId.has_value() &&
						*Match.CompetitorBId == Match.WinnerId;
					const Race::Racer* CompetitorA = Match.CompetitorAId.has_value()
						? GetCompetitor(*CareerData, *Match.CompetitorAId) : nullptr;
					const Race::Racer* CompetitorB = Match.CompetitorBId.has_value()
						? GetCompetitor(*CareerData, *Match.CompetitorBId) : nullptr;
					RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), NameA,
						static_cast<int>(RoundX + 6), NameY,
						Screen->Data.IsPlayerOwnedRacer(CompetitorA) ? Render::PlayerOwnedRacerHighlightColor :
							AIsWinner ? LeagueAccent : LeagueText);
					if (BoxHeight > 30.0)
						RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), NameB,
							static_cast<int>(RoundX + 6), NameY + 18,
							Screen->Data.IsPlayerOwnedRacer(CompetitorB) ? Render::PlayerOwnedRacerHighlightColor :
								BIsWinner ? LeagueAccent : LeagueText);
					else if (NameB != "BYE" && NameB != "TBD")
						RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), NameB,
							static_cast<int>(RoundX + 6), NameY + 11,
							Screen->Data.IsPlayerOwnedRacer(CompetitorB) ? Render::PlayerOwnedRacerHighlightColor :
								BIsWinner ? LeagueAccent : LeagueText);
				}
				}
				if (ContentHeight > ViewportHeight)
					RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont),
						"Scroll vertically to view all first-round matches.", LeagueContentLeft, 624, LeagueMuted);
				if (Bracket.IsComplete)
				{
					const Race::Racer* Champion = GetCompetitor(*CareerData, Bracket.WinnerId);
					RenderText(Context->Store.GetFont(Assets::FontAssetInfoFont),
						"Champion: " + GetCompetitorName(*CareerData, Bracket.WinnerId),
						LeagueDetailLeft + 388, 78, Screen->Data.IsPlayerOwnedRacer(Champion)
							? Render::PlayerOwnedRacerHighlightColor : LeagueAccent);
				}
			}
		}
	}

	const Career::CareerLeague* SelectedLeague = Screen->GetSelectedLeague();
	const bool PlayerIsMember = SelectedLeague != nullptr &&
		CareerData->PlayerLeagueId == SelectedLeague->Id;
	const bool CanJoinLeague = Screen->CanJoinSelectedLeague();
	const bool CanRaceMatch = PlayerIsMember && Screen->Data.Profile != nullptr &&
		Career::CareerOrchestrator(Screen->Data.Career, Screen->Data.Profile).CanRaceNextPlayerLeagueMatch();
	DrawButton(*this, Context, Screen->JoinLeagueButton, false, false, CanJoinLeague || CanRaceMatch);
	if (!Screen->StatusMessage.empty())
		RenderText(Context->Store.GetFont(Assets::FontAssetDebugFont), Screen->StatusMessage,
			LeagueDetailLeft + 18, 660, LeagueAccent);

	DrawButton(*this, Context, Screen->BackButton, false);
	Display();
	return SDL_GetTicks() - StartTime;
}
