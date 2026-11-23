#include "Racer.h"
#include <algorithm>
#include <array>
#include <random>
#include <cmath>
#include <cstdlib>
#include <limits>

using namespace Game;
using namespace Game::Race;
using namespace std;

namespace RacerDetail
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
	constexpr std::size_t RacerCareerHistoryLimit = 256;
	constexpr double AnimationBaseFrameTimeMs = 350.0;
	constexpr double AnimationFrameTimeReductionPerPace = 0.1;
	constexpr double MinimumAnimationFrameTimeMs = 80.0;
	constexpr unsigned int AnimationFrameCount = 4;
	constexpr unsigned int CurveInversionIterations = 40;
	constexpr double BezierControlPointWeight = 3.0;
	constexpr double CurveSubdivisionCount = 2.0;
	constexpr unsigned int DescendingCurveSeedSalt = 0x9e3779b9u;
	constexpr double MinimumAccelerationP1Y = 0.4;
	constexpr std::uint64_t RaceEntryXp = 10;
	constexpr std::uint64_t FirstPlaceXpBonus = 15;
	constexpr std::uint64_t SecondPlaceXpBonus = 10;
	constexpr std::uint64_t ThirdPlaceXpBonus = 5;

	unsigned int GenerateRating(std::mt19937& Generator, std::normal_distribution<double>& Distribution,
		unsigned int Minimum, unsigned int Maximum)
	{
		const long RoundedValue = std::lround(Distribution(Generator));
		return static_cast<unsigned int>(std::clamp(RoundedValue,
			static_cast<long>(Minimum), static_cast<long>(Maximum)));
	}

	double EvaluateBezierCoordinate(double Parameter, double Start, double First, double Second, double End)
	{
		const double Remaining = 1.0 - Parameter;
		return Remaining * Remaining * Remaining * Start +
			BezierControlPointWeight * Remaining * Remaining * Parameter * First +
			BezierControlPointWeight * Remaining * Parameter * Parameter * Second +
			Parameter * Parameter * Parameter * End;
	}

	double EvaluateBezierDerivative(double Parameter, double Start, double First, double Second, double End)
	{
		const double Remaining = 1.0 - Parameter;
		return BezierControlPointWeight * Remaining * Remaining * (First - Start) +
			2.0 * BezierControlPointWeight * Remaining * Parameter * (Second - First) +
		BezierControlPointWeight * Parameter * Parameter * (End - Second);
	}

	double InvertBezierCoordinate(double Value, double Start, double First, double Second, double End, bool Increasing)
	{
		double Low = 0.0;
		double High = 1.0;
		double Parameter = std::clamp((Value - Start) / (End - Start), 0.0, 1.0);
		for (unsigned int Iteration = 0; Iteration < 8; ++Iteration)
		{
			const double Coordinate = EvaluateBezierCoordinate(Parameter, Start, First, Second, End);
			const double Error = Coordinate - Value;
			if (std::abs(Error) <= 1e-14)
				return Parameter;

			if (Increasing ? Coordinate < Value : Coordinate > Value)
				Low = Parameter;
			else
				High = Parameter;

			const double Derivative = EvaluateBezierDerivative(Parameter, Start, First, Second, End);
			double Candidate = Derivative != 0.0 ? Parameter - Error / Derivative :
				std::numeric_limits<double>::quiet_NaN();
			if (!std::isfinite(Candidate) || Candidate <= Low || Candidate >= High)
				Candidate = (Low + High) / CurveSubdivisionCount;
			Parameter = Candidate;
		}

		for (unsigned int Iteration = 0; Iteration < CurveInversionIterations; ++Iteration)
		{
			Parameter = (Low + High) / CurveSubdivisionCount;
			const double Coordinate = EvaluateBezierCoordinate(Parameter, Start, First, Second, End);
			if (std::abs(Coordinate - Value) <= 1e-14)
				return Parameter;

			if (Increasing ? Coordinate < Value : Coordinate > Value)
				Low = Parameter;
			else
				High = Parameter;
		}
		return (Low + High) / CurveSubdivisionCount;
	}
}
using namespace RacerDetail;

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

