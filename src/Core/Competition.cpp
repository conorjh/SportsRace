#include "Competition.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <unordered_set>

namespace CompetitionDetail
{
	void ValidateCompetitors(const std::vector<std::uint64_t>& CompetitorIds)
	{
		std::unordered_set<std::uint64_t> Seen;
		Seen.reserve(CompetitorIds.size());
		for (const std::uint64_t CompetitorId : CompetitorIds)
			if (CompetitorId == 0 || !Seen.insert(CompetitorId).second)
				throw std::invalid_argument("Competitor IDs must be nonzero and unique.");
	}

	std::vector<std::size_t> CreateSeedOrder(std::size_t BracketSize)
	{
		std::vector<std::size_t> Seeds{ 1, 2 };
		while (Seeds.size() < BracketSize)
		{
			const std::size_t ExpandedBracketSize = Seeds.size() * 2;
			std::vector<std::size_t> ExpandedSeeds;
			ExpandedSeeds.reserve(Seeds.size() * 2);
			for (const std::size_t Seed : Seeds)
			{
				ExpandedSeeds.push_back(Seed);
				ExpandedSeeds.push_back(ExpandedBracketSize + 1 - Seed);
			}
			Seeds = std::move(ExpandedSeeds);
		}
		return Seeds;
	}

	void ResolveByes(SportRaceCore::PlayoffBracket& Bracket)
	{
		for (std::size_t RoundIndex = 0; RoundIndex < Bracket.Rounds.size(); ++RoundIndex)
			for (std::size_t MatchIndex = 0; MatchIndex < Bracket.Rounds[RoundIndex].Matches.size(); ++MatchIndex)
			{
				SportRaceCore::PlayoffMatch& Match = Bracket.Rounds[RoundIndex].Matches[MatchIndex];
				if (Match.IsComplete || !Match.CompetitorAId.has_value() || !Match.CompetitorBId.has_value())
					continue;

				const std::uint64_t CompetitorA = *Match.CompetitorAId;
				const std::uint64_t CompetitorB = *Match.CompetitorBId;
				if (CompetitorA == 0 && CompetitorB == 0)
				{
					Match.IsComplete = true;
					continue;
				}
				if (CompetitorA != 0 && CompetitorB != 0)
					continue;

				Match.WinnerId = CompetitorA != 0 ? CompetitorA : CompetitorB;
				Match.IsComplete = true;
				if (RoundIndex + 1 < Bracket.Rounds.size())
				{
					SportRaceCore::PlayoffMatch& NextMatch =
						Bracket.Rounds[RoundIndex + 1].Matches[MatchIndex / 2];
					if (MatchIndex % 2 == 0)
						NextMatch.CompetitorAId = Match.WinnerId;
					else
						NextMatch.CompetitorBId = Match.WinnerId;
				}
			}

		if (Bracket.Rounds.empty())
			Bracket.IsComplete = true;
		else
		{
			const SportRaceCore::PlayoffMatch& Final = Bracket.Rounds.back().Matches.front();
			if (Final.IsComplete)
			{
				Bracket.WinnerId = Final.WinnerId;
				Bracket.IsComplete = true;
			}
		}
	}
}
using namespace CompetitionDetail;

