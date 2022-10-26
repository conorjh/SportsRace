#include "MainMenu.h"
#include "Race.h"
#include "SDL2/SDL_mixer.h"

using namespace std;
using namespace Game;
using namespace Game::States;
using namespace Game::App;
using namespace Game::Audio;
using namespace Game::Race;

Game::States::MainMenuState::MainMenuState(AppStateMachine& _Machine, AppIO& _IO, AppData& _Data) : AppState(_Machine, _IO, _Data), RaceButton(_IO, "Race", 100, 100, 210, 80), ExitButton(_IO, "Exit", 100, 200, 210, 80)
{
	Type = AppStateType::MainMenu;

}

Game::States::MainMenuState::~MainMenuState()
{

}

AppState* Game::States::MainMenuState::Update()
{
	RaceButton.Update();
	if (RaceButton.HasMouseClicked())
	{
		auto Round = Data.League.SeasonFixtures.GetThisRound();
		auto OurFixture = Round.GetFixtureThatContains("1");
		auto OurRace = OurFixture.GetRace();
		vector<RaceResult> Results;

		//sim all other races
		//for (int r = 0; r < Data.League.SeasonFixtures.RoundFixtures.size(); ++r)
		{
			for (int fix = 0; fix < Round.Fixtures.size(); ++fix)
				if (!Round.Fixtures[fix].Contains("1"))
					Results.push_back(Round.Fixtures[fix].GetRace().Sim());
			Data.League.NextRound();
		}
		Data.League.Standing.AddResults(Results);

		//actually run our race
		Machine.SwapTop(new InRaceState(Machine, IO, Data, OurRace));
		return Machine.Top();
	}

	//


	ExitButton.Update();
	if (ExitButton.HasMouseClicked())
		Data.Halted = true;

	return this;
}

Game::MainMenu::MenuButton::MenuButton(AppIO& _IO, string _Text, int _x, int _y, int _w, int _h) : IO(_IO)
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

void Game::MainMenu::MenuButton::Update()
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
		Color = { 255, 255, 255 };
}

bool Game::MainMenu::MenuButton::IsMouseOver()
{
	return SDL_PointInRect(&IO.MousePosition, &Rect);
}

bool Game::MainMenu::MenuButton::HasMouseClicked()
{
	return IsMouseOver() && SDL_BUTTON(IO.MouseButtons) == 1;
}