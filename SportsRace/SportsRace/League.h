#ifndef LEAGUE_H
#define LEAGUE_H
#include <unordered_map>
#include "Race.h"

namespace Game
{
	namespace League
	{
		struct Fixture
		{
			Fixture() {}

			bool Contains(Race::Racer*);
			bool Contains(std::string);

			Race::Race GetRace();

			std::vector<Race::Racer*> Racers;
		};

		struct RoundFixtures
		{
			Fixture GetFixtureThatContains(Race::Racer*);
			Fixture GetFixtureThatContains(std::string);

			std::vector<Fixture> Fixtures;
		};

		struct RoundResults
		{
			std::vector<Race::RacerRaceResult> Results;
		};

		struct SeasonFixtures
		{
			void NextRound() { CurrentRound++; }
			RoundFixtures GetThisRound() { return RoundFixtures[CurrentRound]; }

			unsigned int CurrentRound = 0;
			std::vector<RoundFixtures> RoundFixtures;
		};

		class SeasonFixtureMaker_Random
		{
		public:
			SeasonFixtures Make(std::vector<Race::Racer*> Racers);
		};

		struct LeagueStandingEntry
		{
			LeagueStandingEntry();
			LeagueStandingEntry(Race::Racer* _Racer);

			bool IsNull() { return ((Racer != nullptr && Racer->Name == ""    ) && Rank == Races == First == Second == Third == DNF == Points == 0); }
			
			void Update(Race::RacerRaceResult);

			Race::Racer* Racer;
			unsigned int Rank, Races, First, Second, Third, DNF, Points;
			double AvgTime;
		};

		struct LeagueStanding
		{
			LeagueStanding();

			void AddEntry(LeagueStandingEntry Entry);

			void AddResults(std::vector<Race::RaceResult> Results);
			void AddResult(Race::RacerRaceResult Result), AddResult(Race::RaceResult Results);

			LeagueStandingEntry Get(std::string RacerName);
			LeagueStandingEntry Get(int Rank);

			void Update(Race::RacerRaceResult Result);
			void Sort();

			std::vector<LeagueStandingEntry> Entries;
		};

		class League
		{
			unsigned int CurrentRound = 0;

		public:
			std::vector<Race::Racer*> Racers;
			LeagueStanding Standing;

			League();

			void NextRound() { CurrentRound++; SeasonFixtures.NextRound(); }

			LeagueStanding GetCurrentLeagueStanding();

			SeasonFixtures SeasonFixtures;
		};
	}
}

#endif