std::vector<SportRaceCore::LeagueFixture> SportRaceCore::GenerateRoundRobinFixtures(
	const std::vector<std::uint64_t>& CompetitorIds)
{
	ValidateCompetitors(CompetitorIds);
	if (CompetitorIds.size() < 2)
		return {};

	std::vector<std::uint64_t> Rotation = CompetitorIds;
	if (Rotation.size() % 2 != 0)
		Rotation.push_back(0);

	const std::size_t RoundCount = Rotation.size() - 1;
	const std::size_t MatchesPerRound = Rotation.size() / 2;
	std::vector<LeagueFixture> Fixtures;
	Fixtures.reserve(CompetitorIds.size() * (CompetitorIds.size() - 1) / 2);
	for (std::size_t Round = 0; Round < RoundCount; ++Round)
	{
		for (std::size_t Pair = 0; Pair < MatchesPerRound; ++Pair)
		{
			std::uint64_t First = Rotation[Pair];
			std::uint64_t Second = Rotation[Rotation.size() - 1 - Pair];
			if (First == 0 || Second == 0)
				continue;
			if ((Round + Pair) % 2 != 0)
				std::swap(First, Second);
			Fixtures.push_back({ static_cast<std::uint32_t>(Round), First, Second });
		}

		const std::uint64_t Last = Rotation.back();
		for (std::size_t Index = Rotation.size() - 1; Index > 1; --Index)
			Rotation[Index] = Rotation[Index - 1];
		Rotation[1] = Last;
	}
	return Fixtures;
}

SportRaceCore::PlayoffBracket SportRaceCore::GeneratePlayoffBracket(
	const std::vector<std::uint64_t>& CompetitorIds)
{
	ValidateCompetitors(CompetitorIds);
	PlayoffBracket Bracket;
	Bracket.EntrantCount = CompetitorIds.size();
	if (CompetitorIds.size() < 2)
	{
		Bracket.WinnerId = CompetitorIds.empty() ? 0 : CompetitorIds.front();
		Bracket.IsComplete = true;
		return Bracket;
	}

	std::size_t BracketSize = 1;
	while (BracketSize < CompetitorIds.size())
	{
		if (BracketSize > std::numeric_limits<std::size_t>::max() / 2)
			throw std::length_error("The playoff bracket is too large.");
		BracketSize *= 2;
	}

	const std::vector<std::size_t> SeedOrder = CreateSeedOrder(BracketSize);
	std::vector<std::uint64_t> SeededCompetitors(BracketSize, 0);
	std::copy(CompetitorIds.begin(), CompetitorIds.end(), SeededCompetitors.begin());

	PlayoffRound FirstRound;
	FirstRound.Matches.reserve(BracketSize / 2);
	for (std::size_t Slot = 0; Slot < BracketSize; Slot += 2)
	{
		PlayoffMatch Match;
		Match.CompetitorAId = SeededCompetitors[SeedOrder[Slot] - 1];
		Match.CompetitorBId = SeededCompetitors[SeedOrder[Slot + 1] - 1];
		FirstRound.Matches.push_back(std::move(Match));
	}
	Bracket.Rounds.push_back(std::move(FirstRound));

	for (std::size_t MatchCount = BracketSize / 4; MatchCount > 0; MatchCount /= 2)
	{
		PlayoffRound Round;
		Round.Matches.resize(MatchCount);
		Bracket.Rounds.push_back(std::move(Round));
	}
	ResolveByes(Bracket);
	return Bracket;
}

bool SportRaceCore::SetPlayoffMatchWinner(PlayoffBracket& Bracket, std::size_t RoundIndex,
	std::size_t MatchIndex, std::uint64_t WinnerId)
{
	if (WinnerId == 0 || RoundIndex >= Bracket.Rounds.size() ||
		MatchIndex >= Bracket.Rounds[RoundIndex].Matches.size())
		return false;

	PlayoffMatch& Match = Bracket.Rounds[RoundIndex].Matches[MatchIndex];
	if (Match.IsComplete || !Match.CompetitorAId.has_value() || !Match.CompetitorBId.has_value() ||
		(*Match.CompetitorAId != WinnerId && *Match.CompetitorBId != WinnerId))
		return false;

	Match.WinnerId = WinnerId;
	Match.IsComplete = true;
	if (RoundIndex + 1 < Bracket.Rounds.size())
	{
		PlayoffMatch& NextMatch = Bracket.Rounds[RoundIndex + 1].Matches[MatchIndex / 2];
		if (MatchIndex % 2 == 0)
			NextMatch.CompetitorAId = WinnerId;
		else
			NextMatch.CompetitorBId = WinnerId;
	}
	ResolveByes(Bracket);
	return true;
}