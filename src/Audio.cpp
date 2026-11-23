#include "Audio.h"
#include "Render/Renderer.h"
#include "spdlog/spdlog.h"

namespace AudioDetail
{
	SDL_IOStream* OpenEmbeddedAssetStream(const std::string& AssetPath)
	{
		const auto AssetData = Game::Assets::LoadEmbeddedAssetData(AssetPath);
		if (AssetData == nullptr)
			return nullptr;

		SDL_IOStream* Stream = SDL_IOFromConstMem(AssetData->data(), AssetData->size());
		if (Stream == nullptr)
			spdlog::warn("Failed to open embedded audio asset '{}': {}", AssetPath, SDL_GetError());
		return Stream;
	}

	Mix_Chunk* LoadSoundEffect(const std::string& AssetPath)
	{
		SDL_IOStream* Stream = OpenEmbeddedAssetStream(AssetPath);
		if (Stream == nullptr)
			return nullptr;

		Mix_Chunk* Chunk = Mix_LoadWAV_IO(Stream, true);
		if (Chunk == nullptr)
			spdlog::warn("Failed to decode embedded sound '{}': {}", AssetPath, SDL_GetError());
		return Chunk;
	}

	Mix_Music* LoadSoundtrack(const std::string& AssetPath)
	{
		SDL_IOStream* Stream = OpenEmbeddedAssetStream(AssetPath);
		if (Stream == nullptr)
			return nullptr;

		Mix_Music* Music = Mix_LoadMUS_IO(Stream, true);
		if (Music == nullptr)
			spdlog::warn("Failed to decode embedded soundtrack '{}': {}", AssetPath, SDL_GetError());
		return Music;
	}
}

Game::Audio::AudioPlayer::AudioPlayer()
{

}

bool Game::Audio::AudioPlayer::Init()
{
	// Load short effects as chunks and background tracks as streamed music.
	int Frequency = 0;
	SDL_AudioFormat Format{};
	int Channels = 0;
	if (!Mix_QuerySpec(&Frequency, &Format, &Channels))
	{
		spdlog::warn("Audio device is unavailable; continuing without sound.");
		return true;
	}

	this->Hover = AudioDetail::LoadSoundEffect("media/hover.wav");
	this->Click = AudioDetail::LoadSoundEffect("media/click.wav");
	this->StartersPistol = AudioDetail::LoadSoundEffect("media/starter_pistol.mp3");
	this->CarHorn = AudioDetail::LoadSoundEffect("media/city/isometric/car_horn.wav");
	if (CarHorn == nullptr)
		spdlog::warn("Failed to load ambient traffic horn; continuing without sound: {}", SDL_GetError());
	else
		Mix_VolumeChunk(CarHorn, MIX_MAX_VOLUME / 4);

	this->Catering = AudioDetail::LoadSoundtrack("media/catering.mp3");
	this->Race = AudioDetail::LoadSoundtrack("media/race.mp3");

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
	int Volume;
	switch (Track)
	{
	case Soundtrack::Catering:
		Chunk = Catering;
		Volume = static_cast<int>(MIX_MAX_VOLUME * 0.40);
		break;
	case Soundtrack::Race:
		Chunk = Race;
		Volume = MIX_MAX_VOLUME;
		break;
	
	default:
		return;
	}

	// Loop the background track; screen transitions fade it out.
	if (Chunk == nullptr)
		return;

	Mix_VolumeMusic(Volume);
	Mix_PlayMusic(Chunk, 999);
}

void Game::Audio::AudioPlayer::StopMusic()
{
	if (Mix_PlayingMusic())
		Mix_FadeOutMusic(200);
}
