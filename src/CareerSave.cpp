#include "Career.h"
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <locale>
#include <random>
#include <sstream>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <utility>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

namespace CareerSaveDetail
{
	using Game::Career::CareerCompetitionLevel;
	using Game::Career::CareerData;
	using Game::Career::CareerFixtureState;
	using Game::Career::CareerHistoricalRace;
	using Game::Career::CareerLeague;
	using Game::Career::CareerLeagueDivision;
	using Game::Career::CareerLeagueFixture;
	using Game::Career::CareerLeagueStanding;
	using Game::Career::CareerLeagueState;
	using Game::Career::CareerProfile;
	using Game::Career::CareerRacerActivity;
	using Game::Career::CareerRacerState;
	using Game::Career::CityPlot;
	using Game::Career::CityPlotType;
	using Game::Career::CareerWorldCity;
	using Game::Career::CareerWorldCountry;
	using Game::Career::CareerWorldRegion;
	using Game::Race::Racer;
	using Game::Race::RacerCareerHistoryEntry;
	using Game::Race::RaceHistoryRacerSnapshot;
	using Game::Race::RaceHistoryResultSnapshot;
	using Game::Race::RaceHistorySnapshot;
	using Game::Race::RaceHistoryTimelineSnapshot;

	constexpr std::array<char, 8> CareerSaveMagic{ 'S', 'R', 'S', 'A', 'V', 'E', '\r', '\n' };
	constexpr std::uint64_t MaximumCareerSavePayloadBytes = 256ull * 1024ull * 1024ull;
	constexpr std::uint32_t MaximumCareerSaveCollectionCount = 1000000;
	constexpr std::uint32_t MaximumCareerSaveStringBytes = 1024u * 1024u;

	class CareerSaveWriter
	{
	public:
		std::string Bytes;
		std::string Error;

		bool WriteU8(std::uint8_t Value)
		{
			const char Byte = static_cast<char>(Value);
			return Append(&Byte, sizeof(Byte));
		}

		bool WriteBool(bool Value)
		{
			return WriteU8(Value ? 1u : 0u);
		}

		bool WriteU32(std::uint32_t Value)
		{
			std::array<char, 4> Encoded{};
			for (std::size_t Index = 0; Index < Encoded.size(); ++Index)
				Encoded[Index] = static_cast<char>((Value >> (Index * 8)) & 0xffu);
			return Append(Encoded.data(), Encoded.size());
		}

		bool WriteU64(std::uint64_t Value)
		{
			std::array<char, 8> Encoded{};
			for (std::size_t Index = 0; Index < Encoded.size(); ++Index)
				Encoded[Index] = static_cast<char>((Value >> (Index * 8)) & 0xffu);
			return Append(Encoded.data(), Encoded.size());
		}

		bool WriteI32(std::int32_t Value)
		{
			return WriteU32(std::bit_cast<std::uint32_t>(Value));
		}

		bool WriteFloat(float Value)
		{
			if (!std::isfinite(Value))
				return Fail("Career save contains a non-finite floating-point value.");
			return WriteU32(std::bit_cast<std::uint32_t>(Value));
		}

		bool WriteDouble(double Value)
		{
			if (!std::isfinite(Value))
				return Fail("Career save contains a non-finite floating-point value.");
			return WriteU64(std::bit_cast<std::uint64_t>(Value));
		}

		bool WriteString(const std::string& Value)
		{
			if (Value.size() > MaximumCareerSaveStringBytes)
				return Fail("A career-save string exceeds the supported size limit.");
			if (!WriteU32(static_cast<std::uint32_t>(Value.size())))
				return false;
			return Value.empty() || Append(Value.data(), Value.size());
		}

		bool WriteCount(std::size_t Count)
		{
			if (Count > MaximumCareerSaveCollectionCount)
				return Fail("A career-save collection exceeds the supported item limit.");
			return WriteU32(static_cast<std::uint32_t>(Count));
		}

		bool WriteSize(std::size_t Value)
		{
			if constexpr (sizeof(std::size_t) > sizeof(std::uint64_t))
				if (Value > std::numeric_limits<std::uint64_t>::max())
					return Fail("A career-save index exceeds the supported size range.");
			return WriteU64(static_cast<std::uint64_t>(Value));
		}

		bool Fail(const std::string& Message)
		{
			if (Error.empty())
				Error = Message;
			return false;
		}

	private:
		bool Append(const char* Source, std::size_t Size)
		{
			if (Size > MaximumCareerSavePayloadBytes - Bytes.size())
				return Fail("Career save exceeds the supported payload size limit.");
			if (Size != 0)
				Bytes.append(Source, Size);
			return true;
		}
	};

	class CareerSaveReader
	{
	public:
		explicit CareerSaveReader(std::string_view Bytes)
			: Bytes(Bytes)
		{
		}

		std::string Error;

		bool ReadU8(std::uint8_t& Value)
		{
			char Byte = 0;
			if (!ReadRaw(&Byte, sizeof(Byte)))
				return false;
			Value = static_cast<std::uint8_t>(static_cast<unsigned char>(Byte));
			return true;
		}

		bool ReadBool(bool& Value)
		{
			std::uint8_t Encoded = 0;
			if (!ReadU8(Encoded))
				return false;
			if (Encoded > 1)
				return Fail("Career save contains an invalid boolean value.");
			Value = Encoded != 0;
			return true;
		}

		bool ReadU32(std::uint32_t& Value)
		{
			std::array<unsigned char, 4> Encoded{};
			if (!ReadRaw(reinterpret_cast<char*>(Encoded.data()), Encoded.size()))
				return false;
			Value = 0;
			for (std::size_t Index = 0; Index < Encoded.size(); ++Index)
				Value |= static_cast<std::uint32_t>(Encoded[Index]) << (Index * 8);
			return true;
		}

		bool ReadU64(std::uint64_t& Value)
		{
			std::array<unsigned char, 8> Encoded{};
			if (!ReadRaw(reinterpret_cast<char*>(Encoded.data()), Encoded.size()))
				return false;
			Value = 0;
			for (std::size_t Index = 0; Index < Encoded.size(); ++Index)
				Value |= static_cast<std::uint64_t>(Encoded[Index]) << (Index * 8);
			return true;
		}

		bool ReadI32(std::int32_t& Value)
		{
			std::uint32_t Encoded = 0;
			if (!ReadU32(Encoded))
				return false;
			Value = std::bit_cast<std::int32_t>(Encoded);
			return true;
		}

		bool ReadFloat(float& Value)
		{
			std::uint32_t Encoded = 0;
			if (!ReadU32(Encoded))
				return false;
			Value = std::bit_cast<float>(Encoded);
			if (!std::isfinite(Value))
				return Fail("Career save contains a non-finite floating-point value.");
			return true;
		}

		bool ReadDouble(double& Value)
		{
			std::uint64_t Encoded = 0;
			if (!ReadU64(Encoded))
				return false;
			Value = std::bit_cast<double>(Encoded);
			if (!std::isfinite(Value))
				return Fail("Career save contains a non-finite floating-point value.");
			return true;
		}

		bool ReadString(std::string& Value)
		{
			std::uint32_t Size = 0;
			if (!ReadU32(Size))
				return false;
			if (Size > MaximumCareerSaveStringBytes || Size > Remaining())
				return Fail("Career save contains an invalid string size.");
			Value.assign(Bytes.data() + Offset, Size);
			Offset += Size;
			return true;
		}

		bool ReadCount(std::uint32_t& Count)
		{
			if (!ReadU32(Count))
				return false;
			if (Count > MaximumCareerSaveCollectionCount || Count > Remaining())
				return Fail("Career save contains an invalid collection size.");
			return true;
		}

		bool ReadSize(std::size_t& Value)
		{
			std::uint64_t Encoded = 0;
			if (!ReadU64(Encoded))
				return false;
			if (Encoded > std::numeric_limits<std::size_t>::max())
				return Fail("Career save contains an index outside the supported size range.");
			Value = static_cast<std::size_t>(Encoded);
			return true;
		}

		std::size_t Remaining() const
		{
			return Bytes.size() - Offset;
		}

		bool AtEnd() const
		{
			return Offset == Bytes.size();
		}

		bool Fail(const std::string& Message)
		{
			if (Error.empty())
				Error = Message;
			return false;
		}

	private:
		bool ReadRaw(char* Destination, std::size_t Size)
		{
			if (Size > Remaining())
				return Fail("Career save data is truncated.");
			if (Size != 0)
				std::memcpy(Destination, Bytes.data() + Offset, Size);
			Offset += Size;
			return true;
		}

		std::string_view Bytes;
		std::size_t Offset = 0;
	};

	std::uint64_t CalculateChecksum(std::string_view Bytes)
	{
		std::uint64_t Checksum = 14695981039346656037ull;
		for (const unsigned char Byte : Bytes)
		{
			Checksum ^= Byte;
			Checksum *= 1099511628211ull;
		}
		return Checksum;
	}

	std::uint64_t DecodeUnsigned(const char* Bytes, std::size_t Count)
	{
		std::uint64_t Value = 0;
		for (std::size_t Index = 0; Index < Count; ++Index)
			Value |= static_cast<std::uint64_t>(static_cast<unsigned char>(Bytes[Index])) << (Index * 8);
		return Value;
	}

