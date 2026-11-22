#ifndef SPORT_RACE_CORE_WORLD_CLOCK_H
#define SPORT_RACE_CORE_WORLD_CLOCK_H

#include <cstdint>

namespace SportRaceCore
{
	class WorldClock
	{
	public:
		static constexpr std::uint32_t DefaultTimeScale = 24;

		explicit WorldClock(std::uint32_t TimeScale = DefaultTimeScale);

		bool Advance(std::uint64_t RealElapsedMs);
		std::uint64_t NowMs() const;
		std::uint32_t GetTimeScale() const;

	private:
		std::uint64_t CurrentTimeMs = 0;
		std::uint32_t TimeScale;
	};
}

#endif