#include "Audio.h"
#include "spdlog/spdlog.h"

Game::Audio::AudioPlayer::AudioPlayer()
{

}

bool Game::Audio::AudioPlayer::Init()
{
	spdlog::debug("AudioPlayer::Init()");

	//load audio here for now
	this->Hover = Mix_LoadWAV("hover.wav");
	this->Click = Mix_LoadWAV("click.wav");
	

	spdlog::trace("AudioPlayer::Init() success");
	return true;
}

void Game::Audio::AudioPlayer::Play(BuiltInSounds Sound)
{
	Mix_Chunk* Chunk;
	switch (Sound)
	{
		case BuiltInSounds::Click:
			Chunk = Click;
			break;
		case BuiltInSounds::Hover:
			Chunk = Hover;
			break;
		default:
			return;
	}

	Mix_PlayChannel(-1, Chunk, 0);
}
