#include "SoundSystem.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>

// WAV header structure
struct WAVHeader {
    char riff[4];          // "RIFF"
    uint32_t fileSize;     // File size - 8
    char wave[4];          // "WAVE"
    char fmt[4];           // "fmt "
    uint32_t fmtSize;      // Format chunk size
    uint16_t format;       // Format tag
    uint16_t channels;     // Number of channels
    uint32_t sampleRate;   // Sample rate
    uint32_t byteRate;     // Bytes per second
    uint16_t blockAlign;   // Block alignment
    uint16_t bitsPerSample;// Bits per sample
};

SoundSystem::SoundSystem()
    : device(nullptr)
    , context(nullptr)
    , musicSource(0)
    , currentMusicBuffer(0)
    , masterVolume(1.0f)
    , musicVolume(1.0f)
    , soundEffectVolume(1.0f) {
}

SoundSystem::~SoundSystem() {
    cleanup();
}

bool SoundSystem::initialize() {
    // Open the default audio device
    device = alcOpenDevice(nullptr);
    if (!device) {
        std::cerr << "Failed to open audio device" << std::endl;
        return false;
    }
    std::cout << "Successfully opened audio device" << std::endl;

    // Create audio context
    context = alcCreateContext(device, nullptr);
    if (!context) {
        std::cerr << "Failed to create audio context" << std::endl;
        alcCloseDevice(device);
        return false;
    }
    std::cout << "Successfully created audio context" << std::endl;

    // Make the context current
    if (!alcMakeContextCurrent(context)) {
        std::cerr << "Failed to make audio context current" << std::endl;
        alcDestroyContext(context);
        alcCloseDevice(device);
        return false;
    }
    std::cout << "Successfully made audio context current" << std::endl;

    // Generate source for background music
    alGenSources(1, &musicSource);
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        std::cerr << "Failed to generate music source: " << error << std::endl;
        return false;
    }
    std::cout << "Successfully generated music source" << std::endl;
    
    // Generate sources for sound effects
    alGenSources(MAX_SOUND_SOURCES, soundSources);
    error = alGetError();
    if (error != AL_NO_ERROR) {
        std::cerr << "Failed to generate sound effect sources: " << error << std::endl;
        return false;
    }
    std::cout << "Successfully generated " << MAX_SOUND_SOURCES << " sound effect sources" << std::endl;

    // Set initial properties for music source
    alSourcef(musicSource, AL_GAIN, masterVolume * musicVolume);
    alSourcei(musicSource, AL_LOOPING, AL_FALSE);
    error = alGetError();
    if (error != AL_NO_ERROR) {
        std::cerr << "Failed to set music source properties: " << error << std::endl;
        return false;
    }

    // Set initial properties for sound effect sources
    for (int i = 0; i < MAX_SOUND_SOURCES; ++i) {
        alSourcef(soundSources[i], AL_GAIN, masterVolume * soundEffectVolume);
        alSourcei(soundSources[i], AL_LOOPING, AL_FALSE);
    }
    error = alGetError();
    if (error != AL_NO_ERROR) {
        std::cerr << "Failed to set sound effect source properties: " << error << std::endl;
        return false;
    }

    return true;
}

