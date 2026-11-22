#ifndef APPRAISAL_H
#define APPRAISAL_H
#include <cstdint>

namespace Game
{
	namespace Race
	{
		class Racer;
	}

	namespace Appraisal
	{
		std::uint64_t HashRacer(const Race::Racer& Runner);

		double AppraiseRating(double BaseRating, double AccuracyPercent, std::uint64_t AppraiserIdentity,
			std::uint64_t TargetFingerprint, std::uint64_t StatIdentity);
	}
}

#endif
