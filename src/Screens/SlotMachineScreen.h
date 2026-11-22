#ifndef SLOTMACHINESCREEN_H
#define SLOTMACHINESCREEN_H
#include "BuildingScreen.h"
#include <array>
#include <random>
#include <string>

namespace Game
{
	namespace Screens
	{
		class SlotMachineScreen final : public BuildingScreen
		{
			friend class Game::Render::SlotMachineScreenRenderer;

			static constexpr unsigned int SpinDuration = 2350;
			std::array<unsigned int, 3> Symbols{ 0, 3, 5 };
			std::mt19937 Random;
			std::string ResultMessage = "Choose a stake, then spin the reels.";
			std::string PendingMessage;
			unsigned int SpinStartedAt = 0;
			unsigned int BetStake = 25;
			unsigned int PendingPayout = 0;
			bool IsSpinning = false;
			bool LastSpinWon = false;

			void Spin();
			void UpdateActivity() override;

		public:
			SlotMachineScreen(AppScreenStateMachine& _Machine, App::AppIO& _IO, App::AppData& _Data,
				const Career::CityPlot& _Plot);
		};
	}

	namespace Render
	{
		class SlotMachineScreenRenderer : public BaseRenderer
		{
		public:
			Screens::SlotMachineScreen* Screen = nullptr;

			SlotMachineScreenRenderer(AppRenderContext* Context);
			unsigned int Render();
		};
	}
}

#endif