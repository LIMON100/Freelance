import numpy as np
import os
import librosa
import cv2
from hailo_sdk_client import ClientRunner

# ===================================================================
#  CONFIGURATION
# ===================================================================
class PreprocessingParams:
    SAMPLE_RATE = 16000
    OUTPUT_SEQ_LEN_SAMPLES = 64000
    N_FFT = 512
    HOP_LENGTH = 128
    FINAL_H = 171
    FINAL_W = 560

# --- Paths ---
# Directory with your original calibration .wav files (NOT the .npy files)
CALIBRATION_WAV_DIR = 'selected_wavs/'
ONNX_MODEL_PATH = 'aug_0805_model.onnx'
OUTPUT_HEF_NAME = 'speech_model_v2.hef' # Give it a new name to avoid confusion
HW_ARCH = 'hailo8'

# ===================================================================
#  PRE-PROCESSING (Matching the C++ code EXACTLY)
# ===================================================================

def normalize_audio(audio):
    """Normalizes the audio waveform to have a maximum absolute value of 1.0."""
    if audio.size == 0: return audio
    max_abs_val = np.max(np.abs(audio))
    if max_abs_val > 1e-5: audio = audio / max_abs_val
    return audio

def preprocess_audio_chunk(audio_chunk, params):
    """Creates the final spectrogram tensor from a chunk of audio."""
    # Ensure chunk is the correct length
    if len(audio_chunk) < params.OUTPUT_SEQ_LEN_SAMPLES:
        padding = np.zeros(params.OUTPUT_SEQ_LEN_SAMPLES - len(audio_chunk))
        audio_chunk = np.concatenate((audio_chunk, padding))
    else:
        audio_chunk = audio_chunk[:params.OUTPUT_SEQ_LEN_SAMPLES]
        
    stft_result = librosa.stft(y=audio_chunk, n_fft=params.N_FFT, hop_length=params.HOP_LENGTH)
    magnitude_spectrogram = np.abs(stft_result)
    resized_spectrogram = cv2.resize(magnitude_spectrogram, (params.FINAL_W, params.FINAL_H), interpolation=cv2.INTER_LINEAR)
    final_input = np.expand_dims(resized_spectrogram, axis=0) # Add batch dim
    final_input = np.expand_dims(final_input, axis=-1)         # Add channel dim for NHWC
    return final_input

# ===================================================================
#  MAIN CONVERSION LOGIC
# ===================================================================
def main():
    params = PreprocessingParams()
    
    # --- 1. Prepare CORRECT Calibration Data ---
    print("--- Preparing Normalized Calibration Dataset ---")
    wav_files = [os.path.join(CALIBRATION_WAV_DIR, f) for f in os.listdir(CALIBRATION_WAV_DIR) if f.endswith('.wav')]
    if not wav_files:
        raise ValueError(f"No .wav files found in calibration directory: {CALIBRATION_WAV_DIR}")
    
    # Process each WAV file just like the C++ code does
    calibration_data = []
    for wav_path in wav_files:
        audio, sr = librosa.load(wav_path, sr=params.SAMPLE_RATE, mono=True)
        
        # <<< THE MOST IMPORTANT STEP >>>
        normalized_audio = normalize_audio(audio)
        
        # We process the whole (short) audio file as a single command chunk
        input_tensor = preprocess_audio_chunk(normalized_audio, params)
        calibration_data.append(input_tensor)
        
    # Stack all tensors into a single numpy array for the compiler
    calib_dataset = np.vstack(calibration_data)
    print(f"Prepared {len(calib_dataset)} samples for calibration. Shape: {calib_dataset.shape}")

    # --- 2. Run Hailo Compiler ---
    print("\n--- Starting Model Conversion with Corrected Data ---")
    runner = ClientRunner(hw_arch=HW_ARCH)
    
    # Parse
    runner.translate_onnx_model(
        ONNX_MODEL_PATH,
        net_input_shapes={'input': [1, 1, 171, 560]} # NCHW for parser
    )
    print("Model Parsed.")
    
    # Quantize using our NEW, NORMALIZED dataset
    runner.optimize(calib_dataset)
    print("Model Quantized.")
    
    # Compile
    hef_bundle = runner.compile()
    print("Model Compiled.")
    
    with open(OUTPUT_HEF_NAME, 'wb') as f:
        f.write(hef_bundle)

    print(f"\n--- SUCCESS! ---")
    print(f"New, correctly quantized HEF file saved to: {OUTPUT_HEF_NAME}")

if __name__ == '__main__':
    main()