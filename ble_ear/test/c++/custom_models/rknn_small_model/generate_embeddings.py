# generate_embeddings.py
import os
import numpy as np
import librosa
import onnxruntime
from tqdm import tqdm

# --- CONFIG ---
ONNX_FE_PATH = "/home/limonubuntu/Work/Limon/other_task/ble_project/training/models/06_22/rknn/test_korean_small/models/feature_extractor.onnx"
AUDIO_LIST_PATH = 'quant_dataset.txt'
OUTPUT_EMBED_DIR = 'quant_embeddings'
TARGET_SAMPLE_RATE = 16000

os.makedirs(OUTPUT_EMBED_DIR, exist_ok=True)

# Load the feature extractor ONNX model
session = onnxruntime.InferenceSession(ONNX_FE_PATH)
input_name = session.get_inputs()[0].name

with open(AUDIO_LIST_PATH, 'r') as f:
    audio_files = [line.strip() for line in f]

print(f"Generating embeddings for {len(audio_files)} files...")
for audio_path in tqdm(audio_files):
    # Load and preprocess audio
    speech_array, _ = librosa.load(audio_path, sr=TARGET_SAMPLE_RATE, mono=True)
    input_values = np.expand_dims(speech_array, axis=0).astype(np.float32)

    # Run inference to get the embedding
    embedding = session.run(None, {input_name: input_values})[0]

    # Save the embedding as a .npy file
    base_name = os.path.basename(audio_path).replace('.wav', '.npy')
    np.save(os.path.join(OUTPUT_EMBED_DIR, base_name), embedding)

print("Done. Embeddings saved to ./quant_embeddings/")