#ifndef OPTIONSSCREEN_H
#define OPTIONSSCREEN_H
#include "..\Screens.h"
#include "..\Render\Renderer.h"
#include "..\Util.h"
#include <string>

namespace Game
{
	namespace Screens
	{
		class OptionsScreen : public AppScreen
		{
			std::string StatusMessage;

			void UpdateLayout();
			void CycleResolution(int Direction);
			void ToggleFullscreen();
		public:
			GUI::Button PreviousResolutionButton, NextResolutionButton, FullscreenButton, BackButton;

			OptionsScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data);

			void Entry(), Exit();
			AppScreen* Update();

			std::string GetResolutionLabel() const;
			std::string GetFullscreenLabel() const;
			const std::string& GetStatusMessage() const;
		};
	}

	namespace Render
	{
		class OptionsScreenRenderer : public Render::BaseRenderer
		{
		public:
			Screens::OptionsScreen* Screen;

			OptionsScreenRenderer(Game::Render::AppRenderContext* Context);
			unsigned int Render();
		};
	}
}
#endif
