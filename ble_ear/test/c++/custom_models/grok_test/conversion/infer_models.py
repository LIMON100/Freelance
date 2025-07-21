import torch
import torchaudio
import numpy as np
import onnx
import onnxruntime as ort

# Constants
ONNX_MODEL_PATH = 'grok3.onnx'
SAMPLE_RATE = 16000
OUTPUT_SEQ_LEN = 64000
LABELS = ['100미터', '10미터', '12시', '1미터', '1시', '200미터', '20미터', '2시', '300미터', '3미터', '3시', '400미터', '40미터', '5미터', '60미터', '6시', '80미터', '9시', 'IR꺼', 'IR켜', '경계', '경계모드', '공격모드', '느리게', '대기', '드론', '매우느리게', '매우빠르게', '모드변경', '복귀', '빠르게', '사격', '아래', '야간모드', '우로', '우회전', '위', '전방', '전진', '정지', '정찰', '정찰모드', '조명꺼', '조명켜', '조준', '좌로', '좌회전', '주행모드', '후방', '후진']

# Preprocess audio
def preprocess_audio(audio_path):
    waveform, file_sr = torchaudio.load(audio_path)
    if file_sr != SAMPLE_RATE:
        waveform = torchaudio.transforms.Resample(file_sr, SAMPLE_RATE)(waveform)
    waveform = waveform.mean(dim=0)
    waveform = torch.nn.functional.pad(waveform, (0, OUTPUT_SEQ_LEN - waveform.shape[0]))[:OUTPUT_SEQ_LEN]
    transform = torchaudio.transforms.Spectrogram(n_fft=512, hop_length=128, power=None)
    spec = transform(waveform)
    spec = torch.abs(spec)
    spec = spec.unsqueeze(0).unsqueeze(0)
    spec = torch.nn.functional.interpolate(spec, size=(171, 560), mode='bilinear', align_corners=False)
    return spec.squeeze(0).numpy()

# Inference with ONNX
def infer_onnx(audio_path):
    # Load ONNX model
    session = ort.InferenceSession(ONNX_MODEL_PATH)
    
    # Preprocess audio
    spectrogram = preprocess_audio(audio_path)
    
    # Run inference
    inputs = {session.get_inputs()[0].name: spectrogram[None, ...]}
    outputs = session.run(None, inputs)[0]
    probabilities = np.exp(outputs) / np.sum(np.exp(outputs), axis=1, keepdims=True)
    predicted_idx = np.argmax(probabilities, axis=1)[0]
    predicted_label = LABELS[predicted_idx]
    confidence = probabilities[0][predicted_idx]
    
    return predicted_label, confidence

# Example usage
if __name__ == "__main__":
    audio_path = '/home/limonubuntu/Work/Limon/other_task/ble_project/audio_data/merge_korean/all_comand/드론/aug_clean_speaker0_Drone_3_stretch.wav' 
    predicted_label, confidence = infer_onnx(audio_path)
    print(f"Predicted Label: {predicted_label} (Confidence: {confidence:.2f})")