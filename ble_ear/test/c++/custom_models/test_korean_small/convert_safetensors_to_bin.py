# convert_safetensors_to_bin.py
import torch
from safetensors.torch import load_file

# --- CONFIGURATION ---
# Path to your local model directory
MODEL_DIR = "/home/limonubuntu/Work/Limon/other_task/ble_project/training/models/06_22/test_korean_small/local_wav2vec2_model" 
# The input .safetensors file
SAFETENSORS_PATH = f"{MODEL_DIR}/model.safetensors"
# The output .bin file we want to create
BIN_PATH = f"{MODEL_DIR}/pytorch_model.bin"

print(f"Loading weights from: {SAFETENSORS_PATH}")
# Load the weights from the .safetensors file
weights = load_file(SAFETENSORS_PATH)

print(f"Saving weights to: {BIN_PATH}")
# Save the weights in the old .bin format
torch.save(weights, BIN_PATH)

print("Conversion complete!")
















