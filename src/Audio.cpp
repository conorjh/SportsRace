#include "Audio.h"
#include "spdlog/spdlog.h"

Game::Audio::AudioPlayer::AudioPlayer()
{

}

bool Game::Audio::AudioPlayer::Init()
{
	// Load short effects as chunks and background tracks as streamed music.
	this->Hover = Mix_LoadWAV("media/hover.wav");
	this->Click = Mix_LoadWAV("media/click.wav");
	this->StartersPistol = Mix_LoadWAV("media/starter_pistol.mp3");
	this->CarHorn = Mix_LoadWAV("media/city/isometric/car_horn.wav");
	if (CarHorn == nullptr)
		spdlog::warn("Failed to load ambient traffic horn; continuing without sound: {}", SDL_GetError());
	else
		Mix_VolumeChunk(CarHorn, MIX_MAX_VOLUME / 4);

	this->Catering = Mix_LoadMUS("media/catering.mp3");
	this->Race = Mix_LoadMUS("media/race.mp3");

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
	case BuiltInSounds::CarHorn:
		Chunk = CarHorn;
		break;
	default:
		return;
	}

	if (Chunk != nullptr)
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

	// Loop the background track; screen transitions fade it out.
	if (Chunk != nullptr)
		Mix_PlayMusic(Chunk, 999);
}

void Game::Audio::AudioPlayer::StopMusic()
{
	if (Mix_PlayingMusic())
		Mix_FadeOutMusic(200);
}
