# import queue
# import sys
# import threading
# import numpy as np
# import onnxruntime as ort
# import sounddevice as sd
# import torch
# import torchaudio
# from collections import deque

# # ===================================================================
# #  CONFIGURATION (No changes needed here)
# # ===================================================================
# ONNX_MODEL_PATH = 'models/groknewmodel.onnx'
# LABELS_TXT_PATH = 'models/labels.txt'
# SAMPLE_RATE = 16000
# OUTPUT_SEQ_LEN = 64000
# N_FFT = 512
# HOP_LENGTH = 128
# FINAL_H = 171
# FINAL_W = 560
# VAD_THRESHOLD = 0.01
# VAD_SILENCE_TIMEOUT = 0.5
# CHUNK_SIZE = 1024
# CHANNELS = 1

# # --- NEW VAD PARAMETERS ---
# VAD_PRE_BUFFER_DURATION = 0.2 # seconds
# VAD_PRE_BUFFER_CHUNKS = int(VAD_PRE_BUFFER_DURATION * SAMPLE_RATE / CHUNK_SIZE)
# # ===================================================================
# #  PRE-PROCESSING (This function is correct)
# # ===================================================================
# def preprocess_buffer(audio_buffer: np.ndarray):
#     waveform = torch.from_numpy(audio_buffer).float()
#     if waveform.shape[0] < OUTPUT_SEQ_LEN:
#         waveform = torch.nn.functional.pad(waveform, (0, OUTPUT_SEQ_LEN - waveform.shape[0]))
#     else:
#         waveform = waveform[:OUTPUT_SEQ_LEN]
#     transform = torchaudio.transforms.Spectrogram(n_fft=N_FFT, hop_length=HOP_LENGTH, power=None)
#     spec = torch.abs(transform(waveform))
#     spec = spec.unsqueeze(0).unsqueeze(0)
#     spec = torch.nn.functional.interpolate(spec, size=(FINAL_H, FINAL_W), mode='bilinear', align_corners=False)
#     return spec.numpy()

# # ===================================================================
# #  WORKER THREAD (With the final fix)
# # ===================================================================
# def inference_worker(audio_q: queue.Queue, result_q: queue.Queue, stop_event: threading.Event):
#     session = ort.InferenceSession(ONNX_MODEL_PATH)
#     with open(LABELS_TXT_PATH, "r", encoding="utf-8") as f:
#         labels = [line.strip() for line in f.readlines()]

#     state = "LISTENING"
#     # Use a deque for an efficient rolling buffer
#     pre_buffer = deque(maxlen=VAD_PRE_BUFFER_CHUNKS)
#     command_buffer = []
#     silent_chunks = 0
#     vad_silence_chunks_needed = int(VAD_SILENCE_TIMEOUT * SAMPLE_RATE / CHUNK_SIZE)

#     while not stop_event.is_set():
#         try:
#             chunk = audio_q.get(timeout=0.1)
#             rms = np.sqrt(np.mean(chunk**2))

#             if state == "LISTENING":
#                 pre_buffer.append(chunk)
#                 if rms > VAD_THRESHOLD:
#                     print("Voice detected, recording...", end="", flush=True)
#                     # Voice detected, save the pre-buffer and start recording
#                     command_buffer.extend(list(pre_buffer))
#                     state = "RECORDING"
            
#             elif state == "RECORDING":
#                 command_buffer.append(chunk)
#                 if rms < VAD_THRESHOLD:
#                     silent_chunks += 1
#                     if silent_chunks > vad_silence_chunks_needed:
#                         print(" End of speech. Processing...")
                        
#                         # --- PROCESS THE TRIMMED COMMAND ---
#                         full_command = np.concatenate(command_buffer).flatten()
                        
#                         max_abs = np.max(np.abs(full_command))
#                         if max_abs > 0:
#                             full_command = full_command / max_abs
                        
#                         spectrogram = preprocess_buffer(full_command)
                        
#                         inputs = {session.get_inputs()[0].name: spectrogram}
#                         outputs_batch = session.run(None, inputs)[0]
                        
#                         logits = outputs_batch[0]
#                         exp_logits = np.exp(logits)
#                         probabilities = exp_logits / np.sum(exp_logits)
#                         predicted_idx = np.argmax(probabilities)
#                         confidence = probabilities[predicted_idx]
                        
#                         result_q.put(f"'{labels[predicted_idx]}' (Confidence: {confidence:.2%})")

#                         # --- RESET for next command ---
#                         state = "LISTENING"
#                         command_buffer = []
#                         pre_buffer.clear()
#                 else:
#                     # Reset silence counter if voice is heard again
#                     silent_chunks = 0
#         except queue.Empty:
#             continue

