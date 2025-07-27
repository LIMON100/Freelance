# preprocess_for_quant.py
import os
import numpy as np
import librosa
from tqdm import tqdm

# --- CONFIGURATION ---
AUDIO_LIST_PATH = 'quant_dataset.txt' # Your list of 300+ .wav files
OUTPUT_NPY_DIR = '/home/limonubuntu/Work/Limon/other_task/ble_project/training/models/quant_spectrogram_npy' # Folder to save the processed .npy files

# Spectrogram parameters
SAMPLE_RATE = 16000
N_FFT = 2048
HOP_LENGTH = 115
N_MELS = 171
OUTPUT_H = 171
OUTPUT_W = 560

os.makedirs(OUTPUT_NPY_DIR, exist_ok=True)

with open(AUDIO_LIST_PATH, 'r') as f:
    audio_files = [line.strip() for line in f]

print(f"Processing {len(audio_files)} audio files into .npy format...")
for audio_path in tqdm(audio_files):
    try:
        y, sr = librosa.load(audio_path, sr=SAMPLE_RATE, mono=True)
        mel_spec = librosa.feature.melspectrogram(y=y, sr=sr, n_fft=N_FFT, hop_length=HOP_LENGTH, n_mels=N_MELS)
        log_mel_spec = librosa.power_to_db(mel_spec, ref=np.max)

        h, w = log_mel_spec.shape
        if w < OUTPUT_W:
            pad_width = OUTPUT_W - w
            
            # --- THE FIX ---
            # Replace mode='min' with mode='constant' and provide the padding value.
            # The minimum value of the entire array represents silence.
            min_val = log_mel_spec.min()
            log_mel_spec = np.pad(log_mel_spec, ((0, 0), (0, pad_width)), mode='constant', constant_values=min_val)
            # --- END OF FIX ---

        elif w > OUTPUT_W:
            log_mel_spec = log_mel_spec[:, :OUTPUT_W]

        final_input = log_mel_spec.astype(np.float32).reshape(1, 1, OUTPUT_H, OUTPUT_W)
        
        base_name = os.path.basename(audio_path).replace('.wav', '.npy')
        np.save(os.path.join(OUTPUT_NPY_DIR, base_name), final_input)

    except Exception as e:
        # We can now see the more specific error message if something else goes wrong.
        print(f"Error processing {audio_path}: {e}")

print("Preprocessing complete.")