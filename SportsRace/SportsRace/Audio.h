#ifndef AUDIO_H
#define AUDIO_H
#include "SDL2/SDL_mixer.h"

namespace Game
{
	namespace Audio
	{
		enum class BuiltInSounds
		{
			Hover, Click, StartersPistol
		};

		enum class Soundtrack
		{
			Catering, Race
		};

		class AudioPlayer
		{
			Mix_Chunk* Hover, * Click, * StartersPistol;
			Mix_Music* Catering, *Race;
		public:
			AudioPlayer();

			bool Init();

			void Play(BuiltInSounds), Play(Soundtrack);

			void StopMusic();
		};
	}
}

#endif