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
	this->StartersPistol = Mix_LoadWAV("starter_pistol.mp3");

	this->Catering = Mix_LoadMUS("catering.mp3");
	this->Race = Mix_LoadMUS("race.mp3");

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
	case BuiltInSounds::StartersPistol:
		Chunk = StartersPistol;
		break;
	default:
		return;
	}

	Mix_PlayChannel(-1, Chunk, 0);
}

void Game::Audio::AudioPlayer::Play(Soundtrack Track)
{
	Mix_Music* Chunk;
	switch (Track)
	{
	case Soundtrack::Catering:
		Mix_VolumeMusic(MIX_MAX_VOLUME * 0.40);
		Chunk = Catering;
		break;
	case Soundtrack::Race:
		Mix_VolumeMusic(MIX_MAX_VOLUME * 1);
		Chunk = Race;
		break;
	
	default:
		return;
	}

	Mix_PlayMusic(Chunk, 999);
}

void Game::Audio::AudioPlayer::StopMusic()
{
	Mix_FadeOutMusic(200);
}
