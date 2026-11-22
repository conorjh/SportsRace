#ifndef ROULETTESCREEN_H
#define ROULETTESCREEN_H
#include "BuildingScreen.h"
#include <random>
#include <string>

namespace Game
{
	namespace Screens
	{
		class RouletteScreen final : public BuildingScreen
		{
			friend class Game::Render::RouletteScreenRenderer;

			static constexpr unsigned int SpinDuration = 4300;
			std::mt19937 Random;
			std::string ResultMessage = "Pick red, black, or a single number to place your bet.";
			std::string PendingMessage;
			unsigned int SpinStartedAt = 0;
			unsigned int BetStake = 25;
			unsigned int RouletteNumber = 0;
			int SelectedNumber = -1;
			int SelectedColor = 1;
			int RouletteColor = -1;
			float WheelRotation = 0.0f;
			float SpinStartRotation = 0.0f;
			float SpinRotationDelta = 0.0f;
			bool IsSpinning = false;
			bool HasResult = false;
			bool LastSpinWon = false;

			void Spin();
			void UpdateActivity() override;

		public:
			RouletteScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data,
				const Career::CityPlot& _Plot);
		};
	}

	namespace Render
	{
		class RouletteScreenRenderer : public BaseRenderer
		{
		public:
			Screens::RouletteScreen* Screen = nullptr;

			RouletteScreenRenderer(AppRenderContext* Context);
			unsigned int Render();
		};
	}
}

#endif