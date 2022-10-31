#include "RaceState.h"
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
using namespace Game::GUI;
using namespace Game::Race;
using namespace std;

Game::Race::RaceState::RaceState(RaceStateData& _Data) : Data(_Data)
{
    Type = RaceStateType::PreRace;
}

RaceState* Game::Race::RaceState::Tick(unsigned int TimeTakenMs)
{
    return this;
}

void Game::Race::RaceState::Entry()
{
    spdlog::trace("RaceState::Entry({})", RaceStateTypeToString(Type));
}

void Game::Race::RaceState::Exit()
{
}

Game::Race::RaceStateMachine::RaceStateMachine(Race TheRace) : Data(TheRace)
{
    Push(new RaceState_PreRace(Data));
}

RaceStateType Game::Race::RaceStateMachine::Tick(unsigned int TickTimeMs)
{
    Data.CurrentTick += TickTimeMs;

    auto OldState = State;
    auto NewState = State->Tick(TickTimeMs);

    if (OldState != NewState)
    {
        State = NewState;
        delete OldState;
    }

    return State->Type;
}

void Game::Race::RaceStateMachine::Push(RaceState* NewState)
{
    if (State)
    {
        State->Exit();
        delete(State);
    }
    State = NewState;
    State->Entry();
}

Game::Race::RaceStateData::RaceStateData(Race R)
{
    ThisRace = R;
}

Game::Race::RaceState_PreRace::RaceState_PreRace(RaceStateData& _Data) : RaceState(_Data)
{
    for (int t = 0; t < Data.ThisRace.Racers.size(); ++t)
        Data.ThisRace.Racers[t]->Pos.X = 0;

    Type = RaceStateType::PreRace;
    TickAccumulator = 0;

}

RaceState* Game::Race::RaceState_PreRace::Tick(unsigned int TimeTakenMs)
{
    TickAccumulator += TimeTakenMs;

    if (TickAccumulator > 1000)
        return new RaceState_StartersOrders(Data);

    return this;
}

Game::Race::RaceState_StartersOrders::RaceState_StartersOrders(RaceStateData& _Data) : RaceState(_Data)
{
    Type = RaceStateType::StartersOrders;
    TickAccumulator = 0;
}

RaceState* Game::Race::RaceState_StartersOrders::Tick(unsigned int TimeTakenMs)
{
    TickAccumulator += TimeTakenMs;

    if (TickAccumulator > 1000)
        return new RaceState_Racing(Data);


    for (int t = 0; t < Data.ThisRace.Racers.size(); ++t)
        Data.ThisRace.Racers[t]->Pos.X = 0;

    return this;
}

Game::Race::RaceState_Racing::RaceState_Racing(RaceStateData& _Data) : RaceState(_Data)
{
    Type = RaceStateType::Racing;
}

RaceState* Game::Race::RaceState_Racing::Tick(unsigned int TimeTakenMs)
{
    auto ReturnType = Data.ThisRace.Tick(TimeTakenMs, RaceStatus::Racing);

    if (ReturnType == RaceStatus::Finishing)
        return new RaceState_Finishing(Data);
    return this;
}

Game::Race::RaceState_Finishing::RaceState_Finishing(RaceStateData& _Data) : RaceState(_Data)
{

    Type = RaceStateType::Finishing;
}

RaceState* Game::Race::RaceState_Finishing::Tick(unsigned int TimeTakenMs)
{
    auto ReturnType = Data.ThisRace.Tick(TimeTakenMs, RaceStatus::Finishing);

    if (ReturnType == RaceStatus::Finished)
        return new RaceState_Finished(Data);
    return this;
}

Game::Race::RaceState_Finished::RaceState_Finished(RaceStateData& _Data) : RaceState(_Data)
{
    Type = RaceStateType::Finished;
}

RaceState* Game::Race::RaceState_Finished::Tick(unsigned int TimeTakenMs)
{
    return this;
}

std::string Game::Race::RaceStateTypeToString(RaceStateType Type)
{
    switch (Type)
    {
    case RaceStateType::PreRace:
        return "PreRace";
    case RaceStateType::StartersOrders:
        return "StartersOrders";
    case  RaceStateType::Racing:
        return "Racing";
    case RaceStateType::Finishing:
        return "Finishing";
    case RaceStateType::Finished:
        return "Finished";
    default:
        return "";
    }
}
