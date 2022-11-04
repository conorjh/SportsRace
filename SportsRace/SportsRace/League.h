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

			Race::Race GetRace();

			std::vector<Race::Racer*> Racers;
		};

		struct RoundFixtures
		{
			Fixture Get(Race::Racer*);

			std::vector<Fixture> Fixtures;
		};

		struct RoundResults
		{
			std::vector<Race::RaceResult> Results;
		};

		struct SeasonFixtures
		{
			void NextRound() { CurrentRound++; }
			RoundFixtures Get() { return RoundFixtures[CurrentRound]; }

			unsigned int CurrentRound = 0;
			std::vector<RoundFixtures> RoundFixtures;
		};

		class SeasonFixtureMaker_Random
		{
		public:
			SeasonFixtures Make(std::vector<Race::Racer*> Racers);
		};

		struct Ranking
		{
			Race::Racer* Racer;

			int Ranking;
			int Points;
			unsigned int Races;
		};

		struct Rankings
		{
			std::vector<Ranking>

		};
	}
}

#endif