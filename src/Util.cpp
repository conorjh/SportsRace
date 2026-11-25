#include "Util.h"
#include "Audio.h"
#include "SDL3/SDL.h"
#include "App\App.h"
#include <cmath>
#include <fstream>
#include <iostream>

using namespace std;
using namespace Game::App;
using namespace Game::Audio;
using namespace Game::GUI;

Game::GUI::RelativeLayout::RelativeLayout(float _Width, float _Height)
	: Width(_Width), Height(_Height)
{
}

int Game::GUI::RelativeLayout::X(float Fraction) const
{
	return static_cast<int>(std::lround(Width * Fraction));
}

int Game::GUI::RelativeLayout::Y(float Fraction) const
{
	return static_cast<int>(std::lround(Height * Fraction));
}

int Game::GUI::RelativeLayout::MapX(float ReferenceX, float ReferenceWidth) const
{
	return static_cast<int>(std::lround(ReferenceX * Width / ReferenceWidth));
}

int Game::GUI::RelativeLayout::MapY(float ReferenceY, float ReferenceHeight) const
{
	return static_cast<int>(std::lround(ReferenceY * Height / ReferenceHeight));
}

bool Game::Util::FileExists(std::string Filepath)
{
	ifstream ifile;
	ifile.open(Filepath);
	if (ifile)
		return true;
	else
		return false;
}

void Game::GUI::Button::SetBounds(int _x, int _y, int _w, int _h)
{
	x = Rect.x = _x;
	y = Rect.y = _y;
	w = Rect.w = _w;
	h = Rect.h = _h;
}

std::string  Game::Util::itos(int i)
{
	return "";
}

void Game::GUI::IconButton::SetBounds(int _x, int _y, int _w, int _h)
{
	x = Rect.x = _x;
	y = Rect.y = _y;
	w = Rect.w = _w;
	h = Rect.h = _h;
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
		// Play the hover cue only on entry, not on every frame the pointer stays over the button.
		if (!IsButtonHoveredOver)
			IO.Player.Play(BuiltInSounds::Hover);
		IsButtonHoveredOver = true;

		if (HasMouseClicked())
			IO.Player.Play(BuiltInSounds::Click);
	}
	else
		IsButtonHoveredOver = false;

	if (IsButtonHoveredOver)
		Color = { 255, 0, 255, 255 };
	else
		Color = { 0, 0, 0, 255 };
}

bool Game::GUI::Button::IsMouseOver() const
{
	SDL_Point MousePositionInt;

	MousePositionInt.x = IO.MousePosition.x;
	MousePositionInt.y = IO.MousePosition.y;

	return SDL_PointInRect(&MousePositionInt, &Rect);
}

bool Game::GUI::Button::HasMouseClicked()
{
	return IsMouseOver() && (IO.MouseButtonsPressed & SDL_BUTTON_LMASK) != 0;
}

Game::GUI::IconButton::IconButton(App::AppIO& _IO, IconButtonType _Type, int _x, int _y, int _w, int _h) 
	: IO(_IO), Type(_Type)
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
	return IsMouseOver() && (IO.MouseButtonsPressed & SDL_BUTTON_LMASK) != 0;
}