# # ===================================================================
# #  MAIN THREAD (No changes needed)
# # ===================================================================
# def main():
#     audio_queue = queue.Queue(maxsize=50)
#     result_queue = queue.Queue()
#     stop_event = threading.Event()

#     def audio_callback(indata, frames, time, status):
#         if status: print(status, file=sys.stderr)
#         try:
#             audio_queue.put_nowait(indata.copy())
#         except queue.Full:
#             pass

#     worker = threading.Thread(target=inference_worker, args=(audio_queue, result_queue, stop_event))
#     worker.start()

#     try:
#         stream = sd.InputStream(
#             samplerate=SAMPLE_RATE, channels=CHANNELS, dtype='float32',
#             blocksize=CHUNK_SIZE, callback=audio_callback
#         )
#         stream.start()
#         print("Microphone is open. Listening for commands... (Press Ctrl+C to exit)")

#         while not stop_event.is_set():
#             try:
#                 result = result_queue.get(timeout=0.1)
#                 print(f"\n>>> Predicted: {result}")
#                 print("\nListening for commands...")
#             except queue.Empty:
#                 continue

#     except KeyboardInterrupt:
#         print("\nShutting down...")
#         stop_event.set()
#     except Exception as e:
#         print(f"An error occurred: {e}")
#         stop_event.set()
#     finally:
#         if 'stream' in locals() and stream.active:
#             stream.stop()
#             stream.close()
#         worker.join()
#         print("Shutdown complete.")

# if __name__ == "__main__":
#     main()





# import sys
# import numpy as np
# import onnxruntime as ort
# import torch
# import torchaudio

# # ===================================================================
# #  CONFIGURATION (Copied directly from the real-time script for consistency)
# # ===================================================================
# ONNX_MODEL_PATH = 'models/groknewmodel.onnx'
# LABELS_TXT_PATH = 'models/labels.txt'
# SAMPLE_RATE = 16000
# OUTPUT_SEQ_LEN = 64000
# N_FFT = 512
# HOP_LENGTH = 128
# FINAL_H = 171
# FINAL_W = 560

# # ===================================================================
# #  PRE-PROCESSING (Copied directly from the real-time script)
# # ===================================================================
# def preprocess_buffer(audio_buffer: np.ndarray):
#     """
#     This is the function we are testing. It's identical to the one in the
#     real-time script.
#     """
#     waveform = torch.from_numpy(audio_buffer).float()
    
#     if waveform.shape[0] < OUTPUT_SEQ_LEN:
#         waveform = torch.nn.functional.pad(waveform, (0, OUTPUT_SEQ_LEN - waveform.shape[0]))
#     else:
#         waveform = waveform[:OUTPUT_SEQ_LEN]
        
#     transform = torchaudio.transforms.Spectrogram(n_fft=N_FFT, hop_length=HOP_LENGTH, power=None)
#     spec = torch.abs(transform(waveform))
    
#     spec = spec.unsqueeze(0).unsqueeze(0)
#     spec = torch.nn.functional.interpolate(spec, size=(FINAL_H, FINAL_W), mode='bilinear', align_corners=False)
    
#     return spec.numpy()

# # ===================================================================
# #  MAIN INFERENCE LOGIC
# # ===================================================================
# def main():
#     if len(sys.argv) != 2:
#         print(f"Usage: python3 {sys.argv[0]} <path_to_wav_file>")
#         return

#     audio_path = sys.argv[1]

#     # --- 1. Load Model and Labels ---
#     print("Loading ONNX model and labels...")
#     try:
#         session = ort.InferenceSession(ONNX_MODEL_PATH)
#         with open(LABELS_TXT_PATH, "r", encoding="utf-8") as f:
#             labels = [line.strip() for line in f.readlines()]
#     except Exception as e:
#         print(f"Error loading model or labels: {e}")
#         return

#     # --- 2. Load and prepare the single audio file ---
#     print(f"Processing audio file: {audio_path}")
#     try:
#         waveform, sr = torchaudio.load(audio_path)
        
#         # Resample if necessary
#         if sr != SAMPLE_RATE:
#             resampler = torchaudio.transforms.Resample(sr, SAMPLE_RATE)
#             waveform = resampler(waveform)
            
#         # Convert to mono by averaging channels if it's stereo
#         if waveform.shape[0] > 1:
#             waveform = waveform.mean(dim=0)
            
#         # Remove the channel dimension to make it a 1D array
#         waveform = waveform.squeeze()

#         # Convert the final torch tensor to a numpy array for our function
#         audio_buffer_np = waveform.numpy()