	template <typename Enum>
	bool WriteEnum(CareerSaveWriter& Writer, Enum Value)
	{
		return Writer.WriteU32(static_cast<std::uint32_t>(Value));
	}

	template <typename Enum>
	bool ReadEnum(CareerSaveReader& Reader, Enum& Value, std::uint32_t MaximumValue)
	{
		std::uint32_t Encoded = 0;
		if (!Reader.ReadU32(Encoded))
			return false;
		if (Encoded > MaximumValue)
			return Reader.Fail("Career save contains an invalid enum value.");
		Value = static_cast<Enum>(Encoded);
		return true;
	}

	template <typename Item, typename WriteItem>
	bool WriteVector(CareerSaveWriter& Writer, const std::vector<Item>& Values, WriteItem WriteItemValue)
	{
		if (!Writer.WriteCount(Values.size()))
			return false;
		for (const Item& Value : Values)
			if (!WriteItemValue(Writer, Value))
				return false;
		return true;
	}

	template <typename Item, typename ReadItem>
	bool ReadVector(CareerSaveReader& Reader, std::vector<Item>& Values, ReadItem ReadItemValue)
	{
		std::uint32_t Count = 0;
		if (!Reader.ReadCount(Count))
			return false;
		Values.clear();
		Values.reserve(Count);
		for (std::uint32_t Index = 0; Index < Count; ++Index)
		{
			Item Value{};
			if (!ReadItemValue(Reader, Value))
				return false;
			Values.push_back(std::move(Value));
		}
		return true;
	}

	bool WriteRaceCurve(CareerSaveWriter& Writer, const Game::Race::RaceCurve& Curve)
	{
		return Writer.WriteDouble(Curve.X1) && Writer.WriteDouble(Curve.Y1) &&
			Writer.WriteDouble(Curve.X2) && Writer.WriteDouble(Curve.Y2) && Writer.WriteBool(Curve.Descending);
	}

	bool ReadRaceCurve(CareerSaveReader& Reader, Game::Race::RaceCurve& Curve)
	{
		return Reader.ReadDouble(Curve.X1) && Reader.ReadDouble(Curve.Y1) &&
			Reader.ReadDouble(Curve.X2) && Reader.ReadDouble(Curve.Y2) && Reader.ReadBool(Curve.Descending);
	}

	bool WriteRaceHistoryRacer(CareerSaveWriter& Writer, const RaceHistoryRacerSnapshot& Snapshot)
	{
		return Writer.WriteU32(Snapshot.GUID.GUID) && Writer.WriteString(Snapshot.Name) &&
			Writer.WriteU64(Snapshot.Skills.Xp) && Writer.WriteDouble(Snapshot.Skills.Reflex) &&
			Writer.WriteDouble(Snapshot.Skills.Composure) && Writer.WriteDouble(Snapshot.Skills.Acceleration) &&
			Writer.WriteDouble(Snapshot.Skills.Endurance) && Writer.WriteDouble(Snapshot.Skills.Speed) &&
			Writer.WriteDouble(Snapshot.Skills.Potential) && Writer.WriteDouble(Snapshot.Skills.Trainability) &&
			Writer.WriteDouble(Snapshot.Skills.Luck) && Writer.WriteDouble(Snapshot.Details.HeightCm) &&
			Writer.WriteDouble(Snapshot.Details.WeightKg) && Writer.WriteU32(Snapshot.Details.Age) &&
			Writer.WriteDouble(Snapshot.Conditions.Fatigue) && Writer.WriteDouble(Snapshot.Conditions.Confidence) &&
			Writer.WriteDouble(Snapshot.Conditions.Injury) && Writer.WriteU32(Snapshot.CurveSeed) &&
			WriteRaceCurve(Writer, Snapshot.AccelerationCurve) && WriteRaceCurve(Writer, Snapshot.EnduranceCurve);
	}

	bool ReadRaceHistoryRacer(CareerSaveReader& Reader, RaceHistoryRacerSnapshot& Snapshot)
	{
		return Reader.ReadU32(Snapshot.GUID.GUID) && Reader.ReadString(Snapshot.Name) &&
			Reader.ReadU64(Snapshot.Skills.Xp) && Reader.ReadDouble(Snapshot.Skills.Reflex) &&
			Reader.ReadDouble(Snapshot.Skills.Composure) && Reader.ReadDouble(Snapshot.Skills.Acceleration) &&
			Reader.ReadDouble(Snapshot.Skills.Endurance) && Reader.ReadDouble(Snapshot.Skills.Speed) &&
			Reader.ReadDouble(Snapshot.Skills.Potential) && Reader.ReadDouble(Snapshot.Skills.Trainability) &&
			Reader.ReadDouble(Snapshot.Skills.Luck) && Reader.ReadDouble(Snapshot.Details.HeightCm) &&
			Reader.ReadDouble(Snapshot.Details.WeightKg) && Reader.ReadU32(Snapshot.Details.Age) &&
			Reader.ReadDouble(Snapshot.Conditions.Fatigue) && Reader.ReadDouble(Snapshot.Conditions.Confidence) &&
			Reader.ReadDouble(Snapshot.Conditions.Injury) && Reader.ReadU32(Snapshot.CurveSeed) &&
			ReadRaceCurve(Reader, Snapshot.AccelerationCurve) && ReadRaceCurve(Reader, Snapshot.EnduranceCurve);
	}

	bool WriteRaceHistoryTimeline(CareerSaveWriter& Writer, const RaceHistoryTimelineSnapshot& Timeline)
	{
		if (!Writer.WriteU32(Timeline.RacerIndex) || !Writer.WriteDouble(Timeline.PreRaceConfidence) ||
			!Writer.WriteDouble(Timeline.PreRaceFatigue) || !Writer.WriteDouble(Timeline.PreRaceInjury) ||
			!Writer.WriteCount(Timeline.Nodes.size()))
			return false;
		for (const Game::Race::RaceTimelineNode& Node : Timeline.Nodes)
			if (!WriteEnum(Writer, Node.Type) || !Writer.WriteDouble(Node.StartTimeMs) ||
				!Writer.WriteDouble(Node.EndTimeMs) || !Writer.WriteDouble(Node.Distance) ||
				!Writer.WriteDouble(Node.StartPace) || !Writer.WriteDouble(Node.EndPace) ||
				!Writer.WriteDouble(Node.AccelerationCurveTimeOffsetMs) || !WriteEnum(Writer, Node.Event.Type) ||
				!Writer.WriteU32(Node.Event.RacerIndex) || !Writer.WriteDouble(Node.Event.TimeMs) ||
				!Writer.WriteU32(Node.Event.DurationMs) || !Writer.WriteDouble(Node.Event.Magnitude))
				return false;
		if (!Writer.WriteCount(Timeline.MotionSegments.size()))
			return false;
		for (const Game::Race::RaceMotionSegment& Segment : Timeline.MotionSegments)
			if (!Writer.WriteDouble(Segment.StartTimeMs) || !Writer.WriteDouble(Segment.EndTimeMs) ||
				!Writer.WriteDouble(Segment.StartDistance))
				return false;
		return Writer.WriteDouble(Timeline.FinalDistance) && Writer.WriteDouble(Timeline.FinishTimeMs) &&
			Writer.WriteDouble(Timeline.PullUpTimeMs) && Writer.WriteDouble(Timeline.PullUpDistance) &&
			Writer.WriteU32(Timeline.FinishPosition) && Writer.WriteBool(Timeline.Finished) &&
			Writer.WriteBool(Timeline.PulledUp) && Writer.WriteDouble(Timeline.FatigueAfter) &&
			Writer.WriteDouble(Timeline.ConfidenceAfter) && Writer.WriteDouble(Timeline.InjuryAfter) &&
			Writer.WriteDouble(Timeline.CurrentEndurance) && Writer.WriteU32(Timeline.ActiveEffects);
	}

