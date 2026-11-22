#ifndef SPORT_RACE_CORE_COMPETITION_H
#define SPORT_RACE_CORE_COMPETITION_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace SportRaceCore
{
	struct LeagueFixture
	{
		std::uint32_t Round = 0;
		std::uint64_t FirstCompetitorId = 0;
		std::uint64_t SecondCompetitorId = 0;
	};

	struct PlayoffMatch
	{
		std::optional<std::uint64_t> CompetitorAId;
		std::optional<std::uint64_t> CompetitorBId;
		std::uint64_t WinnerId = 0;
		bool IsComplete = false;
	};

	struct PlayoffRound
	{
		std::vector<PlayoffMatch> Matches;
	};

	struct PlayoffBracket
	{
		std::size_t EntrantCount = 0;
		std::vector<PlayoffRound> Rounds;
		std::uint64_t WinnerId = 0;
		bool IsComplete = false;
	};

	std::vector<LeagueFixture> GenerateRoundRobinFixtures(const std::vector<std::uint64_t>& CompetitorIds);
	PlayoffBracket GeneratePlayoffBracket(const std::vector<std::uint64_t>& CompetitorIds);
	bool SetPlayoffMatchWinner(PlayoffBracket& Bracket, std::size_t RoundIndex, std::size_t MatchIndex,
		std::uint64_t WinnerId);
}

#endif