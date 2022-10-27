#ifndef RACER_H
#define RACER_H
#include <string>

namespace Game
{
	namespace Race
	{

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

			unsigned int CurrentTick;
			std::string Name;
			unsigned int RunFrame, LastRunFrameEnd;
			Position Pos;
		};
	}
}

#endif