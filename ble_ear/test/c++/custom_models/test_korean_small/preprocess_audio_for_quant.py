# preprocess_audio_for_quant.py
import os
import numpy as np
import librosa
from tqdm import tqdm

# --- CONFIGURATION ---
AUDIO_LIST_PATH = 'quant_dataset.txt' # Your list of .wav files
OUTPUT_NPY_DIR = '/home/limonubuntu/Work/Limon/other_task/ble_project/training/models/06_22/rknn/test_korean_small/quant_embeddings_new'       # Folder to save the processed .npy files
FIXED_SHAPE = [1, 16000 * 4]               # The same fixed shape [1, 64000]

os.makedirs(OUTPUT_NPY_DIR, exist_ok=True)

with open(AUDIO_LIST_PATH, 'r') as f:
    audio_files = [line.strip() for line in f]

print(f"Processing {len(audio_files)} audio files for quantization...")
for audio_path in tqdm(audio_files):
    try:
        # Load and resample audio
        speech_array, _ = librosa.load(audio_path, sr=16000, mono=True)
        
        # Pad or truncate to the fixed length
        target_len = FIXED_SHAPE[1]
        if len(speech_array) < target_len:
            pad_width = target_len - len(speech_array)
            speech_array = np.pad(speech_array, (0, pad_width), 'constant')
        elif len(speech_array) > target_len:
            speech_array = speech_array[:target_len]
        
        # Add batch dimension and ensure it's float32
        input_data = np.expand_dims(speech_array, axis=0).astype(np.float32)

        # Save the processed array as a .npy file
        base_name = os.path.basename(audio_path).replace('.wav', '.npy')
        np.save(os.path.join(OUTPUT_NPY_DIR, base_name), input_data)

    except Exception as e:
        print(f"Error processing {audio_path}: {e}")

print("Preprocessing complete.")