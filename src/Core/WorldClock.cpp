#include "WorldClock.h"
#include <limits>
#include <stdexcept>

SportRaceCore::WorldClock::WorldClock(std::uint32_t _TimeScale)
	: TimeScale(_TimeScale)
{
	if (TimeScale == 0)
		throw std::invalid_argument("World time scale must be greater than zero.");
}

bool SportRaceCore::WorldClock::Advance(std::uint64_t RealElapsedMs)
{
	if (RealElapsedMs > (std::numeric_limits<std::uint64_t>::max() - CurrentTimeMs) / TimeScale)
		return false;
	CurrentTimeMs += RealElapsedMs * TimeScale;
	return true;
}

std::uint64_t SportRaceCore::WorldClock::NowMs() const
{
	return CurrentTimeMs;
}

std::uint32_t SportRaceCore::WorldClock::GetTimeScale() const
{
	return TimeScale;
}