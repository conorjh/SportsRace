#include "Career.h"
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>

namespace SportsRaceServerDetail
{
	volatile std::sig_atomic_t KeepRunning = 1;

	void RequestStop(int)
	{
		KeepRunning = 0;
	}

	bool ParseTickLimit(std::string_view Value, std::uint64_t& TickLimit)
	{
		if (Value.empty() || Value.front() == '-')
			return false;

		const std::string Text(Value);
		char* End = nullptr;
		errno = 0;
		const unsigned long long Parsed = std::strtoull(Text.c_str(), &End, 10);
		if (errno != 0 || End == Text.c_str() || *End != '\0' || Parsed == 0)
			return false;

		TickLimit = static_cast<std::uint64_t>(Parsed);
		return true;
	}

	void PrintStatus(const Game::Career::CareerData& Data,
		const Game::Career::CareerOrchestrator& Orchestrator, std::uint64_t Ticks)
	{
		std::uint64_t RaceHistoryEntries = 0;
		std::size_t RacersWithHistory = 0;
		for (const Game::Race::Racer* Runner : Data.Racers.GetAll())
			if (Runner != nullptr && !Runner->CareerRecord.History.empty())
			{
				RaceHistoryEntries += Runner->CareerRecord.History.size();
				++RacersWithHistory;
			}

		std::cout << "ticks=" << Ticks
			<< " world_ms=" << Data.WorldClock.NowMs()
			<< " active_racers=" << Orchestrator.GetActiveRacerCount()
			<< " active_races=" << Orchestrator.GetActiveRaceCount()
			<< " race_history_entries=" << RaceHistoryEntries
			<< " racers_with_history=" << RacersWithHistory << '\n';
	}
}
using namespace SportsRaceServerDetail;

int main(int ArgumentCount, char* Arguments[])
{
	std::uint64_t TickLimit = 0;
	for (int ArgumentIndex = 1; ArgumentIndex < ArgumentCount; ++ArgumentIndex)
	{
		const std::string_view Argument(Arguments[ArgumentIndex]);
		if (Argument == "--help")
		{
			std::cout << "Usage: SportsRaceServer [--ticks <count>]\n"
				"Without --ticks, the server runs until interrupted.\n";
			return 0;
		}
		if (Argument != "--ticks" || ArgumentIndex + 1 >= ArgumentCount ||
			!ParseTickLimit(Arguments[++ArgumentIndex], TickLimit))
		{
			std::cerr << "Invalid arguments. Use --help for usage.\n";
			return 2;
		}
	}

	Game::Career::CareerData CareerData;
	Game::Career::CareerProfile CareerProfile;
	Game::Career::CareerOrchestrator Orchestrator(&CareerData, &CareerProfile);
	Orchestrator.Init();
	std::signal(SIGINT, RequestStop);
	std::signal(SIGTERM, RequestStop);

	std::cout << "SportsRaceServer started; shared career simulation initialized.\n";
	std::uint64_t Ticks = 0;
	if (TickLimit != 0)
	{
		while (Ticks < TickLimit)
		{
			Orchestrator.Update(100);
			++Ticks;
		}
	}
	else
	{
		using Clock = std::chrono::steady_clock;
		auto PreviousUpdate = Clock::now();
		auto PreviousStatus = PreviousUpdate;
		while (KeepRunning != 0)
		{
			const auto Now = Clock::now();
			const auto Elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(Now - PreviousUpdate);
			if (Elapsed.count() > 0)
			{
				Orchestrator.Update(static_cast<std::uint64_t>(Elapsed.count()));
				PreviousUpdate = Now;
				++Ticks;
			}

			if (Now - PreviousStatus >= std::chrono::seconds(5))
			{
				PrintStatus(CareerData, Orchestrator, Ticks);
				PreviousStatus = Now;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	PrintStatus(CareerData, Orchestrator, Ticks);
	std::cout << "SportsRaceServer stopped.\n";
	return 0;
}