void Game::Race::Racer::RecordCareerResult(unsigned int Position, unsigned int FinishTimeMs)
{
	++CareerRecord.RacesEntered;
	Skills.Xp += RaceEntryXp;
	if (Position == 1)
	{
		++CareerRecord.Wins;
		CareerRecord.PositionPoints += 3;
		Skills.Xp += FirstPlaceXpBonus;
	}
	else
	{
		++CareerRecord.Losses;
		if (Position == 2)
		{
			CareerRecord.PositionPoints += 2;
			Skills.Xp += SecondPlaceXpBonus;
		}
		else if (Position == 3)
		{
			++CareerRecord.PositionPoints;
			Skills.Xp += ThirdPlaceXpBonus;
		}
	}

	if (FinishTimeMs > 0 && (CareerRecord.BestTimeMs == 0 || FinishTimeMs < CareerRecord.BestTimeMs))
		CareerRecord.BestTimeMs = FinishTimeMs;
}

void Game::Race::Racer::RecordCareerResult(unsigned int Position, unsigned int FinishTimeMs,
	const RacerCareerHistoryEntry& HistoryEntry)
{
	RecordCareerResult(Position, FinishTimeMs);

	RacerCareerHistoryEntry RecordedEntry = HistoryEntry;
	RecordedEntry.Position = Position;
	RecordedEntry.FinishTimeMs = FinishTimeMs;
	CareerRecord.History.push_back(RecordedEntry);
	if (CareerRecord.History.size() > RacerCareerHistoryLimit)
		CareerRecord.History.erase(CareerRecord.History.begin());
}