	bool ReadRaceHistoryTimeline(CareerSaveReader& Reader, RaceHistoryTimelineSnapshot& Timeline)
	{
		if (!Reader.ReadU32(Timeline.RacerIndex) || !Reader.ReadDouble(Timeline.PreRaceConfidence) ||
			!Reader.ReadDouble(Timeline.PreRaceFatigue) || !Reader.ReadDouble(Timeline.PreRaceInjury))
			return false;
		std::uint32_t NodeCount = 0;
		if (!Reader.ReadCount(NodeCount))
			return false;
		for (std::uint32_t Index = 0; Index < NodeCount; ++Index)
		{
			Game::Race::RaceTimelineNode Node;
			if (!ReadEnum(Reader, Node.Type, static_cast<std::uint32_t>(Game::Race::RaceTimelineNodeType::Finish)) ||
				!Reader.ReadDouble(Node.StartTimeMs) || !Reader.ReadDouble(Node.EndTimeMs) ||
				!Reader.ReadDouble(Node.Distance) || !Reader.ReadDouble(Node.StartPace) ||
				!Reader.ReadDouble(Node.EndPace) || !Reader.ReadDouble(Node.AccelerationCurveTimeOffsetMs) ||
				!ReadEnum(Reader, Node.Event.Type, static_cast<std::uint32_t>(Game::Race::RaceEventType::PullingUp)) ||
				!Reader.ReadU32(Node.Event.RacerIndex) || !Reader.ReadDouble(Node.Event.TimeMs) ||
				!Reader.ReadU32(Node.Event.DurationMs) || !Reader.ReadDouble(Node.Event.Magnitude))
				return false;
			Timeline.Nodes.push_back(std::move(Node));
		}
		std::uint32_t MotionSegmentCount = 0;
		if (!Reader.ReadCount(MotionSegmentCount))
			return false;
		Timeline.MotionSegments.reserve(MotionSegmentCount);
		for (std::uint32_t Index = 0; Index < MotionSegmentCount; ++Index)
		{
			Game::Race::RaceMotionSegment Segment;
			if (!Reader.ReadDouble(Segment.StartTimeMs) || !Reader.ReadDouble(Segment.EndTimeMs) ||
				!Reader.ReadDouble(Segment.StartDistance))
				return false;
			Timeline.MotionSegments.push_back(Segment);
		}
		return Reader.ReadDouble(Timeline.FinalDistance) && Reader.ReadDouble(Timeline.FinishTimeMs) &&
			Reader.ReadDouble(Timeline.PullUpTimeMs) && Reader.ReadDouble(Timeline.PullUpDistance) &&
			Reader.ReadU32(Timeline.FinishPosition) && Reader.ReadBool(Timeline.Finished) &&
			Reader.ReadBool(Timeline.PulledUp) && Reader.ReadDouble(Timeline.FatigueAfter) &&
			Reader.ReadDouble(Timeline.ConfidenceAfter) && Reader.ReadDouble(Timeline.InjuryAfter) &&
			Reader.ReadDouble(Timeline.CurrentEndurance) && Reader.ReadU32(Timeline.ActiveEffects);
	}

	bool WriteRaceHistoryResult(CareerSaveWriter& Writer, const RaceHistoryResultSnapshot& Result)
	{
		return Writer.WriteU32(Result.RacerIndex) && Writer.WriteU32(Result.Position) && Writer.WriteU32(Result.Ms);
	}

	bool ReadRaceHistoryResult(CareerSaveReader& Reader, RaceHistoryResultSnapshot& Result)
	{
		return Reader.ReadU32(Result.RacerIndex) && Reader.ReadU32(Result.Position) && Reader.ReadU32(Result.Ms);
	}

	bool WriteRaceHistorySnapshot(CareerSaveWriter& Writer, const RaceHistorySnapshot& Snapshot)
	{
		if (!Game::Race::IsValidRaceHistorySnapshot(Snapshot))
			return Writer.Fail("Career save contains an invalid archived race snapshot.");
		return Writer.WriteU32(Snapshot.GUID.GUID) && Writer.WriteU32(Snapshot.Seed) &&
			Writer.WriteU32(Snapshot.TrackLength) && Writer.WriteU32(Snapshot.CurrentTick) &&
			WriteEnum(Writer, Snapshot.Error) && Writer.WriteU32(Snapshot.Financials.FirstPlacePrize) &&
			Writer.WriteU32(Snapshot.Financials.SecondPlacePrize) &&
			Writer.WriteU32(Snapshot.Financials.ThirdPlacePrize) &&
			Writer.WriteU32(Snapshot.Financials.EntranceFee) &&
			Writer.WriteDouble(Snapshot.PlaybackTimeMs) && Writer.WriteDouble(Snapshot.TimelineDurationMs) &&
			WriteVector(Writer, Snapshot.Entrants, WriteRaceHistoryRacer) &&
			WriteVector(Writer, Snapshot.Timelines, WriteRaceHistoryTimeline) &&
			WriteVector(Writer, Snapshot.Results, WriteRaceHistoryResult);
	}

	bool ReadRaceHistorySnapshot(CareerSaveReader& Reader, RaceHistorySnapshot& Snapshot)
	{
		if (!Reader.ReadU32(Snapshot.GUID.GUID) || !Reader.ReadU32(Snapshot.Seed) ||
			!Reader.ReadU32(Snapshot.TrackLength) || !Reader.ReadU32(Snapshot.CurrentTick) ||
			!ReadEnum(Reader, Snapshot.Error, static_cast<std::uint32_t>(Game::Race::RaceError::SimulationFailed)) ||
			!Reader.ReadU32(Snapshot.Financials.FirstPlacePrize) ||
			!Reader.ReadU32(Snapshot.Financials.SecondPlacePrize) ||
			!Reader.ReadU32(Snapshot.Financials.ThirdPlacePrize) ||
			!Reader.ReadU32(Snapshot.Financials.EntranceFee) ||
			!Reader.ReadDouble(Snapshot.PlaybackTimeMs) || !Reader.ReadDouble(Snapshot.TimelineDurationMs) ||
			!ReadVector(Reader, Snapshot.Entrants, ReadRaceHistoryRacer) ||
			!ReadVector(Reader, Snapshot.Timelines, ReadRaceHistoryTimeline) ||
			!ReadVector(Reader, Snapshot.Results, ReadRaceHistoryResult))
			return false;
		return Game::Race::IsValidRaceHistorySnapshot(Snapshot) ||
			Reader.Fail("Career save contains an invalid archived race snapshot.");
	}

	bool WriteCareerHistoricalRace(CareerSaveWriter& Writer, const CareerHistoricalRace& HistoricalRace)
	{
		return Writer.WriteU64(HistoricalRace.Id) && Writer.WriteU64(HistoricalRace.WorldTimeMs) &&
			Writer.WriteU64(HistoricalRace.WorldRaceId) && Writer.WriteU64(HistoricalRace.LeagueId) &&
			Writer.WriteSize(HistoricalRace.PlayoffRoundIndex) &&
			Writer.WriteSize(HistoricalRace.PlayoffMatchIndex) && Writer.WriteString(HistoricalRace.EventName) &&
			Writer.WriteBool(HistoricalRace.IsPlayoff) && WriteRaceHistorySnapshot(Writer, HistoricalRace.Snapshot);
	}

	bool ReadCareerHistoricalRace(CareerSaveReader& Reader, CareerHistoricalRace& HistoricalRace)
	{
		return Reader.ReadU64(HistoricalRace.Id) && Reader.ReadU64(HistoricalRace.WorldTimeMs) &&
			Reader.ReadU64(HistoricalRace.WorldRaceId) && Reader.ReadU64(HistoricalRace.LeagueId) &&
			Reader.ReadSize(HistoricalRace.PlayoffRoundIndex) &&
			Reader.ReadSize(HistoricalRace.PlayoffMatchIndex) && Reader.ReadString(HistoricalRace.EventName) &&
			Reader.ReadBool(HistoricalRace.IsPlayoff) && ReadRaceHistorySnapshot(Reader, HistoricalRace.Snapshot);
	}

	bool WriteRacerHistoryEntry(CareerSaveWriter& Writer, const RacerCareerHistoryEntry& Entry)
	{
		return Writer.WriteU64(Entry.WorldTimeMs) && Writer.WriteString(Entry.EventName) &&
			Writer.WriteU32(Entry.TrackLength) && Writer.WriteU32(Entry.Position) &&
			Writer.WriteU32(Entry.FinishTimeMs) && Writer.WriteU64(Entry.HistoricalRaceId);
	}

	bool ReadRacerHistoryEntry(CareerSaveReader& Reader, RacerCareerHistoryEntry& Entry)
	{
		return Reader.ReadU64(Entry.WorldTimeMs) && Reader.ReadString(Entry.EventName) &&
			Reader.ReadU32(Entry.TrackLength) && Reader.ReadU32(Entry.Position) &&
			Reader.ReadU32(Entry.FinishTimeMs) && Reader.ReadU64(Entry.HistoricalRaceId);
	}

	bool WriteRacer(CareerSaveWriter& Writer, const Racer& Runner)
	{
		return Writer.WriteU32(Runner.GUID.GUID) && Writer.WriteString(Runner.Name) &&
			Writer.WriteU64(Runner.Skills.Xp) && Writer.WriteDouble(Runner.Skills.Reflex) &&
			Writer.WriteDouble(Runner.Skills.Composure) && Writer.WriteDouble(Runner.Skills.Acceleration) &&
			Writer.WriteDouble(Runner.Skills.Endurance) && Writer.WriteDouble(Runner.Skills.Speed) &&
			Writer.WriteDouble(Runner.Skills.Potential) && Writer.WriteDouble(Runner.Skills.Trainability) &&
			Writer.WriteDouble(Runner.Skills.Luck) && Writer.WriteDouble(Runner.Details.HeightCm) &&
			Writer.WriteDouble(Runner.Details.WeightKg) && Writer.WriteU32(Runner.Details.Age) &&
			Writer.WriteDouble(Runner.Conditions.Fatigue) && Writer.WriteDouble(Runner.Conditions.Confidence) &&
			Writer.WriteDouble(Runner.Conditions.Injury) && Writer.WriteU64(Runner.CareerRecord.Wins) &&
			Writer.WriteU64(Runner.CareerRecord.Losses) && Writer.WriteU64(Runner.CareerRecord.RacesEntered) &&
			Writer.WriteU64(Runner.CareerRecord.PositionPoints) && Writer.WriteU32(Runner.CareerRecord.BestTimeMs) &&
			WriteVector(Writer, Runner.CareerRecord.History, WriteRacerHistoryEntry) &&
			Writer.WriteU32(Runner.CurveSeed);
	}

