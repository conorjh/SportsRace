#include "Racer.h"

using namespace Game;
using namespace Game::Race;
using namespace std;

Game::Race::Racer::Racer(std::string _Name)
{
    Name = _Name;
    LastRunFrameEnd = CurrentTick = 0;
    RunFrame = 0;
    Pos.X = 0;
    Pos.Velocity = 15 + (rand() % 10);
}

void Game::Race::Racer::Tick(unsigned int Ms)
{
    CurrentTick += Ms;

    if (Name == "1")
    {
        if (rand() % 10 == 1)
            Pos.Velocity = 22 + (rand() % 8);   //1/3 chance of a change
    }
    else
    {
        if (rand() % 10 == 1)
            Pos.Velocity = 20 + (rand() % 10);   //1/3 chance of a change

    }

    auto FrameTime = (350 - (Pos.Velocity * 10)) * 0.9;
    if (CurrentTick > LastRunFrameEnd + FrameTime)
    {
        RunFrame++;
        LastRunFrameEnd = CurrentTick;
    }
    if (RunFrame > 3)
        RunFrame = 0;

    Pos.X += Pos.Velocity;
}