Game::Race::RacerNameMaker::RacerNameMaker()
{
	FirstNames =
	{
		"Alfie", "Andrew", "Archie", "Arthur", "Barry", "Ben", "Callum", "Cameron", "Charlie", "Colin",
		"Connor", "Craig", "Daniel", "Darren", "David", "Dean", "Edward", "Elliot", "Ewan", "Finlay",
		"Frank", "Freddie", "Gareth", "George", "Graham", "Harry", "Ian", "Jack", "James", "Jamie",
		"John", "Josh", "Lewis", "Liam", "Luke", "Martin", "Matthew", "Michael", "Mick", "Nathan",
		"Neil", "Nigel", "Oliver", "Owen", "Peter", "Phil", "Reece", "Richard", "Robert", "Rory",
		"Ryan", "Samuel", "Scott", "Sean", "Shaun", "Simon", "Stephen", "Stuart", "Terry", "Thomas",
		"Timothy", "Toby", "William"
	};

	SecondNames =
	{
		"Andrews", "Atkinson", "Battersby", "Bennett", "Booth", "Bradshaw", "Briggs", "Brooks", "Burns",
		"Campbell", "Cartwright", "Chapman", "Clark", "Clarke", "Clegg", "Cooper", "Cousins", "Curran",
		"Davies", "Denton", "Dodd", "Douglas", "Dunne", "Edwards", "Elliott", "Evans", "Fenwick",
		"Fletcher", "Foster", "Galloway", "Graham", "Greaves", "Hall", "Harding", "Harris", "Harrison",
		"Hartley", "Hargreaves", "Hutton", "Irving", "Jackson", "Jamieson", "Jones", "Kelly", "Kerridge",
		"Kershaw", "Lancaster", "Lawrence", "Lawson", "Leach", "MacGregor", "McAllister", "McBride",
		"McKenna", "McLean", "McNally", "Mercer", "Metcalfe", "Milner", "Mitchell", "Morley", "Murray",
		"Needham", "O'Neill", "Parkinson", "Patterson", "Pearson", "Pennington", "Philips", "Pritchard",
		"Ramsay", "Rawlings", "Redford", "Redbridge", "Rigby", "Robson", "Routledge", "Rowe", "Shepherd",
		"Simpson", "Simmonds", "Sinclair", "Smith", "Spencer", "Sykes", "Tate", "Thompson", "Thorpe",
		"Tindall", "Walker", "Walsh", "Watson", "Whittaker", "Wilcox", "Williams", "Wilson", "Yates"
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
	do
	{
		R->GUID.GUID = static_cast<unsigned int>(rand());
	} while (R->GUID.GUID == 0 || Container.find(R->GUID.GUID) != Container.end());

	Container.insert(make_pair(R->GUID.GUID, R));

	return R;
}

Game::Race::RacerDB::~RacerDB()
{
	for (const auto& Entry : Container)
		delete Entry.second;
}

Racer* Game::Race::RacerDB::Get(unsigned int GUID)
{
	if (Container.find(GUID) == Container.end())
		return nullptr;

	return Container.find(GUID)->second;
}

const Game::Race::Racer* Game::Race::RacerDB::Get(unsigned int GUID) const
{
	const auto Racer = Container.find(GUID);
	return Racer == Container.end() ? nullptr : Racer->second;
}

std::vector<Game::Race::Racer*> Game::Race::RacerDB::GetAll() const
{
	std::vector<Racer*> Racers;
	Racers.reserve(Container.size());
	for (const auto& Entry : Container)
		Racers.push_back(Entry.second);
	std::sort(Racers.begin(), Racers.end(), [](const Racer* Left, const Racer* Right)
		{
			if (Left->Name != Right->Name)
				return Left->Name < Right->Name;
			return Left->GUID.GUID < Right->GUID.GUID;
		});
	return Racers;
}

std::size_t Game::Race::RacerDB::Size() const
{
	return Container.size();
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
	Time = std::clamp(Time, 0.0, 1.0);
	if (Time == 0.0) return Descending ? 1.0 : 0.0;
	if (Time == 1.0) return Descending ? 0.0 : 1.0;
	const double Parameter = InvertBezierCoordinate(Time, 0.0, X1, X2, 1.0, true);
	return EvaluateBezierCoordinate(Parameter, Descending ? 1.0 : 0.0, Y1, Y2, Descending ? 0.0 : 1.0);
}

double Game::Race::RaceCurve::TimeAtValue(double Value) const
{
	Value = std::clamp(Value, 0.0, 1.0);
	const double Start = Descending ? 1.0 : 0.0;
	const double End = Descending ? 0.0 : 1.0;
	if (Value == Start)
		return 0.0;
	if (Value == End)
		return 1.0;
	const double Parameter = InvertBezierCoordinate(Value, Start, Y1, Y2, End, !Descending);
	return EvaluateBezierCoordinate(Parameter, 0.0, X1, X2, 1.0);
}

double Game::Race::RaceCurve::Integrate(double StartTime, double EndTime) const
{
	if (!std::isfinite(StartTime) || !std::isfinite(EndTime))
		return 0.0;
	StartTime = std::clamp(StartTime, 0.0, 1.0);
	EndTime = std::clamp(EndTime, 0.0, 1.0);
	if (StartTime == EndTime)
		return 0.0;
	if (StartTime > EndTime)
		return -Integrate(EndTime, StartTime);

	const double StartValue = Descending ? 1.0 : 0.0;
	const double EndValue = Descending ? 0.0 : 1.0;
	const double StartParameter = InvertBezierCoordinate(StartTime, 0.0, X1, X2, 1.0, true);
	const double EndParameter = InvertBezierCoordinate(EndTime, 0.0, X1, X2, 1.0, true);
	const std::array<double, 4> YCoefficients =
	{
		StartValue,
		BezierControlPointWeight * (Y1 - StartValue),
		BezierControlPointWeight * (StartValue - 2.0 * Y1 + Y2),
		-StartValue + BezierControlPointWeight * Y1 - BezierControlPointWeight * Y2 + EndValue
	};
	const std::array<double, 3> XDerivativeCoefficients =
	{
		BezierControlPointWeight * X1,
		2.0 * BezierControlPointWeight * (X2 - 2.0 * X1),
		BezierControlPointWeight * (1.0 + 3.0 * X1 - 3.0 * X2)
	};
	std::array<double, 6> ProductCoefficients{};
	for (std::size_t YIndex = 0; YIndex < YCoefficients.size(); ++YIndex)
		for (std::size_t XIndex = 0; XIndex < XDerivativeCoefficients.size(); ++XIndex)
			ProductCoefficients[YIndex + XIndex] += YCoefficients[YIndex] * XDerivativeCoefficients[XIndex];

	double Integral = 0.0;
	double StartPower = StartParameter;
	double EndPower = EndParameter;
	for (std::size_t Degree = 0; Degree < ProductCoefficients.size(); ++Degree)
	{
		Integral += ProductCoefficients[Degree] * (EndPower - StartPower) /
			static_cast<double>(Degree + 1);
		StartPower *= StartParameter;
		EndPower *= EndParameter;
	}
	return Integral;
}

Racer* Game::Race::RacerMaker::Make()
{
	Racer* R = new Racer();
	R->Name = RacerNameMaker().Make();
	return R;
}