	bool ReadRacer(CareerSaveReader& Reader, Racer& Runner)
	{
		std::uint32_t Guid = 0;
		std::string Name;
		if (!Reader.ReadU32(Guid) || !Reader.ReadString(Name))
			return false;

		Racer LoadedRunner(std::move(Name));
		LoadedRunner.GUID.GUID = Guid;
		if (!Reader.ReadU64(LoadedRunner.Skills.Xp) || !Reader.ReadDouble(LoadedRunner.Skills.Reflex) ||
			!Reader.ReadDouble(LoadedRunner.Skills.Composure) || !Reader.ReadDouble(LoadedRunner.Skills.Acceleration) ||
			!Reader.ReadDouble(LoadedRunner.Skills.Endurance) || !Reader.ReadDouble(LoadedRunner.Skills.Speed) ||
			!Reader.ReadDouble(LoadedRunner.Skills.Potential) || !Reader.ReadDouble(LoadedRunner.Skills.Trainability) ||
			!Reader.ReadDouble(LoadedRunner.Skills.Luck) || !Reader.ReadDouble(LoadedRunner.Details.HeightCm) ||
			!Reader.ReadDouble(LoadedRunner.Details.WeightKg) || !Reader.ReadU32(LoadedRunner.Details.Age) ||
			!Reader.ReadDouble(LoadedRunner.Conditions.Fatigue) || !Reader.ReadDouble(LoadedRunner.Conditions.Confidence) ||
			!Reader.ReadDouble(LoadedRunner.Conditions.Injury) || !Reader.ReadU64(LoadedRunner.CareerRecord.Wins) ||
			!Reader.ReadU64(LoadedRunner.CareerRecord.Losses) || !Reader.ReadU64(LoadedRunner.CareerRecord.RacesEntered) ||
			!Reader.ReadU64(LoadedRunner.CareerRecord.PositionPoints) ||
			!Reader.ReadU32(LoadedRunner.CareerRecord.BestTimeMs) ||
			!ReadVector(Reader, LoadedRunner.CareerRecord.History, ReadRacerHistoryEntry) ||
			!Reader.ReadU32(LoadedRunner.CurveSeed))
			return false;

		LoadedRunner.AccelerationCurve = Game::Race::RaceCurve::Generate(LoadedRunner.CurveSeed, false);
		LoadedRunner.EnduranceCurve = Game::Race::RaceCurve::Generate(LoadedRunner.CurveSeed, true);
		LoadedRunner.Reset();
		Runner = std::move(LoadedRunner);
		return true;
	}

	bool WriteCareerProfile(CareerSaveWriter& Writer, const CareerProfile& Profile)
	{
		return WriteRacer(Writer, Profile.MainFella) && Writer.WriteI32(Profile.Financials.Cash) &&
			Writer.WriteU64(Profile.AppraisalIdentity) && Writer.WriteDouble(Profile.Accuracy);
	}

	bool ReadCareerProfile(CareerSaveReader& Reader, CareerProfile& Profile)
	{
		std::int32_t Cash = 0;
		if (!ReadRacer(Reader, Profile.MainFella) || !Reader.ReadI32(Cash) ||
			!Reader.ReadU64(Profile.AppraisalIdentity) || !Reader.ReadDouble(Profile.Accuracy))
			return false;
		Profile.Financials.Cash = Cash;
		return true;
	}

	bool WriteCityPlot(CareerSaveWriter& Writer, const CityPlot& Plot)
	{
		return Writer.WriteU32(Plot.GridX) && Writer.WriteU32(Plot.GridY) &&
			WriteEnum(Writer, Plot.Type) && Writer.WriteString(Plot.Name) &&
			Writer.WriteU32(Plot.TrackLength) && Writer.WriteU32(Plot.SpriteIndex);
	}

	bool ReadCityPlot(CareerSaveReader& Reader, CityPlot& Plot)
	{
		return Reader.ReadU32(Plot.GridX) && Reader.ReadU32(Plot.GridY) &&
			ReadEnum(Reader, Plot.Type, static_cast<std::uint32_t>(CityPlotType::Stable)) &&
			Reader.ReadString(Plot.Name) && Reader.ReadU32(Plot.TrackLength) &&
			Reader.ReadU32(Plot.SpriteIndex);
	}

	bool WriteWorldCity(CareerSaveWriter& Writer, const CareerWorldCity& City)
	{
		return Writer.WriteU64(City.Id) && Writer.WriteU64(City.RegionId) &&
			Writer.WriteString(City.Name) && Writer.WriteU64(City.Population) &&
			Writer.WriteFloat(City.MapX) && Writer.WriteFloat(City.MapY) &&
			Writer.WriteBool(City.IsCapital) &&
			WriteVector(Writer, City.Plots, WriteCityPlot);
	}

	bool ReadWorldCity(CareerSaveReader& Reader, CareerWorldCity& City)
	{
		return Reader.ReadU64(City.Id) && Reader.ReadU64(City.RegionId) &&
			Reader.ReadString(City.Name) && Reader.ReadU64(City.Population) &&
			Reader.ReadFloat(City.MapX) && Reader.ReadFloat(City.MapY) &&
			Reader.ReadBool(City.IsCapital) && ReadVector(Reader, City.Plots, ReadCityPlot);
	}

	bool WriteWorldRegion(CareerSaveWriter& Writer, const CareerWorldRegion& Region)
	{
		return Writer.WriteU64(Region.Id) && Writer.WriteU64(Region.CountryId) &&
			Writer.WriteU64(Region.CapitalCityId) && Writer.WriteString(Region.Name) &&
			Writer.WriteU64(Region.Population) && Writer.WriteFloat(Region.MapX) &&
			Writer.WriteFloat(Region.MapY) &&
			WriteVector(Writer, Region.CityIds, [](CareerSaveWriter& ItemWriter, std::uint64_t Id)
			{
				return ItemWriter.WriteU64(Id);
			});
	}

	bool ReadWorldRegion(CareerSaveReader& Reader, CareerWorldRegion& Region)
	{
		return Reader.ReadU64(Region.Id) && Reader.ReadU64(Region.CountryId) &&
			Reader.ReadU64(Region.CapitalCityId) && Reader.ReadString(Region.Name) &&
			Reader.ReadU64(Region.Population) && Reader.ReadFloat(Region.MapX) &&
			Reader.ReadFloat(Region.MapY) &&
			ReadVector(Reader, Region.CityIds, [](CareerSaveReader& ItemReader, std::uint64_t& Id)
			{
				return ItemReader.ReadU64(Id);
			});
	}

	bool WriteWorldCountry(CareerSaveWriter& Writer, const CareerWorldCountry& Country)
	{
		return Writer.WriteU64(Country.Id) && Writer.WriteU64(Country.CapitalRegionId) &&
			Writer.WriteString(Country.Name) && Writer.WriteU64(Country.Population) &&
			WriteVector(Writer, Country.RegionIds, [](CareerSaveWriter& ItemWriter, std::uint64_t Id)
			{
				return ItemWriter.WriteU64(Id);
			});
	}

	bool ReadWorldCountry(CareerSaveReader& Reader, CareerWorldCountry& Country)
	{
		return Reader.ReadU64(Country.Id) && Reader.ReadU64(Country.CapitalRegionId) &&
			Reader.ReadString(Country.Name) && Reader.ReadU64(Country.Population) &&
			ReadVector(Reader, Country.RegionIds, [](CareerSaveReader& ItemReader, std::uint64_t& Id)
			{
				return ItemReader.ReadU64(Id);
			});
	}

	bool WriteRacerReference(CareerSaveWriter& Writer, const CareerData& Data,
		const CareerProfile& Profile, const Racer* Runner)
	{
		constexpr std::uint8_t NullReference = 0;
		constexpr std::uint8_t DatabaseReference = 1;
		constexpr std::uint8_t PlayerProfileReference = 2;
		constexpr std::uint8_t CareerProfileReference = 3;

		if (Runner == nullptr)
			return Writer.WriteU8(NullReference);
		if (Runner == &Profile.MainFella)
			return Writer.WriteU8(PlayerProfileReference) && Writer.WriteU32(Runner->GUID.GUID);
		if (Runner == &Data.Profile.MainFella)
			return Writer.WriteU8(CareerProfileReference) && Writer.WriteU32(Runner->GUID.GUID);
		if (Data.Racers.Get(Runner->GUID.GUID) == Runner)
			return Writer.WriteU8(DatabaseReference) && Writer.WriteU32(Runner->GUID.GUID);
		return Writer.Fail("Career data contains a racer reference that is not owned by its database or profile.");
	}