#     except Exception as e:
#         print(f"Error loading or processing audio file: {e}")
#         return

#     # --- 3. Run the pre-processing and inference ---
#     print("Pre-processing spectrogram and running inference...")
    
#     # The spectrogram should be a 4D numpy array
#     spectrogram = preprocess_buffer(audio_buffer_np)
    
#     # Run inference with the ONNX model
#     inputs = {session.get_inputs()[0].name: spectrogram}
#     outputs_batch = session.run(None, inputs)[0]
    
#     # --- 4. Post-process the result ---
#     # Extract the first (and only) row of logits from the batch output
#     logits = outputs_batch[0] 

#     # Apply softmax to the simple 1D array
#     exp_logits = np.exp(logits)
#     probabilities = exp_logits / np.sum(exp_logits)

#     predicted_idx = np.argmax(probabilities)
#     confidence = probabilities[predicted_idx]
#     predicted_label = labels[predicted_idx]

#     # --- 5. Print the final result ---
#     print("\n--- Single File Prediction Result ---")
#     print(f">>> Predicted: '{predicted_label}' (Confidence: {confidence:.2%})")
#     print("-----------------------------------")


# if __name__ == "__main__":
#     main()





# 7 rank correct

# import numpy as np
# import onnxruntime as ort
# import time
# import librosa
# import matplotlib.pyplot as plt
# from scipy.signal import butter, lfilter
# from scipy.ndimage import zoom # <--- IMPORT ZOOM FOR INTERPOLATION

# # --- Configuration (Matching PyTorch script) ---
# VAD_THRESHOLD = 0.015 
# SAMPLE_RATE = 16000
# ONNX_MODEL_PATH = "models/groknewmodel.onnx"
# LABELS_PATH = "models/labels.txt"
# TEST_WAV_PATH = "combined_output.wav" 

# # This is the fixed audio length your PyTorch script uses.
# # We will use this instead of our old COMMAND_SAMPLES.
# FIXED_AUDIO_SAMPLES = 64000 

# # --- NEW: AudioPreprocessor - A direct port of your PyTorch code ---
# class AudioPreprocessor:
#     def __init__(self, sample_rate):
#         self.sample_rate = sample_rate
#         # Parameters from your PyTorch script
#         self.n_fft = 512
#         self.hop_length = 128
#         self.target_shape = (171, 560)
#         print("Audio preprocessor initialized (PyTorch replica).")

#     def process(self, audio_chunk):
#         """
#         This is a NumPy/Librosa version of your PyTorch preprocess_audio function.
#         """
#         # 1. Pad/Truncate raw audio to a fixed length (64000 samples)
#         if len(audio_chunk) < FIXED_AUDIO_SAMPLES:
#             audio_chunk = np.pad(audio_chunk, (0, FIXED_AUDIO_SAMPLES - len(audio_chunk)), 'constant')
#         else:
#             audio_chunk = audio_chunk[:FIXED_AUDIO_SAMPLES]
            
#         # 2. Create a linear-frequency magnitude spectrogram
#         #    librosa.stft is equivalent to torchaudio.transforms.Spectrogram(power=None)
#         spec = librosa.stft(y=audio_chunk, n_fft=self.n_fft, hop_length=self.hop_length)
#         spec = np.abs(spec)

#         # 3. CRITICAL: Resize the spectrogram using interpolation
#         #    This mimics torch.nn.functional.interpolate
#         zoom_factors = (self.target_shape[0] / spec.shape[0], self.target_shape[1] / spec.shape[1])
#         spec_resized = zoom(spec, zoom_factors, order=1) # order=1 is bilinear

#         return spec_resized

# # --- VAD functions (unchanged) ---
# def create_high_pass_filter(cutoff, fs, order=5):
#     b, a = butter(order, cutoff / (0.5 * fs), btype='high', analog=False)
#     return b, a
# b, a = create_high_pass_filter(100.0, SAMPLE_RATE)
# def is_speech_present(audio_chunk, energy_threshold):
#     filtered_chunk = lfilter(b, a, audio_chunk)
#     rms = np.sqrt(np.mean(filtered_chunk**2))
#     return rms > energy_threshold

# # --- ONNX Inference (updated to use the new preprocessor) ---
# class OnnxInference:
#     def __init__(self, model_path, labels_path):
#         self.session = ort.InferenceSession(model_path)
#         with open(labels_path, 'r') as f: self.labels = [line.strip() for line in f]
#         input_details = self.session.get_inputs()[0]
#         self.input_name = input_details.name
#         # Instantiate the new, correct preprocessor
#         self.preprocessor = AudioPreprocessor(SAMPLE_RATE)

