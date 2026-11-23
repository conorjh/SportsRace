#include "Career.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <random>
#include <utility>

namespace CareerDetail
{
	constexpr unsigned int CareerRacePredictionDnfTimeMs = 1800000;
	constexpr std::size_t CareerUpcomingRaceEntrantCount = 6;

	unsigned int SelectCareerTrackLength(const Game::Career::CareerData* Data, std::mt19937& Random)
	{
		if (Data == nullptr || Data->RaceTrackLengths.empty())
			return static_cast<unsigned int>(Game::Race::TrackLength::Standard100m);

		std::uniform_int_distribution<std::size_t> TrackDistribution(0, Data->RaceTrackLengths.size() - 1);
		return Data->RaceTrackLengths[TrackDistribution(Random)];
	}
}
using namespace CareerDetail;

double Game::Career::EstimateRacerStrength(const Race::Racer& Runner, unsigned int TrackLength)
{
	const double DistanceFactor = std::clamp(static_cast<double>(TrackLength) / 1000.0, 0.0, 1.0);
	const double AccelerationWeight = 0.25 - 0.10 * DistanceFactor;
	const double EnduranceWeight = 0.15 + 0.10 * DistanceFactor;
	const auto NormalizeRating = [](double Rating)
	{
		return std::clamp(Rating, 0.0, 100.0) / 100.0;
	};
	const double BaseStrength = 100.0 * (
		0.35 * NormalizeRating(Runner.Skills.Speed) +
		AccelerationWeight * NormalizeRating(Runner.Skills.Acceleration) +
		0.15 * NormalizeRating(Runner.Skills.Reflex) +
		EnduranceWeight * NormalizeRating(Runner.Skills.Endurance) +
		0.08 * NormalizeRating(Runner.Skills.Composure) +
		0.02 * NormalizeRating(Runner.Skills.Luck));
	const double Confidence = std::clamp(Runner.Conditions.Confidence, 0.0, 1.0);
	const double Fatigue = std::clamp(Runner.Conditions.Fatigue, 0.0, 1.0);
	const double Injury = std::clamp(Runner.Conditions.Injury, 0.0, 1.0);
	const double ConditionMultiplier = (1.0 +
		(Confidence - Race::RacerConditions::InitialConfidence) * 0.1) *
		(1.0 - Fatigue * 0.45) * (1.0 - Injury * 0.35);
	return std::clamp(BaseStrength * ConditionMultiplier, 0.0, 100.0);
}

bool Game::Career::IsLeagueAtStableLocation(const CareerData& Data, const CareerLeague& League)
{
	const auto HomeCity = std::find_if(Data.Cities.begin(), Data.Cities.end(), [&Data](const CareerWorldCity& City)
		{
			return City.Id == Data.HomeCityId;
		});
	if (Data.HomeCityId == 0 || HomeCity == Data.Cities.end())
		return false;

	const auto HomeRegion = std::find_if(Data.Regions.begin(), Data.Regions.end(), [&HomeCity](const CareerWorldRegion& Region)
		{
			return Region.Id == HomeCity->RegionId;
		});
	if (HomeRegion == Data.Regions.end())
		return false;
	const auto HomeCountry = std::find_if(Data.Countries.begin(), Data.Countries.end(),
		[&HomeRegion](const CareerWorldCountry& Country) { return Country.Id == HomeRegion->CountryId; });
	if (HomeCountry == Data.Countries.end())
		return false;

	switch (League.Level)
	{
	case CareerCompetitionLevel::City:
		return League.CityId == HomeCity->Id;
	case CareerCompetitionLevel::Regional:
		return League.RegionId == HomeRegion->Id;
	case CareerCompetitionLevel::National:
		return League.CountryId == HomeCountry->Id;
	default:
		return false;
	}
}

Game::Career::CareerHistoricalRace* Game::Career::FindCareerHistoricalRace(
	CareerData& Data, std::uint64_t HistoryId)
{
	if (HistoryId == 0)
		return nullptr;
	const auto HistoricalRace = std::find_if(Data.HistoricalRaces.begin(), Data.HistoricalRaces.end(),
		[HistoryId](const CareerHistoricalRace& Entry) { return Entry.Id == HistoryId; });
	return HistoricalRace == Data.HistoricalRaces.end() ? nullptr : &*HistoricalRace;
}

