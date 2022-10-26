#include "Util.h"
#include "SDL2/SDL.h"
#include <fstream>
#include <iostream>

using namespace std;

bool Game::Util::FileExists(std::string Filepath)
{
	ifstream ifile;
	ifile.open(Filepath);
	if (ifile)
		return true;
	else
		return false;
}

std::string  Game::Util::itos(int i)
{
	return "";
}

std::string  Game::Util::uitos(unsigned int i)
{
	return "";
}