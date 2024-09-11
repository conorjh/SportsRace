#include "Util.h"
#include "Audio.h"
#include "SDL3/SDL.h"
#include "App.h"
#include <fstream>
#include <iostream>

using namespace std;
using namespace Game::App;
using namespace Game::Audio;
using namespace Game::GUI;

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


Game::GUI::Button::Button(AppIO& _IO, string _Text, int _x, int _y, int _w, int _h) : IO(_IO)
{
	Text = _Text;
	x = _x;
	y = _y;
	w = _w;
	h = _h;
	Rect.x = _x;
	Rect.y = _y;
	Rect.w = _w;
	Rect.h = _h;
}

void Game::GUI::Button::Update()
{
	if (IsMouseOver())
	{
		if (!IsButtonHoveredOver)
			IO.Player.Play(BuiltInSounds::Hover);
		IsButtonHoveredOver = true;

		if (HasMouseClicked())
			IO.Player.Play(BuiltInSounds::Click);
	}
	else
		IsButtonHoveredOver = false;

	if (IsButtonHoveredOver)
		Color = { 255, 0, 255 };
	else
		Color = { 0, 0, 0 };
}

bool Game::GUI::Button::IsMouseOver()
{
	SDL_Point MousePositionInt;

	MousePositionInt.x = IO.MousePosition.x;
	MousePositionInt.y = IO.MousePosition.y;

	return SDL_PointInRect(&MousePositionInt, &Rect);
}

bool Game::GUI::Button::HasMouseClicked()
{
	return IsMouseOver() && SDL_BUTTON(IO.MouseButtons) == 1;
}

Game::GUI::IconButton::IconButton(App::AppIO& _IO, IconButtonType _Type, int _x, int _y, int _w, int _h) : IO(_IO), Type(_Type)
{
	x = _x;
	y = _y;
	w = _w;
	h = _h;
	Rect.x = _x;
	Rect.y = _y;
	Rect.w = _w;
	Rect.h = _h;
}

void Game::GUI::IconButton::Update()
{
	if (IsMouseOver())
	{
		if (!IsButtonHoveredOver)
			IO.Player.Play(BuiltInSounds::Hover);
		IsButtonHoveredOver = true;

		if (HasMouseClicked())
			IO.Player.Play(BuiltInSounds::Click);
	}
	else
		IsButtonHoveredOver = false;
}

bool Game::GUI::IconButton::IsMouseOver()
{
	SDL_Point MousePositionInt;

	MousePositionInt.x = IO.MousePosition.x;
	MousePositionInt.y = IO.MousePosition.y;

	return SDL_PointInRect(&MousePositionInt, &Rect);
}

bool Game::GUI::IconButton::HasMouseClicked()
{
	return IsMouseOver() && IO.MouseButtons == 1;
}