const Game::Career::CareerHistoricalRace* Game::Career::FindCareerHistoricalRace(
	const CareerData& Data, std::uint64_t HistoryId)
{
	if (HistoryId == 0)
		return nullptr;
	const auto HistoricalRace = std::find_if(Data.HistoricalRaces.begin(), Data.HistoricalRaces.end(),
		[HistoryId](const CareerHistoricalRace& Entry) { return Entry.Id == HistoryId; });
	return HistoricalRace == Data.HistoricalRaces.end() ? nullptr : &*HistoricalRace;
}

bool Game::Career::RecordCareerRaceHistory(CareerData& Data, const Race::Race& CompletedRace,
	const std::string& EventName, std::uint64_t WorldTimeMs, std::uint64_t& HistoryId, std::string& Error)
{
	HistoryId = 0;
	Error.clear();
	if (Data.NextCareerRaceHistoryId == 0 ||
		Data.NextCareerRaceHistoryId == std::numeric_limits<std::uint64_t>::max())
	{
		Error = "Career race history identifiers are exhausted.";
		return false;
	}

	Race::RaceHistorySnapshot Snapshot;
	if (!CompletedRace.CaptureHistorySnapshot(Snapshot))
	{
		Error = "The completed race could not be captured for history.";
		return false;
	}

	CareerHistoricalRace HistoricalRace;
	HistoricalRace.Id = Data.NextCareerRaceHistoryId++;
	HistoricalRace.WorldTimeMs = WorldTimeMs;
	HistoricalRace.EventName = EventName;
	HistoricalRace.Snapshot = std::move(Snapshot);
	HistoryId = HistoricalRace.Id;
	Data.HistoricalRaces.push_back(std::move(HistoricalRace));
	if (Data.HistoricalRaces.size() > MaximumCareerHistoricalRaces)
		Data.HistoricalRaces.erase(Data.HistoricalRaces.begin(),
			Data.HistoricalRaces.begin() + static_cast<std::ptrdiff_t>(
				Data.HistoricalRaces.size() - MaximumCareerHistoricalRaces));
	return true;
}

std::shared_ptr<Game::Career::CareerRaceHistoryReplay> Game::Career::CreateCareerRaceHistoryReplay(
	const CareerHistoricalRace& HistoricalRace, std::string& Error)
{
	Error.clear();
	if (!Race::IsValidRaceHistorySnapshot(HistoricalRace.Snapshot))
	{
		Error = "The archived race snapshot is invalid.";
		return nullptr;
	}

	auto Replay = std::make_shared<CareerRaceHistoryReplay>(HistoricalRace.Snapshot.Seed);
	Replay->Entrants.reserve(HistoricalRace.Snapshot.Entrants.size());
	std::vector<Race::Racer*> EntrantPointers;
	EntrantPointers.reserve(HistoricalRace.Snapshot.Entrants.size());
	for (const Race::RaceHistoryRacerSnapshot& RacerSnapshot : HistoricalRace.Snapshot.Entrants)
	{
		auto Runner = std::make_unique<Race::Racer>(RacerSnapshot.Name);
		Runner->GUID = RacerSnapshot.GUID;
		Runner->Skills = RacerSnapshot.Skills;
		Runner->Details = RacerSnapshot.Details;
		Runner->Conditions = RacerSnapshot.Conditions;
		Runner->CurveSeed = RacerSnapshot.CurveSeed;
		Runner->AccelerationCurve = RacerSnapshot.AccelerationCurve;
		Runner->EnduranceCurve = RacerSnapshot.EnduranceCurve;
		Runner->Reset();
		Runner->Conditions = RacerSnapshot.Conditions;
		EntrantPointers.push_back(Runner.get());
		Replay->Entrants.push_back(std::move(Runner));
	}

	if (!Replay->CompletedRace.RestoreHistorySnapshot(HistoricalRace.Snapshot, EntrantPointers))
	{
		Error = "The archived race snapshot could not be restored.";
		return nullptr;
	}
	return Replay;
}