#     def predict(self, audio_chunk, save_debug_image=False):
#         # The audio_chunk passed here will be the full 4-second clip
#         spectrogram = self.preprocessor.process(audio_chunk)
        
#         # if save_debug_image:
#         #     filename = f"debug_final_spectrogram_{int(time.time() * 1000)}.png"
#         #     plt.imsave(filename, spectrogram, cmap='viridis')
#         #     print(f"  -> Saved final debug image: {filename}")
            
#         input_tensor = spectrogram[np.newaxis, np.newaxis, :, :].astype(np.float32)
#         outputs = self.session.run(None, {self.input_name: input_tensor})[0]
#         predicted_index = np.argmax(outputs)
#         probabilities = np.exp(outputs[0]) / np.sum(np.exp(outputs[0]))
#         return self.labels[predicted_index], probabilities[predicted_index]

# # --- Main Test Function (updated to use FIXED_AUDIO_SAMPLES) ---
# def test_wav_file():
#     inference_engine = OnnxInference(ONNX_MODEL_PATH, LABELS_PATH)
#     print(f"\nLoading test file: {TEST_WAV_PATH}")
#     try:
#         audio, sr = librosa.load(TEST_WAV_PATH, sr=SAMPLE_RATE, mono=True)
#     except Exception as e:
#         print(f"Error loading WAV file: {e}")
#         return
    
#     print(f"File loaded successfully. Duration: {len(audio)/SAMPLE_RATE:.2f} seconds.")
#     print("--- Starting simulated real-time processing ---")

#     is_speaking = False
#     vad_frame_samples = int(1.0 * SAMPLE_RATE) # Use a 1s VAD window
#     current_pos = 0
#     while current_pos + vad_frame_samples < len(audio):
#         vad_chunk = audio[current_pos : current_pos + vad_frame_samples]
#         timestamp_ms = int(current_pos / SAMPLE_RATE * 1000)
        
#         if not is_speaking and is_speech_present(vad_chunk, VAD_THRESHOLD):
#             print(f"\n[{timestamp_ms} ms] Speech DETECTED!")
#             is_speaking = True
            
#             # CRITICAL: Grab the full 4-second (64000 samples) clip for the model
#             command_start = current_pos
#             command_end = min(len(audio), command_start + FIXED_AUDIO_SAMPLES)
#             command_chunk = audio[command_start:command_end]
            
#             label, confidence = inference_engine.predict(command_chunk, save_debug_image=True)
#             print(f"  -> Prediction: '{label.upper()}' (Confidence: {confidence:.2%})")
            
#             # Skip forward past this detected command
#             current_pos += int(2.0 * SAMPLE_RATE) # Skip 2 seconds to find the next command
            
#         elif is_speaking and not is_speech_present(vad_chunk, VAD_THRESHOLD):
#             is_speaking = False
#             current_pos += vad_frame_samples
#         else:
#             current_pos += vad_frame_samples
    
#     print("\n--- Processing complete ---")

# if __name__ == "__main__":
#     test_wav_file()









import numpy as np
import onnxruntime as ort
import time
import librosa
import matplotlib.pyplot as plt
from scipy.signal import butter, lfilter
from scipy.ndimage import zoom

# --- Configuration (Unchanged) ---
VAD_THRESHOLD = 0.015 
SAMPLE_RATE = 16000
ONNX_MODEL_PATH = "models/groknewmodel.onnx"
LABELS_PATH = "models/labels.txt"
TEST_WAV_PATH = "combined_output2.wav"
FIXED_AUDIO_SAMPLES = 64000 # 4 seconds

# --- Helper, Preprocessor, VAD, and Inference classes are UNCHANGED ---
# These components are now considered stable and correct.
def find_energy_center(audio_chunk, frame_length=512, hop_length=256):
    energy = np.array([sum(abs(audio_chunk[i:i+frame_length]**2)) for i in range(0, len(audio_chunk) - frame_length, hop_length)])
    if len(energy) == 0: return len(audio_chunk) // 2
    return np.argmax(energy) * hop_length

class AudioPreprocessor:
    def __init__(self, sample_rate):
        self.sample_rate = sample_rate; self.n_fft = 512; self.hop_length = 128; self.target_shape = (171, 560)
        print("Audio preprocessor initialized (PyTorch replica).")
    def process(self, audio_chunk):
        if len(audio_chunk) < FIXED_AUDIO_SAMPLES: audio_chunk = np.pad(audio_chunk, (0, FIXED_AUDIO_SAMPLES - len(audio_chunk)), 'constant')
        else: audio_chunk = audio_chunk[:FIXED_AUDIO_SAMPLES]
        spec = np.abs(librosa.stft(y=audio_chunk, n_fft=self.n_fft, hop_length=self.hop_length))
        zoom_factors = (self.target_shape[0] / spec.shape[0], self.target_shape[1] / spec.shape[1])
        return zoom(spec, zoom_factors, order=1)

