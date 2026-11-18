#include "Racer.h"
#include <algorithm>
#include <random>
#include <cmath>
#include <cstdlib>

using namespace Game;
using namespace Game::Race;
using namespace std;

namespace
{
	constexpr unsigned int MinimumGeneratedSkill = 20;
	constexpr unsigned int MaximumGeneratedSkill = 100;
	constexpr unsigned int MinimumGeneratedPotential = 70;
	constexpr unsigned int MinimumGeneratedTrainability = 1;
	constexpr unsigned int MinimumGeneratedAge = 18;
	constexpr unsigned int MaximumGeneratedAge = 40;
	constexpr double GeneratedRatingMean = 50.0;
	constexpr double GeneratedRatingStandardDeviation = 15.0;
	constexpr double GeneratedPotentialMean = 85.0;
	constexpr double GeneratedPotentialStandardDeviation = 7.5;
	constexpr double StandardNormalMean = 0.0;
	constexpr double StandardNormalDeviation = 1.0;
	constexpr double MeanHeightCm = 174.0;
	constexpr double HeightStandardDeviationCm = 8.8;
	constexpr double MeanWeightKg = 67.3;
	constexpr double WeightHeightCoefficientKg = 9.0;
	constexpr double WeightResidualStandardDeviationKg = 7.3;
	constexpr unsigned int MillisecondsPerSecond = 1000;
	constexpr double AnimationBaseFrameTimeMs = 350.0;
	constexpr double AnimationFrameTimeReductionPerPace = 0.1;
	constexpr double MinimumAnimationFrameTimeMs = 80.0;
	constexpr unsigned int AnimationFrameCount = 4;
	constexpr unsigned int CurveInversionIterations = 40;
	constexpr double BezierControlPointWeight = 3.0;
	constexpr double CurveSubdivisionCount = 2.0;
	constexpr unsigned int DescendingCurveSeedSalt = 0x9e3779b9u;
	constexpr double MinimumAccelerationP1Y = 0.4;

	unsigned int GenerateRating(std::mt19937& Generator, std::normal_distribution<double>& Distribution,
		unsigned int Minimum, unsigned int Maximum)
	{
		const long RoundedValue = std::lround(Distribution(Generator));
		return static_cast<unsigned int>(std::clamp(RoundedValue,
			static_cast<long>(Minimum), static_cast<long>(Maximum)));
	}
}

Game::Race::Racer::Racer() : Racer("")
{
}

Game::Race::Racer::Racer(std::string _Name)
	: Name(_Name), CurveSeed(static_cast<unsigned int>(rand())),
	AccelerationCurve(RaceCurve::Generate(CurveSeed, false)),
	EnduranceCurve(RaceCurve::Generate(CurveSeed, true))
{
	LastRunFrameEnd = CurrentTick = 0;
	RunFrame = 0;
	Pos.X = 0;
	Pos.Velocity = 0.0;
}

void Game::Race::Racer::Tick(unsigned int Ms)
{
	// Pace controls animation cadence; position integrates velocity over the elapsed simulation time.
	CurrentTick += Ms;

	//animation
	auto FrameTime = static_cast<unsigned int>(std::clamp(AnimationBaseFrameTimeMs - Pos.Velocity * AnimationFrameTimeReductionPerPace,
		MinimumAnimationFrameTimeMs, AnimationBaseFrameTimeMs));
	if (CurrentTick > LastRunFrameEnd + FrameTime)
	{
		RunFrame = (RunFrame + 1) % AnimationFrameCount;
		LastRunFrameEnd = CurrentTick;
	}

	Pos.X += static_cast<int>(std::lround(Pos.Velocity * Ms / MillisecondsPerSecond));
}

void Game::Race::Racer::Reset()
{
	this->CurrentTick = this->LastRunFrameEnd = this->RunFrame = 0;
	this->Pos.X = 0;
	this->Pos.Velocity = 0.0;
}

