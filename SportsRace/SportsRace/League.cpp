#include "League.h"

using namespace Game;
using namespace Game::League;
using namespace Game::Race;
using namespace std;

SeasonFixtures Game::League::SeasonFixtureMaker_Random::Make(vector<Racer*> Racers)
{
	SeasonFixtures Season;

	vector<Racer*> TempRacers = Racers;

	unsigned int RacePerRound = Racers.size() / 6;
	unsigned int RoundPerSeason = (Racers.size() / 6);

	for (int RoundCount = 0; RoundCount < RoundPerSeason; ++RoundCount)
	{
		RoundFixtures Round = RoundFixtures();

		for (int FixtureCount = 0; FixtureCount < RacePerRound; ++FixtureCount)
		{
			Fixture ThisFixture = Fixture();

			//get 6 Random players, add them to a fixture
			for (int p = 0; p < 6; ++p)
			{
				unsigned int RandomNum = rand() % Racers.size();
				while (ThisFixture.Contains(Racers[RandomNum]))
					RandomNum = rand() % Racers.size();	//loop until we find a racer not already in the fixture
				ThisFixture.Racers.push_back(Racers[RandomNum]);
			}

			//add that fixture to this round of fixtures
			Round.Fixtures.push_back(ThisFixture);
		}

		//add the round to the season
		Season.RoundFixtures.push_back(Round);
	}
	

	return Season;
}

bool Game::League::Fixture::Contains(Racer* R)
{
	for (int t = 0; t < Racers.size(); ++t)
		if (R->GUID == Racers[t]->GUID)
			return true;
	return false;
}
Game::Race::Race Game::League::Fixture::GetRace()
{
	Game::Race::Race R;

	for (int t = 0; t < Racers.size(); ++t)
		R.Racers.push_back(Racers[t]);

	//R.ThisTrack = new Track();

	return R;
}


Fixture Game::League::RoundFixtures::Get(Race::Racer* R)
{
	for (int t = 0; t < Fixtures.size(); ++t)
		if (Fixtures[t].Contains(R))
			return Fixtures[t];
	return Fixture();
}