def create_high_pass_filter(cutoff, fs, order=5):
    b, a = butter(order, cutoff / (0.5 * fs), btype='high', analog=False)
    return b, a
b, a = create_high_pass_filter(100.0, SAMPLE_RATE)

def is_speech_present(audio_chunk, energy_threshold):
    if len(audio_chunk) == 0: return False
    filtered_chunk = lfilter(b, a, audio_chunk)
    rms = np.sqrt(np.mean(filtered_chunk**2))
    return rms > energy_threshold

class OnnxInference:
    def __init__(self, model_path, labels_path):
        self.session = ort.InferenceSession(model_path)
        with open(labels_path, 'r') as f: self.labels = [line.strip() for line in f]
        input_details = self.session.get_inputs()[0]
        self.input_name = input_details.name
        self.preprocessor = AudioPreprocessor(SAMPLE_RATE)
    def predict(self, audio_chunk):
        spectrogram = self.preprocessor.process(audio_chunk)
        input_tensor = spectrogram[np.newaxis, np.newaxis, :, :].astype(np.float32)
        outputs = self.session.run(None, {self.input_name: input_tensor})[0]
        predicted_index = np.argmax(outputs)
        probabilities = np.exp(outputs[0]) / np.sum(np.exp(outputs[0]))
        return self.labels[predicted_index], probabilities[predicted_index]


# --- Main Test Function (SIMPLIFIED and more ROBUST) ---
def test_wav_file():
    inference_engine = OnnxInference(ONNX_MODEL_PATH, LABELS_PATH)
    print(f"\nLoading test file: {TEST_WAV_PATH}")
    try:
        audio, sr = librosa.load(TEST_WAV_PATH, sr=SAMPLE_RATE, mono=True)
    except Exception as e:
        print(f"Error loading WAV file: {e}"); return
    
    print(f"File loaded successfully. Duration: {len(audio)/SAMPLE_RATE:.2f} seconds.")
    print("--- Starting simplified processing loop ---")

    vad_scan_samples = int(0.1 * SAMPLE_RATE) # Scan in 100ms steps
    last_detection_end_sample = 0 # Keep track of where the last command ended
    detection_count = 0
    
    current_pos = 0
    while current_pos + vad_scan_samples < len(audio):
        
        # If we are too close to the last detection, jump forward
        if current_pos < last_detection_end_sample:
            current_pos = last_detection_end_sample
            continue

        vad_chunk = audio[current_pos : current_pos + vad_scan_samples]
        
        if is_speech_present(vad_chunk, VAD_THRESHOLD):
            # --- TRIGGER: Speech has been detected! ---
            timestamp_ms = int(current_pos / SAMPLE_RATE * 1000)
            detection_count += 1
            
            # Define a search window around the trigger to find the command center
            search_window_start = max(0, current_pos - int(0.5 * SAMPLE_RATE))
            search_window_end = min(len(audio), current_pos + int(1.5 * SAMPLE_RATE))
            search_window = audio[search_window_start:search_window_end]

            # Find the most energetic point in this search window
            energy_center_in_window = find_energy_center(search_window)
            absolute_energy_center = search_window_start + energy_center_in_window

            # Create the 4-second clip centered on that point
            clip_start = max(0, absolute_energy_center - (FIXED_AUDIO_SAMPLES // 2))
            command_chunk = audio[clip_start : clip_start + FIXED_AUDIO_SAMPLES]
            
            # Run inference
            label, confidence = inference_engine.predict(command_chunk)
            print(f"\n[{timestamp_ms} ms] Detection #{detection_count}: '{label.upper()}' (Confidence: {confidence:.2%})")

            # --- THE SIMPLE, ROBUST JUMP ---
            # Set the new "no-go" zone to be 1.5 seconds AFTER the center of our detected command.
            # This is our new "refractory period", implemented much more simply.
            last_detection_end_sample = absolute_energy_center + int(1.5 * SAMPLE_RATE)
            print(f"  -> Jumping past this command. Next scan starts after ~{int(last_detection_end_sample / SAMPLE_RATE * 1000)} ms.")
            
            current_pos = last_detection_end_sample
        else:
            # No speech, just keep scanning forward
            current_pos += vad_scan_samples
            
    print(f"\n--- Processing complete. Total commands detected: {detection_count} ---")

if __name__ == "__main__":
    test_wav_file()