	bool ReadRacerReference(CareerSaveReader& Reader, CareerData& Data,
		CareerProfile& Profile, Racer*& Runner)
	{
		constexpr std::uint8_t NullReference = 0;
		constexpr std::uint8_t DatabaseReference = 1;
		constexpr std::uint8_t PlayerProfileReference = 2;
		constexpr std::uint8_t CareerProfileReference = 3;

		std::uint8_t Kind = 0;
		if (!Reader.ReadU8(Kind))
			return false;
		if (Kind == NullReference)
		{
			Runner = nullptr;
			return true;
		}

		std::uint32_t Guid = 0;
		if (!Reader.ReadU32(Guid))
			return false;
		if (Kind == DatabaseReference)
		{
			Runner = Data.Racers.Get(Guid);
			if (Runner == nullptr)
				return Reader.Fail("Career save references a racer missing from its database.");
			return true;
		}
		if (Kind == PlayerProfileReference && Profile.MainFella.GUID.GUID == Guid)
		{
			Runner = &Profile.MainFella;
			return true;
		}
		if (Kind == CareerProfileReference && Data.Profile.MainFella.GUID.GUID == Guid)
		{
			Runner = &Data.Profile.MainFella;
			return true;
		}
		return Reader.Fail("Career save contains an invalid racer reference.");
	}

	bool WriteRacerReferences(CareerSaveWriter& Writer, const CareerData& Data,
		const CareerProfile& Profile, const std::vector<Racer*>& Runners)
	{
		if (!Writer.WriteCount(Runners.size()))
			return false;
		for (const Racer* Runner : Runners)
			if (!WriteRacerReference(Writer, Data, Profile, Runner))
				return false;
		return true;
	}

	bool ReadRacerReferences(CareerSaveReader& Reader, CareerData& Data,
		CareerProfile& Profile, std::vector<Racer*>& Runners)
	{
		return ReadVector(Reader, Runners, [&Data, &Profile](CareerSaveReader& ItemReader, Racer*& Runner)
			{
				return ReadRacerReference(ItemReader, Data, Profile, Runner);
			});
	}

	bool WriteLeagueFixture(CareerSaveWriter& Writer, const CareerLeagueFixture& Fixture)
	{
		return Writer.WriteU32(Fixture.Fixture.Round) &&
			Writer.WriteU64(Fixture.Fixture.FirstCompetitorId) &&
			Writer.WriteU64(Fixture.Fixture.SecondCompetitorId) &&
			WriteEnum(Writer, Fixture.State) && Writer.WriteU64(Fixture.ScheduledAtMs) &&
			Writer.WriteU64(Fixture.RaceId) && Writer.WriteU64(Fixture.HistoricalRaceId) &&
			Writer.WriteU64(Fixture.WinnerId) &&
			Writer.WriteSize(Fixture.TrackPlotIndex);
	}

	bool ReadLeagueFixture(CareerSaveReader& Reader, CareerLeagueFixture& Fixture)
	{
		return Reader.ReadU32(Fixture.Fixture.Round) &&
			Reader.ReadU64(Fixture.Fixture.FirstCompetitorId) &&
			Reader.ReadU64(Fixture.Fixture.SecondCompetitorId) &&
			ReadEnum(Reader, Fixture.State, static_cast<std::uint32_t>(CareerFixtureState::Cancelled)) &&
			Reader.ReadU64(Fixture.ScheduledAtMs) && Reader.ReadU64(Fixture.RaceId) &&
			Reader.ReadU64(Fixture.HistoricalRaceId) && Reader.ReadU64(Fixture.WinnerId) &&
			Reader.ReadSize(Fixture.TrackPlotIndex);
	}

	bool WritePlayoffMatch(CareerSaveWriter& Writer, const SportRaceCore::PlayoffMatch& Match)
	{
		if (!Writer.WriteBool(Match.CompetitorAId.has_value()))
			return false;
		if (Match.CompetitorAId.has_value() && !Writer.WriteU64(Match.CompetitorAId.value()))
			return false;
		if (!Writer.WriteBool(Match.CompetitorBId.has_value()))
			return false;
		if (Match.CompetitorBId.has_value() && !Writer.WriteU64(Match.CompetitorBId.value()))
			return false;
		return Writer.WriteU64(Match.WinnerId) && Writer.WriteBool(Match.IsComplete) &&
			Writer.WriteU64(Match.HistoricalRaceId);
	}

	bool ReadPlayoffMatch(CareerSaveReader& Reader, SportRaceCore::PlayoffMatch& Match)
	{
		bool HasCompetitor = false;
		std::uint64_t CompetitorId = 0;
		if (!Reader.ReadBool(HasCompetitor))
			return false;
		if (HasCompetitor)
		{
			if (!Reader.ReadU64(CompetitorId))
				return false;
			Match.CompetitorAId = CompetitorId;
		}
		if (!Reader.ReadBool(HasCompetitor))
			return false;
		if (HasCompetitor)
		{
			if (!Reader.ReadU64(CompetitorId))
				return false;
			Match.CompetitorBId = CompetitorId;
		}
		return Reader.ReadU64(Match.WinnerId) && Reader.ReadBool(Match.IsComplete) &&
			Reader.ReadU64(Match.HistoricalRaceId);
	}

	bool WritePlayoffRound(CareerSaveWriter& Writer, const SportRaceCore::PlayoffRound& Round)
	{
		return WriteVector(Writer, Round.Matches, WritePlayoffMatch);
	}

	bool ReadPlayoffRound(CareerSaveReader& Reader, SportRaceCore::PlayoffRound& Round)
	{
		return ReadVector(Reader, Round.Matches, ReadPlayoffMatch);
	}

	bool WritePlayoffBracket(CareerSaveWriter& Writer, const SportRaceCore::PlayoffBracket& Bracket)
	{
		return Writer.WriteSize(Bracket.EntrantCount) &&
			WriteVector(Writer, Bracket.Rounds, WritePlayoffRound) &&
			Writer.WriteU64(Bracket.WinnerId) && Writer.WriteBool(Bracket.IsComplete);
	}

	bool ReadPlayoffBracket(CareerSaveReader& Reader, SportRaceCore::PlayoffBracket& Bracket)
	{
		return Reader.ReadSize(Bracket.EntrantCount) &&
			ReadVector(Reader, Bracket.Rounds, ReadPlayoffRound) &&
			Reader.ReadU64(Bracket.WinnerId) && Reader.ReadBool(Bracket.IsComplete);
	}

	bool WriteLeagueStanding(CareerSaveWriter& Writer, const CareerLeagueStanding& Standing,
		const CareerData& Data, const CareerProfile& Profile)
	{
		return WriteRacerReference(Writer, Data, Profile, Standing.Runner) &&
			Writer.WriteU32(Standing.Wins) && Writer.WriteU32(Standing.Draws) &&
			Writer.WriteU32(Standing.Losses) && Writer.WriteU32(Standing.Points);
	}

	bool ReadLeagueStanding(CareerSaveReader& Reader, CareerLeagueStanding& Standing,
		CareerData& Data, CareerProfile& Profile)
	{
		return ReadRacerReference(Reader, Data, Profile, Standing.Runner) &&
			Reader.ReadU32(Standing.Wins) && Reader.ReadU32(Standing.Draws) &&
			Reader.ReadU32(Standing.Losses) && Reader.ReadU32(Standing.Points);
	}

	bool WriteLeague(CareerSaveWriter& Writer, const CareerLeague& League,
		const CareerData& Data, const CareerProfile& Profile)
	{
		if (!Writer.WriteU64(League.Id) || !Writer.WriteU64(League.CountryId) ||
			!Writer.WriteU64(League.CityId) || !Writer.WriteU64(League.RegionId) ||
			!Writer.WriteString(League.Name) || !WriteEnum(Writer, League.Level) ||
			!WriteEnum(Writer, League.Division) || !Writer.WriteU64(League.PromotionTargetLeagueId) ||
			!Writer.WriteSize(League.PromotionSlots) || !Writer.WriteSize(League.IncomingPromotionLimit) ||
			!Writer.WriteBool(League.IsCup) || !Writer.WriteBool(League.IsAmateurCup) ||
			!Writer.WriteBool(League.IsOpenLobby) || !WriteEnum(Writer, League.State) ||
			!Writer.WriteU32(League.Season) ||
			!WriteRacerReferences(Writer, Data, Profile, League.Entrants) ||
			!WriteVector(Writer, League.Fixtures, WriteLeagueFixture))
			return false;

		if (!Writer.WriteCount(League.Standings.size()))
			return false;
		for (const CareerLeagueStanding& Standing : League.Standings)
			if (!WriteLeagueStanding(Writer, Standing, Data, Profile))
				return false;
		return WritePlayoffBracket(Writer, League.Playoffs);
	}

