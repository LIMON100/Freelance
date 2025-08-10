import os
import numpy as np
from hailo_sdk_client import ClientRunner

# --- 1. Configuration ---
# Update these paths to match your project structure
onnx_model_path = '0807_model_less_aug.onnx'
calibration_data_path = 'quant_spectrogram_npy/' # Your folder of .npy files
har_filename_prefix = 'speech_model'
hw_arch = 'hailo8' # Use 'hailo8' for Raspberry Pi, or 'hailo8l' if you have a Hailo-8L

# --- 2. Parsing the ONNX Model ---
print("--- Starting Step 1: Parsing ONNX Model ---")
runner = ClientRunner(hw_arch=hw_arch)
runner.translate_onnx_model(
    onnx_model_path,
    net_input_shapes={'input': [1, 1, 171, 560]} # NCHW format is correct here for the parser
)
parsed_har_path = f'{har_filename_prefix}_parsed.har'
runner.save_har(parsed_har_path)
print(f"Model successfully parsed. Intermediate HAR saved to: {parsed_har_path}\n")


# --- 3. Quantization (Optimization) ---
print("--- Starting Step 2: Quantizing Model ---")
print(f"Loading calibration data from: {calibration_data_path}")
calibration_files = [os.path.join(calibration_data_path, f) for f in os.listdir(calibration_data_path) if f.endswith('.npy')]
if not calibration_files:
    raise ValueError(f"No .npy files found in {calibration_data_path}. Please provide calibration data.")

calib_dataset = np.array([np.load(f) for f in calibration_files])
print(f"Loaded {len(calib_dataset)} samples for calibration. Original 5D shape: {calib_dataset.shape}")


# !!! THIS IS THE FIX !!!
# The .npy files have an extra batch dim, creating a 5D array. Squeeze it back to 4D (N, C, H, W).
# We squeeze axis=1, which is the extra dimension of size 1.
calib_dataset = calib_dataset.squeeze(axis=1)
print(f"Squeezed to 4D NCHW format. New shape: {calib_dataset.shape}")

# Now, transpose the 4D data from NCHW to NHWC for the optimizer
calib_dataset = calib_dataset.transpose((0, 2, 3, 1))
print(f"Transposed calibration data to NHWC format. Final shape: {calib_dataset.shape}")


# Perform quantization using the now correctly-shaped calibration dataset
runner.optimize(calib_dataset)

quantized_har_path = f'{har_filename_prefix}_quantized.har'
runner.save_har(quantized_har_path)
print(f"Model successfully quantized. Intermediate HAR saved to: {quantized_har_path}\n")


# --- 4. Compilation ---
print("--- Starting Step 3: Compiling to HEF ---")
hef_bundle = runner.compile()
hef_path = f'{har_filename_prefix}.hef'
with open(hef_path, 'wb') as f:
    f.write(hef_bundle)

print(f"--- SUCCESS! ---")
print(f"Compilation complete. HEF file saved to: {hef_path}")