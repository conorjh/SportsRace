#ifndef AUDIO_H
#define AUDIO_H
#include "SDL3_mixer/SDL_mixer.h"

namespace Game
{
	namespace Audio
	{
		enum class BuiltInSounds
		{
			Hover, Click, StartersPistol, CarHorn
		};

		enum class Soundtrack
		{
			Catering, Race
		};

		class AudioPlayer
		{
			Mix_Chunk* Hover = nullptr, * Click = nullptr, * StartersPistol = nullptr, * CarHorn = nullptr;
			Mix_Music* Catering = nullptr, *Race = nullptr;
		public:
			AudioPlayer();

			bool Init();

			void Play(BuiltInSounds), Play(Soundtrack);

			void StopMusic();
		};
	}
}

#endif