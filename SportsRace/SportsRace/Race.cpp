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

            if (Racers[t]->Pos.X > ThisTrack->Length)
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

            if (Racers[t]->Pos.X > ThisTrack->Length && !HasFinished(Racers[t]))
                Finished(Racers[t]);
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
    RaceStatus CurrentState = RaceStatus::Racing;

    while ((CurrentState = Tick(33, CurrentState)) != RaceStatus::Finished) { }

    return Result;
}
