#ifndef UTIL_H
#define UTIL_H
#include <vector>
#include <string>
#include "SDL2/SDL.h"

namespace Game
{
	namespace App
	{
		class AppIO;
	}

	namespace GUI
	{
		class Button
		{
			bool IsButtonHoveredOver = false;
		public:
			App::AppIO& IO;

			std::string Text;
			int x, y, w, h;
			SDL_Color Color;
			SDL_Rect Rect;

			Button(App::AppIO& _IO, std::string _Text, int x, int  y, int w, int h);

			void Update();

			bool IsMouseOver(), HasMouseClicked();
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

			bool IsMouseOver(), HasMouseClicked();
		};
	}

	struct ErrorData
	{
		void AddError(std::string ErrorToAdd) { return Errors.push_back(ErrorToAdd); };
		bool HasErrored() const { return Errors.size(); }
		std::vector<std::string> Errors;
	};

	namespace Util
	{
		unsigned int RandomNumU(unsigned int Max);
		bool FileExists(std::string Filepath);
		std::string uitos(unsigned int i);
		std::string itos(int i);
		
	}
}

#endif
