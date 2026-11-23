#include "AppRenderer.h"
#include "spdlog/spdlog.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

using namespace Game;
using namespace Game::App;
using namespace Game::Render;
using namespace Game::Screens;
using namespace Game::Render;

Game::App::Render::AppRenderer::AppRenderer(Game::App::AppData* Data, Screens::AppScreenStateMachine* _StateMachine) :
	BaseRenderer(Data->RenderContext),
	MainMenuRen(Data->RenderContext),
	InRaceRen(Data->RenderContext),
	NodeViewRen(Data->RenderContext),
	PostRaceRen(Data->RenderContext),
	CareerHubRen(Data->RenderContext),
	WorldMapRen(Data->RenderContext),
	BuildingRen(Data->RenderContext),
	SlotMachineRen(Data->RenderContext),
	RouletteRen(Data->RenderContext),
	RacerScreenRen(Data->RenderContext, Data->Profile),
	RankingScreenRen(Data->RenderContext),
		RacerDBScreenRen(Data->RenderContext),
		LeagueBrowserRen(Data->RenderContext),
		OptionsScreenRen(Data->RenderContext),
	Data(Data),
	StateMachine(_StateMachine)
{
	if (!SDL_SetRenderDrawBlendMode(Context->MainRenderer, SDL_BLENDMODE_BLEND))
		spdlog::error("Failed to enable renderer alpha blending: {}", SDL_GetError());
	Context->Store.RegisterAll(*this);
}