	bool ReadLeague(CareerSaveReader& Reader, CareerLeague& League,
		CareerData& Data, CareerProfile& Profile)
	{
		if (!Reader.ReadU64(League.Id) || !Reader.ReadU64(League.CountryId) ||
			!Reader.ReadU64(League.CityId) || !Reader.ReadU64(League.RegionId) ||
			!Reader.ReadString(League.Name) ||
			!ReadEnum(Reader, League.Level, static_cast<std::uint32_t>(CareerCompetitionLevel::National)) ||
			!ReadEnum(Reader, League.Division, static_cast<std::uint32_t>(CareerLeagueDivision::Division1)) ||
			!Reader.ReadU64(League.PromotionTargetLeagueId) || !Reader.ReadSize(League.PromotionSlots) ||
			!Reader.ReadSize(League.IncomingPromotionLimit) || !Reader.ReadBool(League.IsCup) ||
			!Reader.ReadBool(League.IsAmateurCup) || !Reader.ReadBool(League.IsOpenLobby) ||
			!ReadEnum(Reader, League.State, static_cast<std::uint32_t>(CareerLeagueState::Complete)) ||
			!Reader.ReadU32(League.Season) || !ReadRacerReferences(Reader, Data, Profile, League.Entrants) ||
			!ReadVector(Reader, League.Fixtures, ReadLeagueFixture))
			return false;

		std::uint32_t StandingCount = 0;
		if (!Reader.ReadCount(StandingCount))
			return false;
		League.Standings.clear();
		League.Standings.reserve(StandingCount);
		for (std::uint32_t Index = 0; Index < StandingCount; ++Index)
		{
			CareerLeagueStanding Standing;
			if (!ReadLeagueStanding(Reader, Standing, Data, Profile))
				return false;
			League.Standings.push_back(Standing);
		}
		return ReadPlayoffBracket(Reader, League.Playoffs);
	}

	bool WriteRacerState(CareerSaveWriter& Writer, const CareerRacerState& State,
		const CareerData& Data, const CareerProfile& Profile)
	{
		const std::uint64_t BasedInCityId = State.BasedInCityId;
		return WriteRacerReference(Writer, Data, Profile, State.Runner) &&
			Writer.WriteU64(BasedInCityId) && Writer.WriteI32(State.Cash) &&
			Writer.WriteI32(State.StartingCash) && Writer.WriteU64(State.LeagueId) &&
			Writer.WriteSize(State.LocationPlotIndex) && Writer.WriteSize(State.DestinationPlotIndex) &&
			Writer.WriteU64(State.ActionEndsAtRealMs) &&
			Writer.WriteU64(State.RaceCooldownEndsAtRealMs) &&
			Writer.WriteU64(State.ActiveRaceId) && Writer.WriteU64(State.RetiredAtMs) &&
			WriteEnum(Writer, State.Activity) && Writer.WriteBool(State.HasRacingLicense) &&
			Writer.WriteBool(State.LicenseApplicationInProgress);
	}

	bool ReadRacerState(CareerSaveReader& Reader, CareerRacerState& State,
		CareerData& Data, CareerProfile& Profile)
	{
		std::uint64_t BasedInCityId = 0;
		std::int32_t Cash = 0;
		std::int32_t StartingCash = 0;
		if (!ReadRacerReference(Reader, Data, Profile, State.Runner) ||
			!Reader.ReadU64(BasedInCityId) || !Reader.ReadI32(Cash) || !Reader.ReadI32(StartingCash) ||
			!Reader.ReadU64(State.LeagueId) || !Reader.ReadSize(State.LocationPlotIndex) ||
			!Reader.ReadSize(State.DestinationPlotIndex) || !Reader.ReadU64(State.ActionEndsAtRealMs) ||
			!Reader.ReadU64(State.RaceCooldownEndsAtRealMs) || !Reader.ReadU64(State.ActiveRaceId) ||
			!Reader.ReadU64(State.RetiredAtMs) ||
			!ReadEnum(Reader, State.Activity, static_cast<std::uint32_t>(CareerRacerActivity::Retired)) ||
			!Reader.ReadBool(State.HasRacingLicense) || !Reader.ReadBool(State.LicenseApplicationInProgress))
			return false;
		State.BasedInCityId = BasedInCityId;
		State.Cash = Cash;
		State.StartingCash = StartingCash;
		return true;
	}

	bool WritePendingBet(CareerSaveWriter& Writer, const Game::Career::CareerPendingBet& Bet,
		const CareerData& Data, const CareerProfile& Profile)
	{
		return WriteRacerReference(Writer, Data, Profile, Bet.SelectedRacer) &&
			Writer.WriteU32(Bet.Stake) && Writer.WriteDouble(Bet.DecimalOdds) &&
			Writer.WriteBool(Bet.Active);
	}

	bool ReadPendingBet(CareerSaveReader& Reader, Game::Career::CareerPendingBet& Bet,
		CareerData& Data, CareerProfile& Profile)
	{
		return ReadRacerReference(Reader, Data, Profile, Bet.SelectedRacer) &&
			Reader.ReadU32(Bet.Stake) && Reader.ReadDouble(Bet.DecimalOdds) &&
			Reader.ReadBool(Bet.Active);
	}

	bool WriteCareerData(CareerSaveWriter& Writer, const CareerData& Data,
		const CareerProfile& PlayerProfile)
	{
		const std::vector<Racer*> DatabaseRacers = Data.Racers.GetAll();
		if (!Writer.WriteCount(DatabaseRacers.size()))
			return false;
		for (const Racer* Runner : DatabaseRacers)
			if (Runner == nullptr || !WriteRacer(Writer, *Runner))
				return Runner != nullptr ? false : Writer.Fail("Career database contains a null racer.");

		if (!WriteCareerProfile(Writer, Data.Profile) ||
			!WriteVector(Writer, Data.CityPlots, WriteCityPlot) ||
			!WriteVector(Writer, Data.Cities, WriteWorldCity) ||
			!WriteVector(Writer, Data.Regions, WriteWorldRegion) ||
			!WriteVector(Writer, Data.Countries, WriteWorldCountry) ||
			!Writer.WriteU64(Data.HomeCityId) || !Writer.WriteU64(Data.PlayerStableCityId) ||
			!Writer.WriteSize(Data.PlayerStablePlotIndex) ||
			!WriteVector(Writer, Data.RaceTrackLengths, [](CareerSaveWriter& ItemWriter, unsigned int Length)
			{
				return ItemWriter.WriteU32(Length);
			}) ||
			!WriteRacerReferences(Writer, Data, PlayerProfile, Data.OwnedRacers) ||
			!WriteRacerReference(Writer, Data, PlayerProfile, Data.PlayerLeagueRacer) ||
			!Writer.WriteU64(Data.PlayerLeagueId) || !Writer.WriteU64(Data.NextCareerRaceHistoryId) ||
			!WriteVector(Writer, Data.HistoricalRaces, WriteCareerHistoricalRace))
			return false;

		if (!Writer.WriteCount(Data.RacerStates.size()))
			return false;
		for (const CareerRacerState& State : Data.RacerStates)
			if (!WriteRacerState(Writer, State, Data, PlayerProfile))
				return false;

		if (!Writer.WriteCount(Data.Leagues.size()))
			return false;
		for (const CareerLeague& League : Data.Leagues)
			if (!WriteLeague(Writer, League, Data, PlayerProfile))
				return false;

		if (!WriteRacerReferences(Writer, Data, PlayerProfile, Data.UpcomingRaceEntrants) ||
			!Writer.WriteU32(Data.UpcomingRaceSeed) || !Writer.WriteU32(Data.UpcomingRaceTrackLength) ||
			!Writer.WriteU64(Data.NextWorldRaceId) || !Writer.WriteU64(Data.NextLeagueId) ||
			!Writer.WriteU64(Data.NextOpenRaceAtMs) || !Writer.WriteSize(Data.NextOpenRaceTrackIndex) ||
			!Writer.WriteU64(Data.NextAmateurCupAtMs) || !Writer.WriteSize(Data.TargetAiPopulation) ||
			!Writer.WriteU64(Data.RealTimeMs) || !Writer.WriteU64(Data.WorldClock.NowMs()) ||
			!Writer.WriteU32(Data.WorldClock.GetTimeScale()))
			return false;

		std::ostringstream RandomState;
		RandomState.imbue(std::locale::classic());
		RandomState << Data.WorldRandom;
		if (!RandomState)
			return Writer.Fail("Could not serialize the career world's random-generator state.");
		if (!Writer.WriteString(RandomState.str()) ||
			!Writer.WriteBool(Data.WorldSimulationInitialized) ||
			!Writer.WriteBool(Data.InitialWorldMonthSimulated) ||
			!Writer.WriteBool(Data.InitialWorldMonthSimulationInProgress) ||
			!Writer.WriteBool(Data.InitialWorldMonthSimulationFailed) ||
			!Writer.WriteBool(Data.InitialWorldMonthClockResetPending) ||
			!Writer.WriteU64(Data.InitialWorldMonthTotalRealMs) ||
			!Writer.WriteU64(Data.InitialWorldMonthElapsedRealMs) ||
			!Writer.WriteU64(Data.InitialWorldMonthStartingWorldMs) ||
			!Writer.WriteU64(Data.InitialWorldMonthStartingRealMs) ||
			!Writer.WriteU64(Data.InitialWorldMonthProcessingMs) ||
			!Writer.WriteU32(Data.BookmakerAccuracy))
			return false;

		for (bool HasLicense : Data.Licenses)
			if (!Writer.WriteBool(HasLicense))
				return false;
		return WritePendingBet(Writer, Data.PendingBet, Data, PlayerProfile);
	}

