#pragma once

#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include <string>
#include <unordered_map>
#include <memory>

class SoundSystem {
public:
    SoundSystem();
    ~SoundSystem();

    // Initialize the sound system
    bool initialize();
    
    // Load audio files
    bool loadMusic(const std::string& name, const std::string& filePath);
    bool loadSoundEffect(const std::string& name, const std::string& filePath);
    
    // Playback control for background music
    void playMusic(const std::string& name, bool loop = true);
    void stopMusic();
    void pauseMusic();
    void resumeMusic();
    void setMusicVolume(float volume); // 0.0f to 1.0f
    
    // Playback control for sound effects
    void playSoundEffect(const std::string& name);
    void setSoundEffectVolume(float volume); // 0.0f to 1.0f
    
    // General controls
    void setMasterVolume(float volume); // 0.0f to 1.0f
    void cleanup();

private:
    // OpenAL context and device
    ALCdevice* device;
    ALCcontext* context;
    
    // Audio source for background music
    ALuint musicSource;
    ALuint currentMusicBuffer;
    
    // Pool of sources for sound effects
    static const int MAX_SOUND_SOURCES = 16;
    ALuint soundSources[MAX_SOUND_SOURCES];
    
    // Storage for loaded audio buffers
    std::unordered_map<std::string, ALuint> musicBuffers;
    std::unordered_map<std::string, ALuint> soundBuffers;
    
    // Utility functions
    bool loadWavFile(const std::string& filePath, ALuint& buffer);
    ALuint findAvailableSoundSource();
    
    // Volume controls
    float masterVolume;
    float musicVolume;
    float soundEffectVolume;
}; 