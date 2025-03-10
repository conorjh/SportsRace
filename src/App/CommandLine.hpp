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

		/*
		Takes the standard C/C++ char* array, returns a vector of CommandLineArgument objects
		Arguments have the following syntax
			-argument
			/argument

		Any argument may have unlimited parameters
			-argument param1
			/argument param1 param2

			eg. "app.exe -filepath c:/path/file.csv"
			eg. "app.exe /move c:/src/file.csv c:/dst/file.csv"

		Free floating parameters have no parameters
			argument1 argument2

			eg "app.exe hidelaucher showfps"

		*/
		class CommandLineParser
		{
		public:
			static std::vector<CommandLineArgument> Parse(int argc, char* argv[])
			{
				std::vector<CommandLineArgument> Arguments;
				CommandLineArgument arg;
				bool parsingParams = false;
				int index = 0;
				while (index < argc)
				{
					switch (argv[index][0])
					{
					case '//':
					case '-':	//command			
						if (parsingParams)
						{
							Arguments.push_back(arg);
							arg = CommandLineArgument();
						}

						if (argv[index][0] == '--')
							arg.Type = CommandLineArgumentType::OptionalArgument;
						else
							arg.Type = CommandLineArgumentType::Argument;

						arg.Data = std::string(argv[index]).substr(1, strlen(argv[index]) - 1);
						parsingParams = true;
						break;

						//parameter
					default:
						arg.Parameters.push_back(argv[index]);
						if (!parsingParams)
						{
							//the first item is always the exe location
							arg.Type = index == 0 ? CommandLineArgumentType::ApplicationPath : CommandLineArgumentType::FreeFloatingParameter;
							arg.Data = argv[index];
							Arguments.push_back(arg);
							arg = CommandLineArgument();
						}
						break;
					}
					index++;
				}

				if (parsingParams)
					Arguments.push_back(arg);

				return Arguments;
			}
		};

	}
}
#endif