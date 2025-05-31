from pydub import AudioSegment
import os

def convert_mp3_to_wav():
    input_file = "assets/audio/music/Winter-Long-Version(chosic.com).mp3"
    output_file = "assets/audio/music/background.wav"
    
    # Load MP3 file
    audio = AudioSegment.from_mp3(input_file)
    
    # Export as WAV
    audio.export(output_file, format="wav")
    
    print(f"Converted {input_file} to {output_file}")

if __name__ == "__main__":
    convert_mp3_to_wav() 