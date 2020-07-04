#ifndef SOUND_SDL_MIXER_H
#define SOUND_SDL_MIXER_H
#include "sound_backend.h"
#include <condition_variable>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
struct _Mix_Music;
typedef struct _Mix_Music Mix_Music;
struct Mix_Chunk;

struct MixerThreadMessage {
	enum class Type {
		PLAY_MUSIC,
		PLAY_SOUND,
		TICK,
		TERMINATE
	} type;
	std::string name;
	bool loop;
};

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
	bool ThreadPlayMusic(const std::string& name, bool loop);
	bool ThreadPlaySound(const std::string& name);
	void ThreadTick();
	std::thread mixerThread;
	std::queue<MixerThreadMessage> threadQueue;
	std::mutex mtx;
	std::condition_variable cv;
	std::string cur_music;
	std::map<int, Mix_Chunk*> sounds;
	Mix_Music* music = nullptr;
	int sound_volume, music_volume;
};

#endif //SOUND_SDL_MIXER_H