Game::Race::RacerNameMaker::RacerNameMaker()
{
	FirstNames =
	{
		"Larry", "Barry", "Steve", "Mickey Boy", "Don", "Fat Steve", "Terry", "Darren", "Mick", "Phil", "Bill",
		"Johnson", "Ian", "Nigel", "Colin", "Nathan", "Adam", "Josh","Davey", "David", "Dave", "Dim Clive",
		"Clive", "Clifford", "Jimmy", "Jimbob", "Seth", "Ivan", "Dudley", "McLovin", "Potle", "Woozle", "Elvis",
		"Elmer", "Jussie", "James",
		"Pong Whiffy", "Peter"
	};

	SecondNames =
	{
		"Routledge", "Philips", "Johnson", "Andrews", "Munster", "Jones", "Smith", "Burns", "Simpson", "Mickleover",
		"Romero", "Kerridge", "Fenwick", "Bittlesby", "MacGregor", "Batty", "Harris", "Harrison", "Smithwick",
		"Chibley", "Schneebly", "Bobert", "Jamieson", "Leeds", "Lawrence", "Nigelson", "Trevors", "Redbridge",
		"Brooks", "Battersby", "Clark", "Ramsay", "Berry", "Beard", "Redford", "Cousins", "Chapman", "Boobles"
	};
}

std::string Game::Race::RacerNameMaker::Make()
{
	unsigned int FirstNameIndex = static_cast<unsigned int>(rand() % FirstNames.size());
	unsigned int SecondNameIndex = static_cast<unsigned int>(rand() % SecondNames.size());


	return FirstNames[FirstNameIndex] + " " + SecondNames[SecondNameIndex];
}

Racer* Game::Race::RacerDB::Make(std::string Name)
{
	Racer* R = Name == "" 
				? new Racer(RacerNameMaker().Make()) 
				: new Racer(Name);

	// Retry random identifiers that are already in use so each database entry stays addressable.
	while (Container.find((R->GUID.GUID = rand())) != Container.end())
	{
	}

	Container.insert(make_pair(R->GUID.GUID, R));

	return R;
}

Racer* Game::Race::RacerDB::Get(unsigned int GUID)
{
	if (Container.find(GUID) == Container.end())
		return nullptr;

	return Container.find(GUID)->second;
}

void Game::Race::RacerDB::Add(Racer* R)
{
	Container.insert(make_pair(R->GUID.GUID, R));
}

bool Game::Race::RacerGUID::operator==(const RacerGUID& Other)
{
	return Other.GUID == GUID;
}

bool Game::Race::RacerGUID::operator==(RacerGUID& Other)
{
	return Other.GUID == GUID;
}

RacerGUID& Game::Race::RacerGUID::operator=(const RacerGUID& Other)
{
	GUID = Other.GUID;
	return *this;
}

RacerGUID& Game::Race::RacerGUID::operator=(const unsigned int& Other)
{
	GUID = Other;
	return *this;
}

RacerGUID& Game::Race::RacerGUID::operator=(const int& Other)
{
	GUID = Other;
	return *this;
}

Game::Race::RacerSkills::RacerSkills()
{
	Xp = 0;
	std::mt19937 Generator(static_cast<unsigned int>(rand()));
	std::normal_distribution<double> RatingDistribution(GeneratedRatingMean, GeneratedRatingStandardDeviation);
	std::normal_distribution<double> PotentialDistribution(GeneratedPotentialMean, GeneratedPotentialStandardDeviation);
	// Generate a potential ceiling first; acceleration, endurance, and speed cannot exceed it.
	Potential = GenerateRating(Generator, PotentialDistribution, MinimumGeneratedPotential, MaximumGeneratedSkill);
	Reflex = GenerateRating(Generator, RatingDistribution, MinimumGeneratedSkill, MaximumGeneratedSkill);
	Composure = GenerateRating(Generator, RatingDistribution, MinimumGeneratedSkill, MaximumGeneratedSkill);
	Acceleration = std::min<double>(GenerateRating(Generator, RatingDistribution,
		MinimumGeneratedSkill, MaximumGeneratedSkill), Potential);
	Endurance = std::min<double>(GenerateRating(Generator, RatingDistribution,
		MinimumGeneratedSkill, MaximumGeneratedSkill), Potential);
	Speed = std::min<double>(GenerateRating(Generator, RatingDistribution,
		MinimumGeneratedSkill, MaximumGeneratedSkill), Potential);
	Trainability = GenerateRating(Generator, RatingDistribution, MinimumGeneratedTrainability, MaximumGeneratedSkill);
	Luck = GenerateRating(Generator, RatingDistribution, 0, MaximumGeneratedSkill);
}

