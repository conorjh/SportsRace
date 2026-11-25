#ifndef RACERSCREEN_H
#define RACERSCREEN_H
#include "..\Render/Renderer.h"
#include "..\Screens.h"
#include "..\Race.h"
#include "..\Career.h"
#include "..\Util.h"
#include <string>

namespace Game
{
	namespace Screens
	{
		enum class RacerScreenStateInitType
		{
			RacerCreation, ViewOnly
		};

		// Displays a racer's player-facing profile; available actions depend on the initialization mode.
		class RacerScreen : public AppScreen
		{
		protected:
			Race::Racer* RacerToDisplay;
			void UpdateLayout();

		public:
			RacerScreenStateInitType InitType;

			GUI::Button RegenButton, ApplyButton, ExitButton, LocationButton;
			RacerScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data, Race::Racer* _RacerToDisplay, RacerScreenStateInitType _Type);

			~RacerScreen();

			AppScreen* Update();
			const Career::CareerRacerState* GetCareerRacerState() const;
			std::uint64_t GetBasedInCityId() const;
			std::string GetBasedInCityName() const;
			std::string GetCareerStatusLabel() const;
			const Race::Racer* GetRacerToDisplay() const
			{
				return RacerToDisplay;
			}
		};

	}

	namespace Render
	{

		class RacerScreenRenderer : public Render::BaseRenderer
		{
			Game::Career::CareerProfile* RacerProfile;

		public:
			Screens::RacerScreen* Screen;

			RacerScreenRenderer(Game::Render::AppRenderContext* Context, Game::Career::CareerProfile* RacerProfile);

			unsigned int Render();

		};
	}
}
#endif