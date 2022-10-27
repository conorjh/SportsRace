#ifndef RACER_H
#define RACER_H
#include <string>
#include <vector>

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

		class Racer
		{
		public:
			Racer(std::string);

			void Tick(unsigned int Ms);

			unsigned int GUID;
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


		};
	}
}

#endif