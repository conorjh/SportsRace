#include "Appraisal.h"
#include "Racer.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <string_view>

namespace AppraisalDetail
{
	constexpr std::uint64_t FnvOffsetBasis = 14695981039346656037ull;
	constexpr std::uint64_t FnvPrime = 1099511628211ull;
	constexpr double MinimumRating = 0.0;
	constexpr double MaximumRating = 100.0;
	constexpr double MaximumErrorStandardDeviation = 25.0;

	void AppendByte(std::uint64_t& Hash, std::uint8_t Byte)
	{
		Hash ^= Byte;
		Hash *= FnvPrime;
	}

	void AppendInteger(std::uint64_t& Hash, std::uint64_t Value)
	{
		for (unsigned int ByteIndex = 0; ByteIndex < sizeof(Value); ++ByteIndex)
			AppendByte(Hash, static_cast<std::uint8_t>(Value >> (ByteIndex * 8)));
	}

	void AppendDouble(std::uint64_t& Hash, double Value)
	{
		AppendInteger(Hash, std::bit_cast<std::uint64_t>(Value));
	}

	void AppendString(std::uint64_t& Hash, std::string_view Value)
	{
		for (unsigned char Character : Value)
			AppendByte(Hash, Character);
		AppendByte(Hash, 0);
	}

	std::uint32_t CalculateNameCrc32(std::string_view Name)
	{
		std::uint32_t Crc = 0xffffffffu;
		for (unsigned char Character : Name)
		{
			Crc ^= Character;
			for (unsigned int Bit = 0; Bit < 8; ++Bit)
				Crc = (Crc >> 1) ^ (0xedb88320u & (0u - (Crc & 1u)));
		}
		return ~Crc;
	}
}
using namespace AppraisalDetail;

std::uint64_t Game::Appraisal::HashRacer(const Game::Race::Racer& Runner)
{
	std::uint64_t Hash = FnvOffsetBasis;
	AppendInteger(Hash, Runner.GUID.GUID);
	AppendInteger(Hash, CalculateNameCrc32(Runner.Name));
	AppendString(Hash, Runner.Name);
	AppendInteger(Hash, Runner.Skills.Xp);
	AppendDouble(Hash, Runner.Skills.Reflex);
	AppendDouble(Hash, Runner.Skills.Composure);
	AppendDouble(Hash, Runner.Skills.Acceleration);
	AppendDouble(Hash, Runner.Skills.Endurance);
	AppendDouble(Hash, Runner.Skills.Speed);
	AppendDouble(Hash, Runner.Skills.Potential);
	AppendDouble(Hash, Runner.Skills.Trainability);
	AppendDouble(Hash, Runner.Skills.Luck);
	AppendInteger(Hash, Runner.Details.Age);
	AppendDouble(Hash, Runner.Details.HeightCm);
	AppendDouble(Hash, Runner.Details.WeightKg);
	return Hash;
}

double Game::Appraisal::AppraiseRating(double BaseRating, double AccuracyPercent,
	std::uint64_t AppraiserIdentity, std::uint64_t TargetFingerprint, std::uint64_t StatIdentity)
{
	if (!std::isfinite(BaseRating) || BaseRating < MinimumRating || BaseRating > MaximumRating)
		throw std::invalid_argument("Appraised ratings must be finite values from 0 to 100.");
	if (!std::isfinite(AccuracyPercent) || AccuracyPercent < MinimumRating || AccuracyPercent > MaximumRating)
		throw std::invalid_argument("Appraisal accuracy must be a finite percentage from 0 to 100.");
	if (AccuracyPercent == MaximumRating)
		return BaseRating;

	std::uint64_t Seed = FnvOffsetBasis;
	AppendInteger(Seed, AppraiserIdentity);
	AppendInteger(Seed, TargetFingerprint);
	AppendInteger(Seed, StatIdentity);
	AppendDouble(Seed, BaseRating);
	AppendDouble(Seed, AccuracyPercent);

	std::mt19937_64 Generator(Seed);
	const double ErrorStandardDeviation = (MaximumRating - AccuracyPercent) / MaximumRating *
		MaximumErrorStandardDeviation;
	std::normal_distribution<double> Distribution(BaseRating, ErrorStandardDeviation);
	return std::clamp(std::round(Distribution(Generator)), MinimumRating, MaximumRating);
}
