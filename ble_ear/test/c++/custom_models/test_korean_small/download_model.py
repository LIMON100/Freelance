# export_feature_extractor.py
import torch
from torch import nn
from transformers import Wav2Vec2Model

# --- CONFIGURATION ---
# Use the local path to ensure consistency
PRETRAINED_MODEL_PATH = "/home/limonubuntu/Work/Limon/other_task/ble_project/training/models/06_22/test_korean_small/pre-trained" 
OUTPUT_ONNX_PATH = "feature_extractor.onnx"
DEVICE = torch.device("cpu")

# --- Create a wrapper to include the .mean() operation in the graph ---
class Wav2Vec2FeatureExtractorWithMean(nn.Module):
    def __init__(self, model_path):
        super().__init__()
        self.wav2vec2 = Wav2Vec2Model.from_pretrained(model_path)

    def forward(self, input_values):
        # We don't need the attention mask for the base model forward pass
        outputs = self.wav2vec2(input_values)
        # Get the last hidden state and take the mean across the sequence dimension
        mean_embedding = torch.mean(outputs.last_hidden_state, dim=1)
        return mean_embedding

# --- Main Export Logic ---
if __name__ == "__main__":
    print(f"Loading model from: {PRETRAINED_MODEL_PATH}")
    model = Wav2Vec2FeatureExtractorWithMean(PRETRAINED_MODEL_PATH).to(DEVICE)
    model.eval()

    # Create a dummy input for tracing the model
    # Use a fixed length, e.g., 3 seconds, as the NPU will need this later
    dummy_sequence_length = 16000 * 3
    dummy_input = torch.randn(1, dummy_sequence_length, device=DEVICE)

    print(f"Exporting feature extractor to: {OUTPUT_ONNX_PATH}")
    torch.onnx.export(
        model,
        dummy_input,
        OUTPUT_ONNX_PATH,
        input_names=["input_values"],
        output_names=["embedding"],
        dynamic_axes={
            "input_values": {0: "batch_size", 1: "sequence_length"},
            "embedding": {0: "batch_size"},
        },
        opset_version=14,
        export_params=True,
    )
    print("Export complete!")