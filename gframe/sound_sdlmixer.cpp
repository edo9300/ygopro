#ifdef YGOPRO_USE_SDL_MIXER
#include "sound_sdlmixer.h"
#define SDL_MAIN_HANDLED
#include <stdexcept>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <fmt/printf.h>
#include <thread>
#include <atomic>

SoundMixer::SoundMixer() : threadQueue(), exitThread(false), mtx(), cv() {
	SDL_SetMainReady();
	if(SDL_Init(SDL_INIT_AUDIO) < 0)
		throw std::runtime_error("Failed to init sdl audio device!");
	int flags = MIX_INIT_OGG | MIX_INIT_MP3 | MIX_INIT_FLAC;
	int initted = Mix_Init(flags);
	fmt::printf("MIX_INIT_OGG: %s\n", initted&MIX_INIT_OGG ? "true" : "false");
	fmt::printf("MIX_INIT_MP3: %s\n", initted&MIX_INIT_MP3 ? "true" : "false");
	fmt::printf("MIX_INIT_FLAC: %s\n", initted&MIX_INIT_FLAC ? "true" : "false");
	if((initted&flags) != flags) {
		throw std::runtime_error("Not all flags set");
	}
	if(Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096) == -1) {
		fmt::printf("Mix_OpenAudio: %s\n", Mix_GetError());
		throw std::runtime_error("Cannot open channels");
	}
	mixerThread = std::thread(&SoundMixer::RunThread, this);
	mixerThread.detach();
}
void SoundMixer::SetSoundVolume(double volume) {
	CallAsync([this, volume]() {
		sound_volume = (int)(volume * 128);
		Mix_Volume(-1, sound_volume);
	});
}
void SoundMixer::SetMusicVolume(double volume) {
	CallAsync([this, volume]() {
		music_volume = (int)(volume * 128);
		Mix_VolumeMusic(music_volume);
	});
}
bool SoundMixer::PlayMusic(const std::string& name, bool loop) {
	CallAsync([this, name, loop]() {
		if (music && cur_music == name)
			return false;
		if (music) {
			Mix_HaltMusic();
			Mix_FreeMusic(music);
		}
		music = Mix_LoadMUS(name.c_str());
		if (music) {
			if (Mix_PlayMusic(music, loop ? -1 : 0) == -1) {
				Mix_FreeMusic(music);
				return false;
			}
			else {
				cur_music = name;
			}
		}
		else {
			return false;
		}
		return true;
	});
	return true;
}
bool SoundMixer::PlaySound(const std::string& name) {
	CallAsync([this, name]() {
		auto chunk = Mix_LoadWAV(name.c_str());
		if (chunk) {
			auto channel = Mix_PlayChannel(-1, chunk, 0);
			if (channel == -1) {
				Mix_FreeChunk(chunk);
				return false;
			}
			sounds[channel] = chunk;
			Mix_Volume(-1, sound_volume);
		}
		else {
			return false;
		}
		return true;
	});
	return true;
}
void SoundMixer::StopSounds() {
	CallAsync([]() {
		Mix_HaltChannel(-1);
	});
}
void SoundMixer::StopMusic() {
	CallAsync([this]() {
		if (music) {
			Mix_HaltMusic();
			Mix_FreeMusic(music);
			music = nullptr;
		}
	});
}
void SoundMixer::PauseMusic(bool pause) {
	CallAsync([pause]() {
		if (pause)
			Mix_PauseMusic();
		else
			Mix_ResumeMusic();
	});
}
bool SoundMixer::MusicPlaying() {
	return Mix_PlayingMusic();
}
void SoundMixer::Tick() {
	CallAsync([this]() {
		for (auto chunk = sounds.begin(); chunk != sounds.end();) {
			if (Mix_Playing(chunk->first) == 0) {
				Mix_FreeChunk(chunk->second);
				sounds.erase(chunk++);
			}
			else
				chunk++;
		}
	});
}
void KillSwitch(std::atomic_bool& die) {
	std::this_thread::sleep_for(std::chrono::milliseconds(250));
	if(die)
		exit(0);
}
SoundMixer::~SoundMixer() {
	{
		std::unique_lock<std::mutex> lock(mtx);
		exitThread = true;
		cv.notify_one();
	}
	std::atomic_bool die{ true };
	std::thread(KillSwitch, std::ref(die)).detach();
	Mix_HaltChannel(-1);
	Mix_HaltMusic();
	for(auto& chunk : sounds) {
		if(chunk.second)
			Mix_FreeChunk(chunk.second);
	}
	if(music)
		Mix_FreeMusic(music);
	Mix_CloseAudio();
	die = false;
}
void SoundMixer::CallAsync(std::function<void()>&& operation) {
	std::unique_lock<std::mutex> lock(mtx);
	threadQueue.push(std::move(operation));
	cv.notify_one();
}
void SoundMixer::RunThread() {
	while (true) {
		std::function<void()> operation;
		{
			std::unique_lock<std::mutex> lock(mtx);
			while (threadQueue.empty()) {
				cv.wait(lock);
				if (exitThread) {
					return;
				}
			}
			operation = std::move(threadQueue.front());
			threadQueue.pop();
		}
		operation();
	}
}
#endif //YGOPRO_USE_SDL_MIXER
