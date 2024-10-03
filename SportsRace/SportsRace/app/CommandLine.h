#ifndef COMMANDLINE_H
#define COMMANDLINE_H
#include <string>
#include <vector>

namespace Game
{
	namespace App
	{
		enum class CommandLineArgumentType
		{
			ApplicationPath, Argument, OptionalArgument, FreeFloatingParameter
		};

		struct CommandLineArgument
		{
			CommandLineArgumentType Type;
			std::string Data;
			std::vector<std::string> Parameters;
		};

		class CommandLine
		{
		public:
			CommandLine(int argc, char* argv[]);

			std::vector<CommandLineArgument> Arguments;
		};

	}
}
#endif