#include "Race.h"
#include "SDL2/SDL.h"
#include "spdlog/spdlog.h"
#include "Audio.h"
#include "App.h"
#include "MainMenu.h"
#include <map>
#include <string>

using namespace Game;
using namespace Game::States;
using namespace Game::Race;
using namespace Game::App;
using namespace Game::Audio;
using namespace Game::GUI;
using namespace Game::Race;
using namespace std;

void Game::Race::Race::Finished(Racer* R)
{
	Result.RacerResults.push_back(RacerRaceResult(R, Result.RacerResults.size() + 1, CurrentTick, GUID));
}

bool Game::Race::Race::HasFinished(Racer* R)
{
	for (auto it = Result.RacerResults.begin(); it != Result.RacerResults.end(); ++it)
		if (R->GUID == it->Racer->GUID)
			return true;
	return false;
}

bool Game::Race::Race::HasFinished()
{
	return (FinishedCount() >= Racers.size());
}

Racer* Game::Race::Race::CurrentWinner()
{
	unsigned int Max = 0, RacerNum = 0, t = -1;
	for (vector<Racer*>::iterator it = Racers.begin(); it != Racers.end(); ++it)
	{
		t++;
		if ((*it)->Pos.X > Max)
		{
			RacerNum = t;
			Max = (*it)->Pos.X;
		}
	}
	return Racers[RacerNum];
}

unsigned int Game::Race::Race::CurrentWinnerDistance()
{
	unsigned int Max = 0;
	for (vector<Racer*>::iterator it = Racers.begin(); it != Racers.end(); ++it)
		if ((*it)->Pos.X > Max)
			Max = (*it)->Pos.X;
	return Max;
}

Racer* Game::Race::Race::Get(RacerGUID Guid)
{
	for (vector<Racer*>::iterator it = Racers.begin(); it != Racers.end(); ++it)
		if ((*it)->GUID == Guid)
			return (*it);
	return nullptr;
}

//count from 1
Racer* Game::Race::Race::GetByRank(unsigned int Rank)
{
	if (Rank == 0 || Rank > Racers.size())
		return nullptr;

	multimap<unsigned int, Racer*> SortList;

	for (vector<Racer*>::iterator it = Racers.begin(); it != Racers.end(); ++it)
		SortList.insert(make_pair((*it)->Pos.X, (*it)));
	
	auto it = SortList.rbegin();
	for (int t = 0; t < Rank - 1; ++t)
		it++;
	
	auto* output = it->second;
	return Get(output->GUID);
}

bool Game::Race::Race::Contains(RacerGUID Guid)
{
	return Get(Guid) != nullptr;
}

Game::Race::Race::Race()
{
	ThisTrack = new Track();
	
}

RaceStatus Game::Race::Race::Tick(unsigned int Ms, RaceStatus Type)
{
	CurrentTick += Ms;

	unsigned int Transition = 0;
	switch (Type)
	{
	case RaceStatus::Racing:
		for (int t = 0; t < Racers.size(); ++t)
		{
			Racers[t]->Tick(Ms);

			if (Racers[t]->Pos.X > unsigned int(ThisTrack->Length))
			{
				if (!HasFinished(Racers[t]))
					Finished(Racers[t]);
				Transition++;
			}
		}

		if (Transition >= 1)
			return RaceStatus::Finishing;
		break;

	case RaceStatus::Finishing:
		for (int t = 0; t < Racers.size(); ++t)
		{
			Racers[t]->Tick(Ms);

			if (Racers[t]->Pos.X > unsigned int(ThisTrack->Length) && !HasFinished(Racers[t]))
				Finished(Racers[t]);

			//stop racers after finish line
			if (Racers[t]->Pos.X > unsigned int(ThisTrack->Length) + 1000)
				Racers[t]->Pos.X = unsigned int(ThisTrack->Length) + 1000;
		}

		if (Result.RacerResults.size() >= Racers.size())
			return RaceStatus::Finished;
		break;

	default:
		return Type;
	}

	return Type;
}

RaceResult Game::Race::Race::Sim()
{
	Result.GUID = GUID;
	RaceStatus CurrentState = RaceStatus::Racing;

	while ((CurrentState = Tick(33, CurrentState)) != RaceStatus::Finished) {}

	return Result;
}

void Game::Race::Race::Reset()
{
	this->Result.RacerResults.clear();
	this->CurrentTick = 0;

	for (int t = 0; t < Racers.size(); ++t)
		Racers[t]->Reset();
}

Game::Race::RaceFinancials::RaceFinancials()
{

	FirstPlacePrize = 
	SecondPlacePrize = 
	ThirdPlacePrize = 
	EntranceFee = 0;
}

Game::Race::RaceFinancials::RaceFinancials(unsigned int _FirstPlacePrize, unsigned int _EntranceFee)
{
	FirstPlacePrize = _FirstPlacePrize;
	SecondPlacePrize = FirstPlacePrize / 2;
	ThirdPlacePrize = FirstPlacePrize / 4;
	EntranceFee = _EntranceFee;

}
