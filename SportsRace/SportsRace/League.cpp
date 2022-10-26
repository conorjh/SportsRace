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
		if (R == Racers[t])
			return true;
	return false;
}

bool Game::League::Fixture::Contains(string RacerName)
{
	for (int t = 0; t < Racers.size(); ++t)
		if (RacerName == Racers[t]->Name)
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

Game::League::League::League()
{
	for (int t = 0; t < 600; ++t)
	{
		auto R = new Racer(to_string(t));
		Racers.push_back(R);
		Standing.AddEntry(LeagueStandingEntry(R));
	}

	SeasonFixtures = SeasonFixtureMaker_Random().Make(Racers);
}

LeagueStandingEntry Game::League::LeagueStanding::Get(std::string RacerName)
{
	for(int t=0;t<Entries.size();++t)
		if(Entries[t].Racer->Name == RacerName)
			return Entries[t];

	return LeagueStandingEntry();
}

LeagueStandingEntry Game::League::LeagueStanding::Get(int Rank)
{
	if(Rank  < 0 || Rank >= Entries.size())
		return LeagueStandingEntry();

	return Entries[Rank];
}


Fixture Game::League::RoundFixtures::GetFixtureThatContains(Race::Racer* R)
{
	for (int t = 0; t < Fixtures.size(); ++t)
		if (Fixtures[t].Contains(R))
			return Fixtures[t];
	return Fixture();
}

Fixture Game::League::RoundFixtures::GetFixtureThatContains(string RacerName)
{
	for (int t = 0; t < Fixtures.size(); ++t)
		if (Fixtures[t].Contains(RacerName))
			return Fixtures[t];
	return Fixture();
}


Game::League::LeagueStanding::LeagueStanding()
{
}

void Game::League::LeagueStanding::AddEntry(LeagueStandingEntry Entry)
{
	Entry.Rank = Entries.size();
	Entries.push_back(Entry);
}

void Game::League::LeagueStanding::AddResults(std::vector<Race::RaceResult> Results)
{
	for (int t = 0; t < Results.size(); ++t)
		AddResult(Results[t]);
}

void Game::League::LeagueStanding::AddResult(Race::RacerRaceResult Result)
{
	//find this racers league standing entry
	for (int t = 0; t <  Entries.size(); ++t)
		if (Entries[t].Racer->Name == Result.Racer->Name)
		{
			Entries[t].Update(Result);
		}
}

void Game::League::LeagueStanding::AddResult(RaceResult Results)
{
	for (int t = 0; t < Results.RacerResults.size(); ++t)
		AddResult(Results.RacerResults[t]);
}

void Game::League::LeagueStanding::Update(Race::RacerRaceResult Result)
{
	//find this racers league standing entry
	for (int t = 0; t < Entries.size(); ++t)
	{
		Entries[t].Rank = t;
		if (Entries[t].Racer->Name == Result.Racer->Name)
		{
			Entries[t].Update(Result);
		}
	}

}


void Game::League::LeagueStanding::Sort()
{
	vector<LeagueStandingEntry>& a = Entries;

	bool SomethingSwapped = true;
	while (SomethingSwapped)
	{
		SomethingSwapped = false;

		for (size_t i = 0; i < a.size() - 1; i++)
			if (a[i ].Points > a[i+ 1].Points)
			{
				a[i].Rank = i;
				a[i + 1].Rank = i + 1;
				std::swap(a[i], a[i + 1]);

				SomethingSwapped = true;
			}
	}

	std::reverse(a.begin(), a.end());
	for (size_t i = 0; i < a.size() - 1; i++)
		a[i].Rank = i;

}

Game::League::LeagueStandingEntry::LeagueStandingEntry()
{
	Racer = nullptr;
	Races = First = Second = Third = DNF = Points = Rank = 0;
}

Game::League::LeagueStandingEntry::LeagueStandingEntry(Race::Racer* _Racer)
{
	 Racer = _Racer;
	 Races = First = Second = Third = DNF = Points = Rank = 0;
}

void Game::League::LeagueStandingEntry::Update(Race::RacerRaceResult Result)
{
	if (Result.Racer->Name != Racer->Name)
		return;

	if (Result.Position == 1)
	{
		First++;
		Points += 3;
	}
	else if (Result.Position == 2)
	{
		Second++;
		Points += 2;
	}
	else if (Result.Position == 3)
	{
		Third++;
		Points += 1;
	}
	
	Races++;
	
	if (AvgTime > 0)
		AvgTime = double((AvgTime + Result.Ms) / 2);
	else
		AvgTime = Result.Ms;
}
