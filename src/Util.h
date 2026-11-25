#ifndef UTIL_H
#define UTIL_H
#include <vector>
#include <string>
#include "SDL3/SDL.h"

namespace Game
{
	namespace App
	{
		class AppIO;
	}

	namespace GUI
	{
		struct RelativeLayout
		{
			float Width;
			float Height;

			RelativeLayout(float _Width, float _Height);
			int X(float Fraction) const;
			int Y(float Fraction) const;
			int MapX(float ReferenceX, float ReferenceWidth = 1024.0f) const;
			int MapY(float ReferenceY, float ReferenceHeight = 768.0f) const;
		};

		class Button
		{
			bool IsButtonHoveredOver = false;
		public:
			App::AppIO& IO;

			std::string Text;
			int x, y, w, h;
			SDL_Color Color{ 0, 0, 0, 255 };
			SDL_Rect Rect;

			Button(App::AppIO& _IO, std::string _Text, int x, int  y, int w, int h);

			void Update();
			void SetBounds(int _x, int _y, int _w, int _h);

			bool IsMouseOver() const;
			bool HasMouseClicked();
		}; 

		enum class IconButtonType
		{
			Racer, Training, Race, Ranking
		};
		
		class IconButton
		{
			bool IsButtonHoveredOver = false;
		public:
			App::AppIO& IO;

			IconButtonType Type;
			int x, y, w, h;
			SDL_Rect Rect;

			IconButton(App::AppIO& _IO, IconButtonType _Type, int x, int  y, int w, int h);

			void Update();
			void SetBounds(int _x, int _y, int _w, int _h);

			bool IsMouseOver(), HasMouseClicked();
		};
	}


	namespace Util
	{
		unsigned int RandomNumU(unsigned int Max);
		bool FileExists(std::string Filepath);
		std::string uitos(unsigned int i);
		std::string itos(int i);		
	}
}

#endif