	bool ReadCareerData(CareerSaveReader& Reader, CareerData& Data, CareerProfile& PlayerProfile,
		std::uint32_t Version)
	{
		std::uint32_t RacerCount = 0;
		if (!Reader.ReadCount(RacerCount))
			return false;
		std::unordered_set<std::uint32_t> RacerIds;
		RacerIds.reserve(RacerCount);
		for (std::uint32_t Index = 0; Index < RacerCount; ++Index)
		{
			Racer Runner;
			if (!ReadRacer(Reader, Runner))
				return false;
			const std::uint32_t Guid = Runner.GUID.GUID;
			if (Guid == 0 || !RacerIds.insert(Guid).second)
				return Reader.Fail("Career save contains a zero or duplicate racer identifier.");
			auto StoredRunner = std::make_unique<Racer>(std::move(Runner));
			Data.Racers.Add(StoredRunner.get());
			StoredRunner.release();
		}

		if (!ReadCareerProfile(Reader, Data.Profile) ||
			!ReadVector(Reader, Data.CityPlots, ReadCityPlot) ||
			!ReadVector(Reader, Data.Cities, ReadWorldCity) ||
			!ReadVector(Reader, Data.Regions, ReadWorldRegion) ||
			!ReadVector(Reader, Data.Countries, ReadWorldCountry) ||
			!Reader.ReadU64(Data.HomeCityId))
			return false;
		if (Version >= 4 && (!Reader.ReadU64(Data.PlayerStableCityId) ||
			!Reader.ReadSize(Data.PlayerStablePlotIndex)))
			return false;
		if (!ReadVector(Reader, Data.RaceTrackLengths, [](CareerSaveReader& ItemReader, unsigned int& Length)
			{
				return ItemReader.ReadU32(Length);
			}) ||
			!ReadRacerReferences(Reader, Data, PlayerProfile, Data.OwnedRacers) ||
			!ReadRacerReference(Reader, Data, PlayerProfile, Data.PlayerLeagueRacer) ||
			!Reader.ReadU64(Data.PlayerLeagueId) ||
			!Reader.ReadU64(Data.NextCareerRaceHistoryId) ||
			!ReadVector(Reader, Data.HistoricalRaces, ReadCareerHistoricalRace) ||
			!ReadVector(Reader, Data.RacerStates, [&Data, &PlayerProfile](CareerSaveReader& ItemReader,
				CareerRacerState& State)
			{
				return ReadRacerState(ItemReader, State, Data, PlayerProfile);
			}) ||
			!ReadVector(Reader, Data.Leagues, [&Data, &PlayerProfile](CareerSaveReader& ItemReader,
				CareerLeague& League)
			{
				return ReadLeague(ItemReader, League, Data, PlayerProfile);
			}) ||
			!ReadRacerReferences(Reader, Data, PlayerProfile, Data.UpcomingRaceEntrants) ||
			!Reader.ReadU32(Data.UpcomingRaceSeed) || !Reader.ReadU32(Data.UpcomingRaceTrackLength) ||
			!Reader.ReadU64(Data.NextWorldRaceId) || !Reader.ReadU64(Data.NextLeagueId) ||
			!Reader.ReadU64(Data.NextOpenRaceAtMs) || !Reader.ReadSize(Data.NextOpenRaceTrackIndex) ||
			!Reader.ReadU64(Data.NextAmateurCupAtMs) || !Reader.ReadSize(Data.TargetAiPopulation) ||
			!Reader.ReadU64(Data.RealTimeMs))
			return false;

		std::uint64_t WorldTimeMs = 0;
		std::uint32_t WorldTimeScale = 0;
		if (!Reader.ReadU64(WorldTimeMs) || !Reader.ReadU32(WorldTimeScale))
			return false;
		if (!Data.WorldClock.Restore(WorldTimeMs, WorldTimeScale))
			return Reader.Fail("Career save contains an invalid world-clock scale.");

		std::string RandomStateText;
		if (!Reader.ReadString(RandomStateText))
			return false;
		std::istringstream RandomState(RandomStateText);
		RandomState.imbue(std::locale::classic());
		RandomState >> Data.WorldRandom;
		if (!RandomState)
			return Reader.Fail("Career save contains an invalid random-generator state.");
		RandomState >> std::ws;
		if (RandomState.peek() != std::char_traits<char>::eof())
			return Reader.Fail("Career save contains trailing random-generator data.");

		if (!Reader.ReadBool(Data.WorldSimulationInitialized) ||
			!Reader.ReadBool(Data.InitialWorldMonthSimulated) ||
			!Reader.ReadBool(Data.InitialWorldMonthSimulationInProgress) ||
			!Reader.ReadBool(Data.InitialWorldMonthSimulationFailed) ||
			!Reader.ReadBool(Data.InitialWorldMonthClockResetPending) ||
			!Reader.ReadU64(Data.InitialWorldMonthTotalRealMs) ||
			!Reader.ReadU64(Data.InitialWorldMonthElapsedRealMs) ||
			!Reader.ReadU64(Data.InitialWorldMonthStartingWorldMs) ||
			!Reader.ReadU64(Data.InitialWorldMonthStartingRealMs) ||
			!Reader.ReadU64(Data.InitialWorldMonthProcessingMs) ||
			!Reader.ReadU32(Data.BookmakerAccuracy))
			return false;

		for (bool& HasLicense : Data.Licenses)
			if (!Reader.ReadBool(HasLicense))
				return false;
		return ReadPendingBet(Reader, Data.PendingBet, Data, PlayerProfile);
	}

	bool ValidateLoadedCareer(CareerSaveReader& Reader, const CareerData& Data)
	{
		if (!Data.WorldSimulationInitialized || !Data.InitialWorldMonthSimulated ||
			Data.InitialWorldMonthSimulationInProgress || Data.InitialWorldMonthSimulationFailed)
			return Reader.Fail("Career save does not contain a completed, initialized career world.");
		if (Data.NextCareerRaceHistoryId == 0 || Data.HistoricalRaces.size() > Game::Career::MaximumCareerHistoricalRaces)
			return Reader.Fail("Career save contains invalid archived race history metadata.");
		if (!Data.OwnedRacers.empty() && Data.PlayerStableCityId == 0)
			return Reader.Fail("Career save contains owned racers without a leased stable.");
		std::unordered_set<std::uint64_t> HistoryIds;
		HistoryIds.reserve(Data.HistoricalRaces.size());
		for (const CareerHistoricalRace& HistoricalRace : Data.HistoricalRaces)
			if (HistoricalRace.Id == 0 || HistoricalRace.Id >= Data.NextCareerRaceHistoryId ||
				!HistoryIds.insert(HistoricalRace.Id).second ||
				!Game::Race::IsValidRaceHistorySnapshot(HistoricalRace.Snapshot))
				return Reader.Fail("Career save contains a zero, duplicate, or invalid archived race entry.");
		if (Data.PlayerStableCityId != 0)
		{
			if (Data.PlayerStableCityId != Data.HomeCityId)
				return Reader.Fail("Career save contains a stable lease outside the supported home city.");
			const std::vector<CityPlot>* StablePlots = nullptr;
			if (Data.PlayerStableCityId == Data.HomeCityId)
				StablePlots = &Data.CityPlots;
			else
			{
				const auto City = std::find_if(Data.Cities.begin(), Data.Cities.end(), [&Data](
					const CareerWorldCity& Candidate) { return Candidate.Id == Data.PlayerStableCityId; });
				if (City != Data.Cities.end())
					StablePlots = &City->Plots;
			}
			if (StablePlots == nullptr || Data.PlayerStablePlotIndex >= StablePlots->size() ||
				(*StablePlots)[Data.PlayerStablePlotIndex].Type != CityPlotType::Stable)
				return Reader.Fail("Career save contains an invalid player stable lease.");
		}
		return true;
	}

	void NormalizeInterruptedRaces(CareerData& Data)
	{
		for (CareerRacerState& State : Data.RacerStates)
		{
			if (State.Activity == CareerRacerActivity::InRace)
			{
				State.Activity = CareerRacerActivity::Resting;
				State.ActionEndsAtRealMs = Data.RealTimeMs;
				State.RaceCooldownEndsAtRealMs = Data.RealTimeMs;
			}
			State.ActiveRaceId = 0;
		}

		const std::uint64_t WorldTimeMs = Data.WorldClock.NowMs();
		for (CareerLeague& League : Data.Leagues)
			for (CareerLeagueFixture& Fixture : League.Fixtures)
				if (Fixture.State == CareerFixtureState::InProgress)
				{
					Fixture.State = CareerFixtureState::Scheduled;
					Fixture.ScheduledAtMs = WorldTimeMs;
					Fixture.RaceId = 0;
					Fixture.WinnerId = 0;
				}

		Data.PendingPlayerLeagueRace = {};
		Data.PendingOpenRaces.clear();
		Data.PendingWorldLeagueRaces.clear();
		Data.PendingWorldPlayoffRaces.clear();
		Data.WorldRaces.clear();
	}

	void RemoveTemporaryFile(const std::filesystem::path& Path)
	{
		std::error_code Error;
		std::filesystem::remove(Path, Error);
	}
}

