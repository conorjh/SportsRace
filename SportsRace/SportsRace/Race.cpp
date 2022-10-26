#include "Race.h"
#include "SDL2/SDL.h"
#include "spdlog/spdlog.h"
#include "Audio.h"
#include "App.h"
#include "MainMenu.h"
#include <string>

using namespace Game;
using namespace Game::States;
using namespace Game::Race;
using namespace Game::App;
using namespace Game::Audio;
using namespace Game::MainMenu;
using namespace Game::Race;
using namespace std;

Game::Race::Racer::Racer(std::string _Name)
{
    Name = _Name;
    Pos.X = 0;
    Pos.Velocity = 15 + (rand() % 10);
}

void Game::Race::Racer::Tick(unsigned int Ms)
{
    if (Name == "1")
        if (rand() % 3 == 1) 
            Pos.Velocity =20 + (rand() % 15);   //1/3 chance of a change

    else
    if(rand() % 3 == 1)
        Pos.Velocity = 0 + (rand() % 35);   //1/3 chance of a change
}

void Game::Race::Race::Finished(Racer* R)
{
    Result.RacerResults.push_back(RacerRaceResult(R, Result.RacerResults.size() + 1, CurrentTick));
}

bool Game::Race::Race::HasFinished(Racer* R)
{
    for (auto it = Result.RacerResults.begin(); it != Result.RacerResults.end(); ++it)
        if (R->Name == it->Racer->Name)
            return true;
    return false;
}

bool Game::Race::Race::HasFinished()
{
    return (FinishedCount() >= Racers.size());
}

unsigned int Game::Race::Race::CurrentWinnerDistance()
{
    unsigned int Max = 0;
    for (vector<Racer*>::iterator it = Racers.begin(); it != Racers.end(); ++it)
        if ( (*it)->Pos.X > Max)
            Max = (*it)->Pos.X;
    return Max;
}

Racer* Game::Race::Race::Get(std::string RacerName)
{
    for (vector<Racer*>::iterator it = Racers.begin(); it != Racers.end(); ++it)
        if ((*it)->Name == RacerName)
            return (*it);
    return nullptr;
}

bool Game::Race::Race::Contains(std::string RacerName)
{
    return Get(RacerName) != nullptr;
}

Game::Race::Race::Race()
{
    ThisTrack = new Track();
}

RaceStateType Game::Race::Race::Tick(unsigned int Ms, RaceStateType Type)
{
    CurrentTick += Ms;

    unsigned int Transition = 0;
    switch (Type)
    {
    case RaceStateType::Racing:
        for (int t = 0; t < Racers.size(); ++t)
        {
            Racers[t]->Tick(Ms);
            Racers[t]->Pos.X += Racers[t]->Pos.Velocity;

            if (Racers[t]->Pos.X > ThisTrack->Length)
            {
                if (!HasFinished(Racers[t]))
                    Finished(Racers[t]);
                Transition++;
            }
        }

        if (Transition >= 1)
            return RaceStateType::Finishing;
        break;

    case RaceStateType::Finishing:
        for (int t = 0; t < Racers.size(); ++t)
        {
            Racers[t]->Tick(Ms);
            Racers[t]->Pos.X += Racers[t]->Pos.Velocity;

            if (Racers[t]->Pos.X > ThisTrack->Length && !HasFinished(Racers[t]))
                Finished(Racers[t]);
        }

        if (Result.RacerResults.size() >= Racers.size())
            return RaceStateType::Finished;
        break;

        default:
            return Type;
    }

    return Type;
}

RaceResult Game::Race::Race::Sim()
{
    RaceStateType CurrentState = RaceStateType::Racing;

    while ((CurrentState = Tick(33, CurrentState)) != RaceStateType::Finished) { }

    return Result;
}
