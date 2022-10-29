#ifndef RACER_H
#define RACER_H
#include <string>
#include <vector>
#include <unordered_map>

namespace Game
{
	namespace Race
	{
		class RacerNameMaker
		{
			std::vector<std::string> FirstNames, SecondNames;
		public:
			RacerNameMaker();

			std::string Make();
		};

		struct Position
		{
			int X;
			double Velocity;
		};

		struct RacerGUID
		{
			bool operator==(const RacerGUID& Other);
			RacerGUID& operator=(const RacerGUID& Other);
			RacerGUID& operator=(const unsigned int& Other);
			RacerGUID& operator=(const int& Other);

			unsigned int GUID;
		};

		class Racer
		{
		public:
			Racer();
			Racer(std::string);

			void Tick(unsigned int Ms);
			void Reset();

			RacerGUID GUID;
			unsigned int CurrentTick;
			std::string Name;
			unsigned int RunFrame, LastRunFrameEnd;
			Position Pos;
		};

		class RacerDB
		{
			std::unordered_map<unsigned int, Racer*> Container;
		public:

			Racer* Make(std::string Name = "");
			Racer* Get(unsigned int GUID);

			bool Contains(unsigned int GUID) { return Get(GUID) != nullptr; }
		};
	}
}

#endif