Game::Race::RacerDetails::RacerDetails()
{
	std::mt19937 Generator(static_cast<unsigned int>(rand()));
	std::normal_distribution<double> Normal(StandardNormalMean, StandardNormalDeviation);
	// Approximate the combined sprinter population described by Uth (2005).
	do
	{
		const double HeightScore = Normal(Generator);
		HeightCm = MeanHeightCm + HeightStandardDeviationCm * HeightScore;
		WeightKg = MeanWeightKg + WeightHeightCoefficientKg * HeightScore + WeightResidualStandardDeviationKg * Normal(Generator);
	} while (HeightCm <= 0.0 || WeightKg <= 0.0);
	Age = MinimumGeneratedAge + rand() % (MaximumGeneratedAge - MinimumGeneratedAge + 1);
}

Game::Race::RaceCurve Game::Race::RaceCurve::Generate(unsigned int Seed, bool Descending)
{
	// Derive each curve from the racer's seed while keeping acceleration and endurance streams distinct.
	std::mt19937 Generator(Seed ^ (Descending ? DescendingCurveSeedSalt : 0u));
	std::uniform_real_distribution<double> Uniform(0.0, 1.0);
	double X1 = Uniform(Generator), X2 = Uniform(Generator);
	double Y1 = Uniform(Generator), Y2 = Uniform(Generator);
	if (X1 > X2) std::swap(X1, X2);
	if ((!Descending && Y1 > Y2) || (Descending && Y1 < Y2)) std::swap(Y1, Y2);
	if (!Descending)
	{
		// Keep acceleration rising and prevent its first control point from producing a sluggish opening.
		Y1 = MinimumAccelerationP1Y + (1.0 - MinimumAccelerationP1Y) * Y1;
		Y2 = std::max(Y1, Y2);
	}
	return { X1, Y1, X2, Y2, Descending };
}

double Game::Race::RaceCurve::Evaluate(double Time) const
{
	// Invert the Bezier x-coordinate so the curve can be sampled by elapsed-time fraction.
	Time = std::clamp(Time, 0.0, 1.0);
	if (Time == 0.0) return Descending ? 1.0 : 0.0;
	if (Time == 1.0) return Descending ? 0.0 : 1.0;
	double Low = 0.0, High = 1.0;
	for (unsigned int I = 0; I < CurveInversionIterations; ++I)
	{
		const double T = (Low + High) / CurveSubdivisionCount;
		const double S = 1.0 - T;
		const double X = BezierControlPointWeight * S * S * T * X1 + BezierControlPointWeight * S * T * T * X2 + T * T * T;
		if (X < Time) Low = T; else High = T;
	}
	const double T = (Low + High) / CurveSubdivisionCount;
	const double S = 1.0 - T;
	return S * S * S * (Descending ? 1.0 : 0.0) + BezierControlPointWeight * S * S * T * Y1 +
		BezierControlPointWeight * S * T * T * Y2 + T * T * T * (Descending ? 0.0 : 1.0);
}

Racer* Game::Race::RacerMaker::Make()
{
	Racer* R = new Racer();
	R->Name = RacerNameMaker().Make();
	return R;
}