unsigned int Game::App::Render::AppRenderer::Render()
{
	Context->ShowDebugMenu = Data != nullptr && Data->DebugMenuOpen;
	Context->NodeViewerEnabled = Data != nullptr && Data->NodeViewerEnabled;
	Context->DebugMenuStatus = Data == nullptr ? std::string{} : Data->DebugMenuStatus;
	Context->RacerInfoClickTargets.clear();
	Context->RacerInfoCandidates.clear();
	Context->RacerInfoLinksEnabled = false;
	if (StateMachine->Top() == nullptr)
		return 0;

	const AppScreenType ActiveScreenType = StateMachine->Top()->Type;
	Context->RacerInfoLinksEnabled = Data != nullptr && !Data->DebugMenuOpen &&
		ActiveScreenType != AppScreenType::RaceScreen && ActiveScreenType != AppScreenType::RacerScreen;
	if (Context->RacerInfoLinksEnabled)
	{
		std::unordered_map<std::string, Race::Racer*> CandidatesByName;
		std::unordered_set<std::string> AmbiguousNames;
		const auto AddCandidate = [&CandidatesByName, &AmbiguousNames](Race::Racer* Runner)
		{
			if (Runner == nullptr || Runner->Name.empty() || AmbiguousNames.contains(Runner->Name))
				return;

			const auto [Candidate, Inserted] = CandidatesByName.emplace(Runner->Name, Runner);
			if (!Inserted && Candidate->second != Runner)
			{
				AmbiguousNames.insert(Runner->Name);
				CandidatesByName.erase(Candidate);
			}
		};

		if (Data->Career != nullptr)
		{
			for (Race::Racer* Runner : Data->Career->Racers.GetAll())
				AddCandidate(Runner);
			AddCandidate(Data->Career->PlayerLeagueRacer);
			AddCandidate(&Data->Career->Profile.MainFella);
		}
		if (Data->Profile != nullptr)
			AddCandidate(&Data->Profile->MainFella);

		if (ActiveScreenType == AppScreenType::PostRaceScreen)
		{
			const auto* Screen = static_cast<const PostRaceScreen*>(StateMachine->Top());
			for (const Race::RacerRaceResult& Result : Screen->GetCompletedRace().Result.RacerResults)
				AddCandidate(Result.Racer);
		}
		else if (ActiveScreenType == AppScreenType::NodeViewScreen)
		{
			auto* Screen = static_cast<NodeViewScreen*>(StateMachine->Top());
			AddCandidate(Screen->GetSelectedRacer());
		}

		for (const auto& Candidate : CandidatesByName)
			Context->RacerInfoCandidates.push_back({ Candidate.first, Candidate.second });
		std::sort(Context->RacerInfoCandidates.begin(), Context->RacerInfoCandidates.end(),
			[](const Game::Render::AppRenderContext::RacerInfoCandidate& Left,
				const Game::Render::AppRenderContext::RacerInfoCandidate& Right)
			{
				if (Left.Name.size() != Right.Name.size())
					return Left.Name.size() > Right.Name.size();
				return Left.Name < Right.Name;
			});
	}

	// The active screen type selects the concrete screen object and its matching renderer.
	switch (ActiveScreenType)
	{
	case AppScreenType::RaceScreen:
	{
		RaceScreen* Race_Screen = reinterpret_cast<RaceScreen*>(StateMachine->Top());
		InRaceRen.Screen = Race_Screen;
		return InRaceRen.Render();
	}

	case AppScreenType::NodeViewScreen:
	{
		NodeViewScreen* NodeView_Screen = reinterpret_cast<NodeViewScreen*>(StateMachine->Top());
		NodeViewRen.Screen = NodeView_Screen;
		return NodeViewRen.Render();
	}

	case AppScreenType::PostRaceScreen:
	{
		PostRaceRen.Screen = static_cast<PostRaceScreen*>(StateMachine->Top());
		return PostRaceRen.Render();
	}

	case AppScreenType::MainMenu:
	{
		MainMenuScreen* Menu_Screen = reinterpret_cast<MainMenuScreen*>(StateMachine->Top());
		MainMenuRen.Screen = Menu_Screen;
		return MainMenuRen.Render();
	}

	case AppScreenType::CareerHub:
	{
		CareerHubScreen* CareerHub_Screen = reinterpret_cast<CareerHubScreen*>(StateMachine->Top());
		CareerHubRen.Screen = CareerHub_Screen;
		return CareerHubRen.Render();
	}

	case AppScreenType::WorldMapScreen:
	{
		WorldMapScreen* WorldMap_Screen = reinterpret_cast<WorldMapScreen*>(StateMachine->Top());
		WorldMapRen.Screen = WorldMap_Screen;
		return WorldMapRen.Render();
	}

	case AppScreenType::BookiesScreen:
	case AppScreenType::GymScreen:
	case AppScreenType::LicenseOfficeScreen:
	case AppScreenType::BreedingFarmScreen:
	case AppScreenType::RaceTrackScreen:
	case AppScreenType::StableScreen:
	{
		BuildingScreen* Building_Screen = static_cast<BuildingScreen*>(StateMachine->Top());
		BuildingRen.Screen = Building_Screen;
		return BuildingRen.Render();
	}

	case AppScreenType::SlotMachineScreen:
	{
		SlotMachineRen.Screen = static_cast<SlotMachineScreen*>(StateMachine->Top());
		return SlotMachineRen.Render();
	}

	case AppScreenType::RouletteScreen:
	{
		RouletteRen.Screen = static_cast<RouletteScreen*>(StateMachine->Top());
		return RouletteRen.Render();
	}

	case AppScreenType::RacerScreen:
	{
		RacerScreen* Racer_Screen = reinterpret_cast<RacerScreen*>(StateMachine->Top());
		RacerScreenRen.Screen = Racer_Screen;
		return RacerScreenRen.Render();
	}

	case AppScreenType::RankingScreen:
	{
		RankingScreen* Ranking_Screen = reinterpret_cast<RankingScreen*>(StateMachine->Top());
		RankingScreenRen.Screen = Ranking_Screen;
		return RankingScreenRen.Render();
	}

	case AppScreenType::RacerDBScreen:
	{
		RacerDBScreen* RacerDB_Screen = reinterpret_cast<RacerDBScreen*>(StateMachine->Top());
		RacerDBScreenRen.Screen = RacerDB_Screen;
		return RacerDBScreenRen.Render();
	}

	case AppScreenType::LeagueBrowserScreen:
	{
		LeagueBrowserRen.Screen = static_cast<LeagueBrowserScreen*>(StateMachine->Top());
		return LeagueBrowserRen.Render();
	}

	case AppScreenType::OptionsScreen:
	{
		OptionsScreen* Options_Screen = reinterpret_cast<OptionsScreen*>(StateMachine->Top());
		OptionsScreenRen.Screen = Options_Screen;
		return OptionsScreenRen.Render();
	}
	}

	return 0;
}