bool Game::Career::SaveCareer(const CareerData& Data, const CareerProfile& Profile,
	const std::string& FilePath, std::string& Error)
{
	using namespace CareerSaveDetail;
	Error.clear();
	if (FilePath.empty())
	{
		Error = "Career save path is empty.";
		return false;
	}
	if (!Data.WorldSimulationInitialized || !Data.InitialWorldMonthSimulated ||
		Data.InitialWorldMonthSimulationInProgress || Data.InitialWorldMonthSimulationFailed)
	{
		Error = "Career saving is available after the initial career simulation has completed.";
		return false;
	}

	CareerSaveWriter Payload;
	if (!WriteCareerProfile(Payload, Profile) || !WriteCareerData(Payload, Data, Profile))
	{
		Error = Payload.Error.empty() ? "Could not serialize career data." : Payload.Error;
		return false;
	}

	const std::filesystem::path SavePath(FilePath);
	std::filesystem::path TemporaryPath = SavePath;
	TemporaryPath += ".tmp";
	std::error_code FileError;
	if (!SavePath.parent_path().empty())
	{
		std::filesystem::create_directories(SavePath.parent_path(), FileError);
		if (FileError)
		{
			Error = "Could not create the career save directory: " + FileError.message();
			return false;
		}
	}

	CareerSaveWriter Header;
	if (!Header.WriteU32(CurrentCareerSaveFormatVersion) ||
		!Header.WriteU64(static_cast<std::uint64_t>(Payload.Bytes.size())) ||
		!Header.WriteU64(CalculateChecksum(Payload.Bytes)))
	{
		Error = Header.Error.empty() ? "Could not create the career save header." : Header.Error;
		return false;
	}

	std::ofstream Output(TemporaryPath, std::ios::binary | std::ios::trunc);
	if (!Output)
	{
		Error = "Could not open the temporary career save file for writing.";
		return false;
	}
	Output.write(CareerSaveMagic.data(), static_cast<std::streamsize>(CareerSaveMagic.size()));
	Output.write(Header.Bytes.data(), static_cast<std::streamsize>(Header.Bytes.size()));
	Output.write(Payload.Bytes.data(), static_cast<std::streamsize>(Payload.Bytes.size()));
	Output.flush();
	if (!Output)
	{
		Output.close();
		RemoveTemporaryFile(TemporaryPath);
		Error = "Could not write the career save file.";
		return false;
	}
	Output.close();
	if (!Output)
	{
		RemoveTemporaryFile(TemporaryPath);
		Error = "Could not finish writing the career save file.";
		return false;
	}

#if defined(_WIN32)
	if (!MoveFileExW(TemporaryPath.c_str(), SavePath.c_str(),
		MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
	{
		const std::error_code ReplaceError(static_cast<int>(GetLastError()), std::system_category());
		RemoveTemporaryFile(TemporaryPath);
		Error = "Could not replace the career save file: " + ReplaceError.message();
		return false;
	}
#else
	std::filesystem::rename(TemporaryPath, SavePath, FileError);
	if (FileError)
	{
		RemoveTemporaryFile(TemporaryPath);
		Error = "Could not replace the career save file: " + FileError.message();
		return false;
	}
#endif
	return true;
}

bool Game::Career::LoadCareer(const std::string& FilePath,
	std::unique_ptr<CareerData>& LoadedData, std::unique_ptr<CareerProfile>& LoadedProfile,
	std::string& Error)
{
	using namespace CareerSaveDetail;
	Error.clear();
	if (FilePath.empty())
	{
		Error = "Career save path is empty.";
		return false;
	}

	std::ifstream Input(std::filesystem::path(FilePath), std::ios::binary);
	if (!Input)
	{
		Error = "Could not open career save file '" + FilePath + "'.";
		return false;
	}

	std::array<char, CareerSaveMagic.size()> Magic{};
	Input.read(Magic.data(), static_cast<std::streamsize>(Magic.size()));
	if (Input.gcount() != static_cast<std::streamsize>(Magic.size()) || Magic != CareerSaveMagic)
	{
		Error = "Career save has an invalid or truncated file signature.";
		return false;
	}

	std::array<char, 20> EncodedHeader{};
	Input.read(EncodedHeader.data(), static_cast<std::streamsize>(EncodedHeader.size()));
	if (Input.gcount() != static_cast<std::streamsize>(EncodedHeader.size()))
	{
		Error = "Career save header is truncated.";
		return false;
	}
	const std::uint32_t Version = static_cast<std::uint32_t>(DecodeUnsigned(EncodedHeader.data(), 4));
	if (Version != 3 && Version != CurrentCareerSaveFormatVersion)
	{
		Error = "Unsupported career save version " + std::to_string(Version) +
			"; this game supports versions 3 through " +
			std::to_string(CurrentCareerSaveFormatVersion) + ".";
		return false;
	}
	const std::uint64_t PayloadSize = DecodeUnsigned(EncodedHeader.data() + 4, 8);
	const std::uint64_t ExpectedChecksum = DecodeUnsigned(EncodedHeader.data() + 12, 8);
	if (PayloadSize > MaximumCareerSavePayloadBytes || PayloadSize > std::numeric_limits<std::size_t>::max())
	{
		Error = "Career save payload exceeds the supported size limit.";
		return false;
	}

	std::string Payload(static_cast<std::size_t>(PayloadSize), '\0');
	if (!Payload.empty())
	{
		Input.read(Payload.data(), static_cast<std::streamsize>(Payload.size()));
		if (Input.gcount() != static_cast<std::streamsize>(Payload.size()))
		{
			Error = "Career save payload is truncated.";
			return false;
		}
	}
	char TrailingByte = 0;
	if (Input.get(TrailingByte))
	{
		Error = "Career save contains unexpected trailing data.";
		return false;
	}
	if (!Input.eof())
	{
		Error = "Could not finish reading the career save file.";
		return false;
	}
	if (CalculateChecksum(Payload) != ExpectedChecksum)
	{
		Error = "Career save checksum does not match; the file may be incomplete or damaged.";
		return false;
	}

	CareerSaveReader Reader(Payload);
	auto NewProfile = std::make_unique<CareerProfile>();
	auto NewData = std::make_unique<CareerData>();
	if (!ReadCareerProfile(Reader, *NewProfile) ||
		!ReadCareerData(Reader, *NewData, *NewProfile, Version))
	{
		Error = Reader.Error.empty() ? "Could not decode career save data." : Reader.Error;
		return false;
	}
	if (Version == 3 && !NewData->OwnedRacers.empty() && NewData->HomeCityId != 0)
	{
		const auto StablePlot = std::find_if(NewData->CityPlots.begin(), NewData->CityPlots.end(),
			[](const CityPlot& Plot) { return Plot.Type == CityPlotType::Stable; });
		if (StablePlot != NewData->CityPlots.end())
		{
			NewData->PlayerStableCityId = NewData->HomeCityId;
			NewData->PlayerStablePlotIndex = static_cast<std::size_t>(StablePlot - NewData->CityPlots.begin());
		}
	}
	if (!Reader.AtEnd())
	{
		Error = "Career save contains unrecognized data after the career state.";
		return false;
	}
	if (!ValidateLoadedCareer(Reader, *NewData))
	{
		Error = Reader.Error;
		return false;
	}

	NormalizeInterruptedRaces(*NewData);
	LoadedData = std::move(NewData);
	LoadedProfile = std::move(NewProfile);
	return true;
}

void Game::Career::RebindCareerProfileRacerReferences(CareerData& Data,
	const CareerProfile& OldProfile, CareerProfile& NewProfile)
{
	using namespace CareerSaveDetail;
	const Racer* OldRunner = &OldProfile.MainFella;
	Racer* NewRunner = &NewProfile.MainFella;
	const auto Rebind = [OldRunner, NewRunner](Racer*& Runner)
	{
		if (Runner == OldRunner)
			Runner = NewRunner;
	};
	const auto RebindRacerVector = [&Rebind](std::vector<Racer*>& Runners)
	{
		for (Racer*& Runner : Runners)
			Rebind(Runner);
	};

	RebindRacerVector(Data.OwnedRacers);
	Rebind(Data.PlayerLeagueRacer);
	for (CareerRacerState& State : Data.RacerStates)
		Rebind(State.Runner);
	for (CareerLeague& League : Data.Leagues)
	{
		RebindRacerVector(League.Entrants);
		for (CareerLeagueStanding& Standing : League.Standings)
			Rebind(Standing.Runner);
	}
	RebindRacerVector(Data.PendingPlayerLeagueRace.Entrants);
	for (Game::Career::CareerPendingOpenRace& PendingRace : Data.PendingOpenRaces)
		RebindRacerVector(PendingRace.Entrants);
	for (Game::Career::CareerPendingWorldLeagueRace& PendingRace : Data.PendingWorldLeagueRaces)
		RebindRacerVector(PendingRace.Entrants);
	for (Game::Career::CareerPendingWorldPlayoffRace& PendingRace : Data.PendingWorldPlayoffRaces)
		RebindRacerVector(PendingRace.Entrants);
	for (const std::shared_ptr<Game::Career::CareerWorldRace>& WorldRace : Data.WorldRaces)
		if (WorldRace != nullptr)
			RebindRacerVector(WorldRace->Entrants);
	RebindRacerVector(Data.UpcomingRaceEntrants);
	Rebind(Data.PendingBet.SelectedRacer);
}
