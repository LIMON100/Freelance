import os
import torch
import torchaudio
import numpy as np
import onnxruntime
from transformers import Wav2Vec2Processor, Wav2Vec2ForCTC

# ===================================================================
#  CONFIGURATION
# ===================================================================
ONNX_MODEL_PATH = "/home/limonubuntu/Work/Limon/other_task/ble_project/training/models/06_22/test_korean_small/k3_150epoch.onnx"
LABELS_TXT_PATH = "/home/limonubuntu/Work/Limon/other_task/ble_project/training/models/06_22/test_korean_small/labels.txt"
PRETRAINED_MODEL = "/home/limonubuntu/Work/Limon/other_task/ble_project/training/models/06_22/test_korean_small/local_wav2vec2_model" 
SAMPLE_RATE    = 16_000
MAX_AUDIO_LEN  = 4
DEVICE = torch.device("cpu")

# ===================================================================
#  INFERENCE FUNCTION (This part is correct)
# ===================================================================
def predict_from_onnx(wav_path: str, onnx_session, feature_extractor, processor, labels):
    # STAGE 1: FEATURE EXTRACTION
    waveform, sr = torchaudio.load(wav_path)
    waveform = waveform.mean(0, keepdim=True)
    if sr != SAMPLE_RATE:
        waveform = torchaudio.functional.resample(waveform, sr, SAMPLE_RATE)
    waveform = waveform[:, :SAMPLE_RATE * MAX_AUDIO_LEN]
    inputs = processor(
        waveform.squeeze(), sampling_rate=SAMPLE_RATE, return_tensors="pt", padding=True
    )
    with torch.no_grad():
        hidden_states = feature_extractor(inputs.input_values.to(DEVICE)).last_hidden_state
        embedding = hidden_states.mean(dim=1)
    # STAGE 2: CLASSIFICATION
    onnx_input = embedding.cpu().numpy()
    if onnx_input.ndim == 1:
        onnx_input = np.expand_dims(onnx_input, axis=0)
    input_name = onnx_session.get_inputs()[0].name
    logits_onnx = onnx_session.run(None, {input_name: onnx_input})[0]
    predicted_id = np.argmax(logits_onnx, axis=1)[0]
    return labels[predicted_id]


# ===================================================================
#  MAIN EXECUTION BLOCK (Corrected Order)
# ===================================================================
if __name__ == "__main__":
    print(f"--- Running inference on device: {DEVICE} ---")

    # --- Step 1: Check that all required files exist ---
    if not all(os.path.exists(p) for p in [ONNX_MODEL_PATH, LABELS_TXT_PATH, PRETRAINED_MODEL]):
        print("Error: Make sure model files and directories exist:")
        print(f"  - ONNX Model: {ONNX_MODEL_PATH}")
        print(f"  - Labels: {LABELS_TXT_PATH}")
        print(f"  - Feature Extractor: {PRETRAINED_MODEL}")
        exit()

    # --- Step 2: Load the ONNX model for the classifier ---
    print(f"Loading ONNX classifier from: {ONNX_MODEL_PATH}")
    onnx_session = onnxruntime.InferenceSession(ONNX_MODEL_PATH)

    # --- Step 3: Load the labels ---
    print(f"Loading labels from: {LABELS_TXT_PATH}")
    with open(LABELS_TXT_PATH, "r", encoding="utf-8") as f:
        labels = [line.strip() for line in f.readlines()]

    # --- Step 4: Load the feature extractor correctly ---
    print(f"Loading feature extractor from local path: '{PRETRAINED_MODEL}'...")
    processor = Wav2Vec2Processor.from_pretrained(PRETRAINED_MODEL)
    full_model = Wav2Vec2ForCTC.from_pretrained(PRETRAINED_MODEL).to(DEVICE)
    feature_extractor = full_model.wav2vec2
    feature_extractor.eval()

    # --- Step 5: Run prediction on a test file ---
    test_wav_path = "/home/limonubuntu/Work/Limon/other_task/ble_project/audio_data/merge_korean/all_comand/공격모드/aug_clean_Attack_Mode_13_stretch.wav"

    if os.path.exists(test_wav_path):
        print("\n---------------------------------")
        print(f"Predicting command for: {os.path.basename(test_wav_path)}")
        
        # Now all variables (onnx_session, etc.) are defined before being used
        predicted_command = predict_from_onnx(
            wav_path=test_wav_path,
            onnx_session=onnx_session,
            feature_extractor=feature_extractor,
            processor=processor,
            labels=labels
        )
        
        print(f"\n>>> Predicted Command: '{predicted_command}'")
        print("---------------------------------")
    else:
        print(f"\n[!] Test file not found: '{test_wav_path}'")
        print("[!] Please update the 'test_wav_path' variable in the script.")