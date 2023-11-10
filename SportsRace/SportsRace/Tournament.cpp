#include "Tournament.hpp"

using namespace Tournie;

tree<Competition> Tournie::RoundRobinScheduler::CreateTournament(std::vector<Competitor> Competitors)
{
    //get next power of 2
    int TotalCompetitors = 1;
    while (TotalCompetitors < Competitors.size())
        TotalCompetitors *= 2;

    tree<Competition> output()
    while (true)
    {

    }

    return tree<Competition>();
}

Tournie::RoundRobinSchedule::RoundRobinSchedule(std::vector<Competitor> competitors, unsigned int matchesPerOpponent)
{

}
