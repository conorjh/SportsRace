#ifndef TOURNAMENT_H
#define TOURNAMENT_H
#include <string>
#include <vector>
#include "tree.hh"
namespace Tournie
{	
	struct Competitor
	{
		std::string id;
	};	

	struct Team : Competitor
	{
		std::vector<std::string> TeamMemberIds;
	};

	struct Individual : Competitor
	{

	};

	struct Competition
	{
		std::string id;
		std::vector<Competitor> competitors;
	};

	struct Round
	{
		std::string id;
		std::vector<Competition> competitions;
	};

	struct TournamentSchedule
	{
		TournamentSchedule();

		Round GetRound(int RoundIndex);
		unsigned int GetRoundCount();

	private:
	};

	struct RoundRobinSchedule : public TournamentSchedule
	{
		RoundRobinSchedule(std::vector<Competitor> competitors);

	private:
		tree<Competition> TournamentTree;
	};

	class TournamentScheduler
	{
		virtual TournamentSchedule CreateTournament(std::vector<Competitor> Competitors);
	};

	class RoundRobinScheduler : public TournamentScheduler
	{
		virtual RoundRobinSchedule CreateTournament(std::vector<Competitor> Competitors);
	};
}
#endif