Game::Career::RacePrediction Game::Career::PredictRace(const std::vector<Race::Racer*>& Entrants,
	unsigned int TrackLength, unsigned int Seed, std::size_t SampleCount)
{
	RacePrediction Prediction;
	if (Entrants.size() < 2 || SampleCount == 0)
	{
		Prediction.Error = Race::RaceError::SimulationFailed;
		return Prediction;
	}
	if (TrackLength == 0)
	{
		Prediction.Error = Race::RaceError::ZeroLengthTrack;
		return Prediction;
	}
	if (std::find(Entrants.begin(), Entrants.end(), nullptr) != Entrants.end())
	{
		Prediction.Error = Race::RaceError::SimulationFailed;
		return Prediction;
	}

	Prediction.AverageFinishTimesMs.assign(Entrants.size(), 0.0);
	Prediction.WinProbabilities.assign(Entrants.size(), 0.0);
	for (std::size_t SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
	{
		std::vector<Race::Racer> SimulatedRacers;
		std::vector<Race::Racer*> SimulatedEntrants;
		SimulatedRacers.reserve(Entrants.size());
		SimulatedEntrants.reserve(Entrants.size());
		for (Race::Racer* Entrant : Entrants)
		{
			SimulatedRacers.push_back(*Entrant);
			SimulatedRacers.back().Reset();
			SimulatedEntrants.push_back(&SimulatedRacers.back());
		}

		Race::Race SimulatedRace(Seed + static_cast<unsigned int>(SampleIndex));
		if (SimulatedRace.ThisTrack == nullptr)
		{
			Prediction.Error = Race::RaceError::MissingTrack;
			return Prediction;
		}
		SimulatedRace.ThisTrack->Length = static_cast<Race::TrackLength>(TrackLength);
		SimulatedRace.Racers = SimulatedEntrants;
		const Race::RaceResult Result = SimulatedRace.Sim();
		if (Result.Error != Race::RaceError::None || Result.RacerResults.size() != Entrants.size())
		{
			Prediction.Error = Result.Error == Race::RaceError::None
				? Race::RaceError::SimulationFailed : Result.Error;
			return Prediction;
		}

		for (const Race::RacerRaceResult& RacerResult : Result.RacerResults)
		{
			const auto Entrant = std::find(SimulatedEntrants.begin(), SimulatedEntrants.end(), RacerResult.Racer);
			if (Entrant == SimulatedEntrants.end())
			{
				Prediction.Error = Race::RaceError::SimulationFailed;
				return Prediction;
			}

			const std::size_t EntrantIndex = static_cast<std::size_t>(Entrant - SimulatedEntrants.begin());
			Prediction.AverageFinishTimesMs[EntrantIndex] += RacerResult.Position == Race::PulledUpPosition
				? CareerRacePredictionDnfTimeMs : RacerResult.Ms;
			if (RacerResult.Position == Race::FirstPlaceRank)
				Prediction.WinProbabilities[EntrantIndex] += 1.0;
		}
	}

	for (std::size_t EntrantIndex = 0; EntrantIndex < Entrants.size(); ++EntrantIndex)
	{
		Prediction.AverageFinishTimesMs[EntrantIndex] /= static_cast<double>(SampleCount);
		Prediction.WinProbabilities[EntrantIndex] /= static_cast<double>(SampleCount);
	}
	return Prediction;
}

Game::Career::CareerOrchestrator::CareerOrchestrator(CareerData* _Data, CareerProfile* _Profile)
	: Data(_Data), Profile(_Profile)
{
}

std::vector<Game::Race::Racer*> Game::Career::CareerOrchestrator::SelectMatchedOpponents(
	std::size_t Count, unsigned int Seed) const
{
	if (Data == nullptr || Count == 0)
		return {};

	std::vector<Race::Racer*> Opponents = Data->Racers.GetAll();
	Opponents.erase(std::remove_if(Opponents.begin(), Opponents.end(), [this](const Race::Racer* Runner)
		{
			if (Runner == nullptr || std::find(Data->OwnedRacers.begin(), Data->OwnedRacers.end(), Runner) !=
				Data->OwnedRacers.end())
				return true;
			if (!Data->WorldSimulationInitialized)
				return false;
			const auto State = std::find_if(Data->RacerStates.begin(), Data->RacerStates.end(),
				[Runner](const CareerRacerState& Candidate) { return Candidate.Runner == Runner; });
			return State != Data->RacerStates.end() && State->Activity == CareerRacerActivity::Retired;
		}), Opponents.end());
	const std::uint64_t PlayerPositionPoints = Profile == nullptr ? 0 :
		Profile->MainFella.CareerRecord.PositionPoints;
	std::mt19937 Random(Seed);
	std::shuffle(Opponents.begin(), Opponents.end(), Random);
	std::stable_sort(Opponents.begin(), Opponents.end(), [PlayerPositionPoints](const Race::Racer* Left,
		const Race::Racer* Right)
		{
			const std::uint64_t LeftDistance = Left->CareerRecord.PositionPoints > PlayerPositionPoints
				? Left->CareerRecord.PositionPoints - PlayerPositionPoints
				: PlayerPositionPoints - Left->CareerRecord.PositionPoints;
			const std::uint64_t RightDistance = Right->CareerRecord.PositionPoints > PlayerPositionPoints
				? Right->CareerRecord.PositionPoints - PlayerPositionPoints
				: PlayerPositionPoints - Right->CareerRecord.PositionPoints;
			return LeftDistance < RightDistance;
		});
	if (Opponents.size() > Count)
		Opponents.resize(Count);
	return Opponents;
}

bool Game::Career::CareerOrchestrator::PrepareUpcomingRace(unsigned int Seed, unsigned int TrackLength)
{
	if (Data == nullptr || Profile == nullptr)
		return false;

	if (Data->UpcomingRaceEntrants.empty())
	{
		Data->UpcomingRaceEntrants.push_back(&Profile->MainFella);
		if (!Data->OwnedRacers.empty())
		{
			const std::size_t FirstOwnedRacerIndex = Seed % Data->OwnedRacers.size();
			for (std::size_t Offset = 0; Offset < Data->OwnedRacers.size() &&
				Data->UpcomingRaceEntrants.size() < CareerUpcomingRaceEntrantCount; ++Offset)
			{
				Race::Racer* OwnedRacer = Data->OwnedRacers[
					(FirstOwnedRacerIndex + Offset) % Data->OwnedRacers.size()];
				if (OwnedRacer != nullptr && std::find(Data->UpcomingRaceEntrants.begin(),
					Data->UpcomingRaceEntrants.end(), OwnedRacer) == Data->UpcomingRaceEntrants.end())
					Data->UpcomingRaceEntrants.push_back(OwnedRacer);
			}
		}
		const std::size_t OpponentCount = CareerUpcomingRaceEntrantCount - Data->UpcomingRaceEntrants.size();
		std::vector<Race::Racer*> Opponents = SelectMatchedOpponents(OpponentCount, Seed);
		Data->UpcomingRaceEntrants.insert(Data->UpcomingRaceEntrants.end(), Opponents.begin(), Opponents.end());
		Data->UpcomingRaceSeed = Seed;
	}
	if (Data->UpcomingRaceEntrants.size() < 2)
	{
		Data->UpcomingRaceEntrants.clear();
		return false;
	}

	if (Data->PendingBet.Active && Data->UpcomingRaceTrackLength != 0)
		return true;
	if (TrackLength != 0)
		Data->UpcomingRaceTrackLength = TrackLength;
	else if (Data->UpcomingRaceTrackLength == 0)
	{
		std::mt19937 Random(Seed);
		Data->UpcomingRaceTrackLength = SelectCareerTrackLength(Data, Random);
	}
	return Data->UpcomingRaceTrackLength != 0;
}

Game::Race::Race Game::Career::CareerOrchestrator::CreateUpcomingRace() const
{
	Game::Race::Race UpcomingRace(Data == nullptr ? 0 : Data->UpcomingRaceSeed);
	if (Data == nullptr)
		return UpcomingRace;

	UpcomingRace.Racers = Data->UpcomingRaceEntrants;
	if (UpcomingRace.ThisTrack != nullptr)
		UpcomingRace.ThisTrack->Length = static_cast<Race::TrackLength>(Data->UpcomingRaceTrackLength);
	UpcomingRace.Financials = Race::RaceFinancials(500, 0);
	return UpcomingRace;
}

Game::Race::Racer* Game::Career::CareerOrchestrator::BreedOwnedRacers(Race::Racer* FirstParent,
	Race::Racer* SecondParent, unsigned int Seed)
{
	if (Data == nullptr || FirstParent == nullptr || SecondParent == nullptr || FirstParent == SecondParent ||
		std::find(Data->OwnedRacers.begin(), Data->OwnedRacers.end(), FirstParent) == Data->OwnedRacers.end() ||
		std::find(Data->OwnedRacers.begin(), Data->OwnedRacers.end(), SecondParent) == Data->OwnedRacers.end())
		return nullptr;

	Race::Racer* Child = Data->Racers.Make(Race::RacerNameMaker().Make());
	std::mt19937 Random(Seed);
	std::normal_distribution<double> Mutation(0.0, 4.0);
	const auto InheritSkill = [&Random, &Mutation](double First, double Second)
		{
			return std::clamp((First + Second) * 0.5 + Mutation(Random), 0.0, 100.0);
		};
	Child->Skills.Xp = 0;
	Child->Skills.Reflex = InheritSkill(FirstParent->Skills.Reflex, SecondParent->Skills.Reflex);
	Child->Skills.Composure = InheritSkill(FirstParent->Skills.Composure, SecondParent->Skills.Composure);
	Child->Skills.Acceleration = InheritSkill(FirstParent->Skills.Acceleration, SecondParent->Skills.Acceleration);
	Child->Skills.Endurance = InheritSkill(FirstParent->Skills.Endurance, SecondParent->Skills.Endurance);
	Child->Skills.Speed = InheritSkill(FirstParent->Skills.Speed, SecondParent->Skills.Speed);
	Child->Skills.Potential = InheritSkill(FirstParent->Skills.Potential, SecondParent->Skills.Potential);
	Child->Skills.Trainability = InheritSkill(FirstParent->Skills.Trainability, SecondParent->Skills.Trainability);
	Child->Skills.Luck = InheritSkill(FirstParent->Skills.Luck, SecondParent->Skills.Luck);
	Child->Details.HeightCm = (FirstParent->Details.HeightCm + SecondParent->Details.HeightCm) * 0.5;
	Child->Details.WeightKg = (FirstParent->Details.WeightKg + SecondParent->Details.WeightKg) * 0.5;
	Child->Details.Age = 0;
	Child->Conditions = Race::RacerConditions{};
	Child->CareerRecord = Race::RacerCareerRecord{};
	Child->CurveSeed = Seed;
	Child->AccelerationCurve = Race::RaceCurve::Generate(Seed, false);
	Child->EnduranceCurve = Race::RaceCurve::Generate(Seed, true);
	Data->OwnedRacers.push_back(Child);
	return Child;
}

void Game::Career::CareerOrchestrator::CompleteUpcomingRace(const Race::RaceResult& Result,
	std::uint64_t HistoricalRaceId)
{
	if (Data == nullptr)
		return;
	CompletePlayerLeagueRace(Result, HistoricalRaceId);
	if (Result.Error != Race::RaceError::None)
		return;

	if (Data->PendingBet.Active && Profile != nullptr && Data->PendingBet.SelectedRacer != nullptr)
	{
		const auto Winner = std::find_if(Result.RacerResults.begin(), Result.RacerResults.end(), [this](const auto& Entry)
			{
				return Entry.Racer == Data->PendingBet.SelectedRacer && Entry.Position == Race::FirstPlaceRank;
			});
		if (Winner != Result.RacerResults.end())
		{
			const long long Payout = std::llround(Data->PendingBet.Stake * Data->PendingBet.DecimalOdds);
			const long long UpdatedCash = static_cast<long long>(Profile->Financials.Cash) + Payout;
			Profile->Financials.Cash = static_cast<int>(std::clamp(UpdatedCash,
				static_cast<long long>(std::numeric_limits<int>::min()),
				static_cast<long long>(std::numeric_limits<int>::max())));
		}
	}

	Data->PendingBet = CareerPendingBet{};
	Data->UpcomingRaceEntrants.clear();
	Data->UpcomingRaceSeed = 0;
	Data->UpcomingRaceTrackLength = 0;
}

Game::Career::CareerData::CareerData()
{
}

Game::Career::CareerData::~CareerData() = default;

Game::Career::CareerProfile::CareerProfile()
{
	std::seed_seq Seed{ static_cast<unsigned int>(std::rand()), static_cast<unsigned int>(std::rand()),
		static_cast<unsigned int>(std::rand()), static_cast<unsigned int>(std::rand()) };
	std::mt19937_64 Generator(Seed);
	std::uniform_int_distribution<unsigned int> AccuracyDistribution(1, 99);
	AppraisalIdentity = Generator();
	Accuracy = static_cast<double>(AccuracyDistribution(Generator));
}

// The shared-core project has an explicit compile list, so persistence is included in this translation unit.
#include "CareerSave.cpp"
