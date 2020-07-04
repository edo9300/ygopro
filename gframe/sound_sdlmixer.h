#ifndef SOUND_SDL_MIXER_H
#define SOUND_SDL_MIXER_H
#include "sound_backend.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
struct _Mix_Music;
typedef struct _Mix_Music Mix_Music;
struct Mix_Chunk;

class SoundMixer : public SoundBackend {
public:
	SoundMixer();
	~SoundMixer();
	void SetSoundVolume(double volume);
	void SetMusicVolume(double volume);
	bool PlayMusic(const std::string& name, bool loop);
	bool PlaySound(const std::string& name);
	void StopSounds();
	void StopMusic();
	void PauseMusic(bool pause);
	bool MusicPlaying();
	void Tick();
private:
	void RunThread();
	void CallAsync(std::function<void()>&& operation);
	std::thread mixerThread;
	std::queue<std::function<void()>> threadQueue;
	std::atomic<bool> exitThread;
	std::mutex mtx;
	std::condition_variable cv;
	std::string cur_music;
	std::map<int, Mix_Chunk*> sounds;
	Mix_Music* music = nullptr;
	int sound_volume, music_volume;
};

#endif //SOUND_SDL_MIXER_H