bool SoundSystem::loadWavFile(const std::string& filePath, ALuint& buffer) {
    std::cout << "Loading WAV file: " << filePath << std::endl;
    
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open WAV file: " << filePath << std::endl;
        return false;
    }

    // Read WAV header
    WAVHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(WAVHeader));

    // Verify RIFF header
    if (strncmp(header.riff, "RIFF", 4) != 0 || strncmp(header.wave, "WAVE", 4) != 0) {
        std::cerr << "Invalid WAV file format: " << filePath << std::endl;
        std::cerr << "RIFF header: " << std::string(header.riff, 4) << std::endl;
        std::cerr << "WAVE header: " << std::string(header.wave, 4) << std::endl;
        return false;
    }
    std::cout << "WAV format verified: " << filePath << std::endl;
    std::cout << "Channels: " << header.channels << std::endl;
    std::cout << "Sample rate: " << header.sampleRate << std::endl;
    std::cout << "Bits per sample: " << header.bitsPerSample << std::endl;

    // Find data chunk
    char chunkId[4];
    uint32_t chunkSize;
    while (file.read(chunkId, 4) && file.read(reinterpret_cast<char*>(&chunkSize), 4)) {
        if (strncmp(chunkId, "data", 4) == 0) {
            break;
        }
        // Skip this chunk
        file.seekg(chunkSize, std::ios::cur);
    }

    if (strncmp(chunkId, "data", 4) != 0) {
        std::cerr << "No data chunk found in WAV file: " << filePath << std::endl;
        return false;
    }
    std::cout << "Found data chunk, size: " << chunkSize << " bytes" << std::endl;

    // Read audio data
    std::vector<char> data(chunkSize);
    file.read(data.data(), chunkSize);
    if (file.gcount() != chunkSize) {
        std::cerr << "Failed to read all audio data. Expected " << chunkSize << " bytes, got " << file.gcount() << std::endl;
        return false;
    }

    // Generate and fill OpenAL buffer
    alGenBuffers(1, &buffer);
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        std::cerr << "Failed to generate OpenAL buffer: " << error << std::endl;
        return false;
    }
    
    // Determine format based on channels and bits per sample
    ALenum format;
    if (header.channels == 1) {
        format = (header.bitsPerSample == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
    } else {
        format = (header.bitsPerSample == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
    }
    std::cout << "Using format: " << (format == AL_FORMAT_MONO8 ? "MONO8" : 
                                    format == AL_FORMAT_MONO16 ? "MONO16" :
                                    format == AL_FORMAT_STEREO8 ? "STEREO8" : "STEREO16") << std::endl;

    alBufferData(buffer, format, data.data(), chunkSize, header.sampleRate);
    error = alGetError();
    if (error != AL_NO_ERROR) {
        std::cerr << "Failed to fill OpenAL buffer: " << error << std::endl;
        alDeleteBuffers(1, &buffer);
        return false;
    }
    std::cout << "Successfully loaded WAV file: " << filePath << std::endl;

    return true;
}

bool SoundSystem::loadMusic(const std::string& name, const std::string& filePath) {
    std::cout << "Loading music: " << name << " from " << filePath << std::endl;
    ALuint buffer;
    if (!loadWavFile(filePath, buffer)) {
        return false;
    }
    
    // Store the buffer
    musicBuffers[name] = buffer;
    std::cout << "Successfully stored music buffer: " << name << std::endl;
    return true;
}

bool SoundSystem::loadSoundEffect(const std::string& name, const std::string& filePath) {
    std::cout << "Loading sound effect: " << name << " from " << filePath << std::endl;
    ALuint buffer;
    if (!loadWavFile(filePath, buffer)) {
        return false;
    }
    
    // Store the buffer
    soundBuffers[name] = buffer;
    std::cout << "Successfully stored sound effect buffer: " << name << std::endl;
    return true;
}

void SoundSystem::playMusic(const std::string& name, bool loop) {
    auto it = musicBuffers.find(name);
    if (it == musicBuffers.end()) {
        std::cerr << "Music not found: " << name << std::endl;
        return;
    }

    // Stop currently playing music
    stopMusic();

    // Attach new buffer and play
    currentMusicBuffer = it->second;
    alSourcei(musicSource, AL_BUFFER, currentMusicBuffer);
    alSourcei(musicSource, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
    alSourcePlay(musicSource);
    
    // Check for errors
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        std::cerr << "Failed to play music: " << error << std::endl;
        return;
    }
    std::cout << "Started playing music: " << name << " (loop: " << (loop ? "true" : "false") << ")" << std::endl;
}

void SoundSystem::stopMusic() {
    alSourceStop(musicSource);
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        std::cerr << "Failed to stop music: " << error << std::endl;
    }
}

void SoundSystem::pauseMusic() {
    alSourcePause(musicSource);
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        std::cerr << "Failed to pause music: " << error << std::endl;
    }
}

void SoundSystem::resumeMusic() {
    ALint state;
    alGetSourcei(musicSource, AL_SOURCE_STATE, &state);
    if (state == AL_PAUSED) {
        alSourcePlay(musicSource);
        ALenum error = alGetError();
        if (error != AL_NO_ERROR) {
            std::cerr << "Failed to resume music: " << error << std::endl;
        }
    }
}

ALuint SoundSystem::findAvailableSoundSource() {
    for (int i = 0; i < MAX_SOUND_SOURCES; ++i) {
        ALint state;
        alGetSourcei(soundSources[i], AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING) {
            return soundSources[i];
        }
    }
    return 0; // No available source found
}

void SoundSystem::playSoundEffect(const std::string& name) {
    auto it = soundBuffers.find(name);
    if (it == soundBuffers.end()) {
        std::cerr << "Sound effect not found: " << name << std::endl;
        return;
    }

    ALuint source = findAvailableSoundSource();
    if (source == 0) {
        std::cerr << "No available sound sources" << std::endl;
        return;
    }

    alSourcei(source, AL_BUFFER, it->second);
    alSourcePlay(source);
}

void SoundSystem::setMasterVolume(float volume) {
    masterVolume = volume;
    setMusicVolume(musicVolume);
    setSoundEffectVolume(soundEffectVolume);
}

void SoundSystem::setMusicVolume(float volume) {
    musicVolume = volume;
    alSourcef(musicSource, AL_GAIN, masterVolume * musicVolume);
}

void SoundSystem::setSoundEffectVolume(float volume) {
    soundEffectVolume = volume;
    for (int i = 0; i < MAX_SOUND_SOURCES; ++i) {
        alSourcef(soundSources[i], AL_GAIN, masterVolume * soundEffectVolume);
    }
}

void SoundSystem::cleanup() {
    // Stop all playback
    alSourceStop(musicSource);
    for (int i = 0; i < MAX_SOUND_SOURCES; ++i) {
        alSourceStop(soundSources[i]);
    }

    // Delete sources
    alDeleteSources(1, &musicSource);
    alDeleteSources(MAX_SOUND_SOURCES, soundSources);

    // Delete buffers
    for (const auto& pair : musicBuffers) {
        alDeleteBuffers(1, &pair.second);
    }
    for (const auto& pair : soundBuffers) {
        alDeleteBuffers(1, &pair.second);
    }

    musicBuffers.clear();
    soundBuffers.clear();

    // Cleanup OpenAL context and device
    if (context) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context);
        context = nullptr;
    }
    if (device) {
        alcCloseDevice(device);
        device = nullptr;
    }
} 