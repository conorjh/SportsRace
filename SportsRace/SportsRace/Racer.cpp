#include "Racer.h"

using namespace Game;
using namespace Game::Race;
using namespace std;

Game::Race::Racer::Racer()
{
    Name = "";
    LastRunFrameEnd = CurrentTick = 0;
    RunFrame = 0;
    Pos.X = 0;
    Pos.Velocity = 15 + (rand() % 10);
}

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


    if (Pos.X < Skills.SprintCutoff && Pos.X < Skills.StandardCutoff)
    {
        if (rand() % 8 == 1)
        {
            Pos.Velocity = Skills.BaseSpeed + (rand() % Skills.StandardVelocity);
            if (Skills.Sprint > 0)
                Pos.Velocity += (rand() % Skills.Sprint);
            if (Skills.Luck > 0)
                Pos.Velocity += (rand() % Skills.Luck);   //1/5
        }
    }
    else if (Pos.X >= Skills.SprintCutoff && Pos.X < Skills.StandardCutoff)
    {
        if (rand() % 10 == 1)
        {
            Pos.Velocity = Skills.BaseSpeed + (rand() % Skills.StandardVelocity);
            if (Skills.Luck > 0)
                Pos.Velocity += (rand() % Skills.Luck);   //1/10
        }
    }
    else if (Pos.X >= Skills.SprintCutoff && Pos.X >= Skills.StandardCutoff)
    {
        if (rand() % 20 == 1)
        {
            Pos.Velocity = Skills.BaseSpeed;
            if (Skills.Luck > 0)
                Pos.Velocity += (rand() % Skills.Luck);   //1/20
        }
    }

    //animation
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

void Game::Race::Racer::Reset()
{
    this->CurrentTick = this->LastRunFrameEnd = this->RunFrame = 0;
    this->Pos.X = 0;
}

Game::Race::RacerNameMaker::RacerNameMaker()
{
    FirstNames =
    {
        "Larry", "Barry", "Steve", "Mickey Boy", "Don", "Fat Steve", "Terry", "Darren", "Mick", "Phil", "Bill", "Johnson", "Ian", "Nigel", "Colin", "Nathan", "Adam", "Josh",
        "Davey", "David", "Dave", "Dim Clive", "Clive", "Clifford", "Jimmy", "Jimbob", "Seth", "Ivan", "Dudley", "McLovin", "Potle", "Woozle", "Elvis", "Elmer", "Jussie", "James", 
        "Pong Whiffy", "Peter"
    };

    SecondNames = 
    {
        "Routledge", "Philips", "Johnson", "Andrews", "Munster", "Jones", "Smith", "Burns", "Simpson", "Mickleover", "Romero", "Kerridge", "Fenwick", "Bittlesby", "MacGregor", "Batty",
        "Harris", "Harrison", "Smithwick", "Chibley", "Schneebly", "Bobert", "Jamieson", "Leeds", "Lawrence", "Nigelson", "Trevors", "Redbridge", "Brooks", "Battersby", "Clark", "Ramsay", 
        "Berry", "Beard", "Redford", "Cousins", "Chapman", "Boobles"
    };
}

std::string Game::Race::RacerNameMaker::Make()
{
    unsigned int FirstNameIndex = rand() % (FirstNames.size() - 1);
    unsigned int SecondNameIndex = rand() % (SecondNames.size() - 1);


    return FirstNames[FirstNameIndex] + " " + SecondNames[SecondNameIndex];
}

Racer* Game::Race::RacerDB::Make(std::string Name)
{
    Racer* R = Name == "" ? new Racer(RacerNameMaker().Make()) : new Racer(Name);

    //while loop until we randomly generate an unused GUID
    while (Container.find((R->GUID.GUID = rand())) != Container.end())
    {
    }

    Container.insert(make_pair(R->GUID.GUID, R));

    return R;
}

Racer* Game::Race::RacerDB::Get(unsigned int GUID)
{
    if (Container.find(GUID) == Container.end())
        return nullptr;

    return Container.find(GUID)->second;
}

void Game::Race::RacerDB::Add(Racer* R)
{
    Container.insert(make_pair(R->GUID.GUID, R));
}

bool Game::Race::RacerGUID::operator==(const RacerGUID& Other)
{
    return Other.GUID == GUID;
}

RacerGUID& Game::Race::RacerGUID::operator=(const RacerGUID& Other)
{
    GUID = Other.GUID;
    return *this;
}

RacerGUID& Game::Race::RacerGUID::operator=(const unsigned int& Other)
{
    GUID = Other;
    return *this;
}

RacerGUID& Game::Race::RacerGUID::operator=(const int& Other)
{
    GUID = Other;
    return *this;
}

Game::Race::RacerSkills::RacerSkills()
{
    Xp = 0;

    BaseSpeed = 1 + rand() % 15;
    StandardVelocity = 1 + rand() % 9;
    Sprint = rand() % 5;
    Luck = rand() % 2;

    StandardCutoff = rand() % 50000;
    SprintCutoff = rand() % (StandardCutoff / 2);
}

Racer* Game::Race::RacerMaker::Make()
{
    Racer* R = new Racer();
    R->Name = RacerNameMaker().Make();
    return R;
}
