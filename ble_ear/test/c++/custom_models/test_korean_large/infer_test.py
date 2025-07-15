import os
import json
import numpy as np
import librosa
import onnxruntime
from scipy.special import softmax

# ===============================================================
#  CONFIGURATION
# ===============================================================
MODEL_DIR = "/home/limonubuntu/Work/Limon/other_task/ble_project/training/models/06_22/test_korean_large/models/"
ONNX_MODEL_PATH = os.path.join(MODEL_DIR, "korean_command_model.onnx")
LABEL_MAP_PATH = os.path.join(MODEL_DIR, "label_mappings.json")

# --- IMPORTANT: Set the path to the WAV file you want to test ---
# You can get a path from your validation set or upload a new file.
# Example: Let's assume you have a test file in the input directory.
# This is just an example path, you MUST change it.
TEST_AUDIO_PATH = "/home/limonubuntu/Work/Limon/other_task/ble_project/audio_data/merge_korean/all_comand/IR꺼/noaug_clean_IR_Off_15.wav"

TARGET_SAMPLE_RATE = 16000

# ===============================================================
#  INFERENCE FUNCTION
# ===============================================================
def predict_command(audio_path, onnx_session, id2label_map):
    """
    Loads an audio file, preprocesses it, and predicts the command using the ONNX model.
    """
    # 1. Load and Preprocess Audio
    try:
        speech_array, sampling_rate = librosa.load(audio_path, sr=TARGET_SAMPLE_RATE, mono=True)
    except Exception as e:
        print(f"Error loading audio file {audio_path}: {e}")
        return None, None
        
    input_values = np.expand_dims(speech_array, axis=0).astype(np.float32)
    
    # --- THIS IS THE FIX ---
    # The ONNX model expects the attention mask to be float32, not int64.
    attention_mask = np.ones(input_values.shape, dtype=np.float32)

    # 2. Run Inference
    onnx_inputs = {
        'input_values': input_values,
        'attention_mask': attention_mask
    }
    
    onnx_outputs = onnx_session.run(None, onnx_inputs)
    logits = onnx_outputs[0]

    # 3. Post-process the Output
    probabilities = softmax(logits, axis=-1)[0]
    predicted_id = np.argmax(probabilities)
    confidence = probabilities[predicted_id]
    predicted_label = id2label_map.get(str(predicted_id), "Unknown")
    
    return predicted_label, confidence

# ===============================================================
#  MAIN EXECUTION BLOCK
# ===============================================================
if __name__ == "__main__":
    if not os.path.exists(ONNX_MODEL_PATH):
        print(f"Error: ONNX model not found at {ONNX_MODEL_PATH}")
    elif not os.path.exists(LABEL_MAP_PATH):
        print(f"Error: Label mapping file not found at {LABEL_MAP_PATH}")
    elif not os.path.exists(TEST_AUDIO_PATH):
        print(f"Error: Test audio file not found at {TEST_AUDIO_PATH}")
        print("Please update the 'TEST_AUDIO_PATH' variable with a valid path to a .wav file.")
    else:
        print("Loading ONNX model and label mappings...")
        session = onnxruntime.InferenceSession(ONNX_MODEL_PATH)
        with open(LABEL_MAP_PATH, 'r', encoding='utf-8') as f:
            label_data = json.load(f)
        id2label = label_data['id2label']
        print("Model and mappings loaded successfully.")

        print(f"\nPredicting command for: {os.path.basename(TEST_AUDIO_PATH)}")
        predicted_label, confidence = predict_command(TEST_AUDIO_PATH, session, id2label)

        if predicted_label:
            print("\n--- Prediction Result ---")
            print(f"Predicted Command: '{predicted_label}'")
            print(f"Confidence: {confidence:.2%}")
            print("-------------------------")