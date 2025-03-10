#include "Career.h"

Game::Career::CareerOrchestrator::CareerOrchestrator(CareerData* _Data, CareerProfile* _Profile)
{
	Data = _Data;
	Profile = _Profile;
}

void Game::Career::CareerOrchestrator::Init()
{
	for (int t = 0; t < 1000; ++t)
	{
		Data->Racers.Make();
	}
}

void Game::Career::CareerOrchestrator::Update(unsigned int Ms)
{
}

Game::Career::CareerData::CareerData()
{

}

Game::Career::CareerProfile::CareerProfile() 
{

}
