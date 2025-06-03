from gtts import gTTS
import os
import subprocess
import tempfile

# Define Korean commands
commands = [
    ("위로", "up"),
    ("아래로", "down"),
    ("시작", "start"),
    ("중지", "stop"),
    ("볼륨 업", "volume_up"),
    ("볼륨 다운", "volume_down"),
    ("유지", "hold"),
    ("종료", "exit"),
    ("일시정지", "pause"),
    ("재개", "resume")
]

# Temporary directory for intermediate files
temp_dir = tempfile.mkdtemp()

# Generate individual audio files
wav_files = []
for text, name in commands:
    # Generate MP3 with gTTS
    tts = gTTS(text=text, lang='ko')
    mp3_path = os.path.join(temp_dir, f"{name}.mp3")
    tts.save(mp3_path)
    
    # Convert to WAV (16kHz, mono, PCM 16-bit)
    wav_path = os.path.join(temp_dir, f"{name}.wav")
    subprocess.run([
        "ffmpeg", "-y", "-i", mp3_path,
        "-ar", "16000", "-ac", "1", "-c:a", "pcm_s16le", wav_path
    ], check=True)
    wav_files.append(wav_path)

# Create silence audio (1 second)
silence_path = os.path.join(temp_dir, "silence.wav")
subprocess.run([
    "ffmpeg", "-y", "-f", "lavfi", "-i", "anullsrc=r=16000:cl=mono",
    "-t", "1", "-c:a", "pcm_s16le", silence_path
], check=True)

# Create concatenation list with silence between commands
concat_list = []
for i, wav in enumerate(wav_files):
    concat_list.append(wav)
    if i < len(wav_files) - 1:  # No silence after the last command
        concat_list.append(silence_path)

# Write concat list to a text file
concat_file = os.path.join(temp_dir, "concat.txt")
with open(concat_file, "w") as f:
    for path in concat_list:
        f.write(f"file '{path}'\n")

# Concatenate all files into final WAV
output_wav = "korean_commands.wav"
subprocess.run([
    "ffmpeg", "-y", "-f", "concat", "-safe", "0", "-i", concat_file,
    "-c:a", "pcm_s16le", "-ar", "16000", "-ac", "1", output_wav
], check=True)

print(f"Generated WAV file: {output_wav}")
