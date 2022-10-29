#ifndef AUDIO_H
#define AUDIO_H
#include "SDL2/SDL_mixer.h"

namespace Game
{
	namespace Audio
	{
		enum class BuiltInSounds
		{
			Hover, Click
		};

		enum class Soundtrack
		{
			Catering
		};

		class AudioPlayer
		{
			Mix_Chunk* Hover, * Click;
			Mix_Music* Catering;
		public:
			AudioPlayer();

			bool Init();

			void Play(BuiltInSounds), Play(Soundtrack);

			void StopMusic();
		};
	}
}

#endif