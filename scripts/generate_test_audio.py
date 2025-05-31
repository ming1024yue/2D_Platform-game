import numpy as np
from scipy.io import wavfile
import os

def generate_sine_wave(frequency, duration, sample_rate=44100):
    t = np.linspace(0, duration, int(sample_rate * duration), False)
    return np.sin(2 * np.pi * frequency * t)

def create_test_audio():
    # Create directories if they don't exist
    os.makedirs("assets/audio/music", exist_ok=True)
    os.makedirs("assets/audio/sfx", exist_ok=True)
    
    # Generate background music (a simple melody)
    sample_rate = 44100
    duration = 10.0  # 10 seconds
    frequencies = [262, 294, 330, 349]  # C4, D4, E4, F4
    music = np.zeros(int(sample_rate * duration))
    
    for i, freq in enumerate(frequencies):
        start = int(i * sample_rate * 2.5)
        end = int((i + 1) * sample_rate * 2.5)
        music[start:end] = generate_sine_wave(freq, 2.5, sample_rate) * 0.5
    
    # Add fade in/out
    fade_samples = int(sample_rate * 0.1)
    fade_in = np.linspace(0, 1, fade_samples)
    fade_out = np.linspace(1, 0, fade_samples)
    music[:fade_samples] *= fade_in
    music[-fade_samples:] *= fade_out
    
    # Save background music
    wavfile.write("assets/audio/music/background.wav", sample_rate, (music * 32767).astype(np.int16))
    
    # Generate sound effects
    sfx_params = {
        "jump": {"freq": 440, "duration": 0.2},  # A4 note
        "land": {"freq": 220, "duration": 0.3},  # A3 note
        "hit": {"freq": 880, "duration": 0.1},   # A5 note
        "collect": {"freq": 660, "duration": 0.15}  # E5 note
    }
    
    for name, params in sfx_params.items():
        sound = generate_sine_wave(params["freq"], params["duration"], sample_rate)
        
        # Add fade out
        fade_samples = int(sample_rate * 0.05)
        fade_out = np.linspace(1, 0, fade_samples)
        sound[-fade_samples:] *= fade_out
        
        wavfile.write(f"assets/audio/sfx/{name}.wav", sample_rate, (sound * 32767).astype(np.int16))

if __name__ == "__main__":
    create_test_audio() 