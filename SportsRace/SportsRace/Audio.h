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

		class AudioPlayer
		{
			Mix_Chunk* Hover, * Click;
		public:
			AudioPlayer();

			bool Init();

			void Play(BuiltInSounds);
		};
	}
}

#endif